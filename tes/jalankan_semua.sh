#!/usr/bin/env bash
# tes/jalankan_semua.sh — jalankan semua tes Loop; gagal bila ada yang tidak lulus.
#
# Sebuah tes dianggap lulus bila exit code 0 DAN keluarannya memuat baris
# "SEMUA TES ... SELESAI". Tes yang crash di tengah (exit non-nol) atau berhenti
# diam-diam sebelum selesai sama-sama dihitung gagal.
#
# Pakai (dari akar repo):
#   gcc -O2 -o seed/loop seed/loop.c -lm -lssl -lcrypto -lpthread
#   tes/jalankan_semua.sh                 # memakai ./seed/loop
#   LOOP=./seed/loop_asan tes/jalankan_semua.sh   # memakai build ASan/UBSan
set -u
cd "$(dirname "$0")/.."
LOOP="${LOOP:-./seed/loop}"
export LOOP_STDLIB="$PWD/Library"
[ -x "$LOOP" ] || { echo "Biner Loop tidak ditemukan: $LOOP (build dulu, lihat README)"; exit 2; }

gagal=0
lulus=0
ECHO_PID=""

mulai_echo() {
  if ! command -v python3 >/dev/null 2>&1; then
    echo "PERINGATAN: python3 tidak ada, tes yang butuh server echo (jaringan.lp, library/net.lp) DILEWATI"
    return 1
  fi
  python3 - <<'PY' &
import socket, threading
s = socket.socket(); s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(('127.0.0.1', 9999)); s.listen(8); s.settimeout(300)
def h(c):
    d = c.recv(4096); c.sendall(b'ECHO:' + d); c.close()
try:
    while True:
        c, _ = s.accept(); threading.Thread(target=h, args=(c,), daemon=True).start()
except Exception:
    pass
PY
  ECHO_PID=$!
  sleep 0.7
  return 0
}
trap '[ -n "$ECHO_PID" ] && kill "$ECHO_PID" 2>/dev/null' EXIT
mulai_echo && ADA_ECHO=1 || ADA_ECHO=0

jalankan_tes() {
  local f="$1" out rc
  out=$("$LOOP" "$f" 2>&1); rc=$?
  if [ $rc -eq 0 ] && printf '%s\n' "$out" | grep -qE 'SEMUA TES .* SELESAI'; then
    echo "LULUS  $f"
    lulus=$((lulus + 1))
  else
    echo "GAGAL  $f (exit $rc)"
    printf '%s\n' "$out" | tail -15 | sed 's/^/    | /'
    gagal=$((gagal + 1))
  fi
}

periksa() {  # periksa "nama" <kondisi-shell>
  local nama="$1"; shift
  if "$@"; then echo "LULUS  $nama"; lulus=$((lulus + 1))
  else echo "GAGAL  $nama"; gagal=$((gagal + 1)); fi
}

for f in tes/*.lp tes/library/*.lp tes/contoh/*.lp; do
  case "$f" in
    tes/jaringan.lp|tes/library/net.lp)
      if [ "$ADA_ECHO" -ne 1 ]; then echo "LEWAT  $f (butuh python3 untuk server echo)"; continue; fi ;;
  esac
  jalankan_tes "$f"
done

# ---- pemeriksaan tingkat proses ----
T=$(mktemp -d)
trap '[ -n "$ECHO_PID" ] && kill "$ECHO_PID" 2>/dev/null; rm -rf "$T"' EXIT

# 1. galat tak tertangkap: jejak dicetak ke stderr, exit code 1
cat > "$T/takTertangkap.lp" <<'LP'
fungsi dalam() {
    lempar galat("GalatJaringan", "gagal konek ke 127.0.0.1:1")
}
fungsi tengah() {
    dalam()
}
cetak("sebelum")
tengah()
cetak("TIDAK BOLEH SAMPAI SINI")
LP
out=$("$LOOP" "$T/takTertangkap.lp" 2>"$T/err.txt"); rc=$?
periksa "galat tak tertangkap: exit code 1" [ $rc -eq 1 ]
periksa "galat tak tertangkap: stdout hanya sebelum galat" [ "$out" = "sebelum" ]
periksa "galat tak tertangkap: stderr memuat jenis dan pesan" grep -q '^\[GALAT\] GalatJaringan: gagal konek ke 127.0.0.1:1' "$T/err.txt"
periksa "galat tak tertangkap: jejak memuat nama fungsi dan baris" grep -qE 'di dalam \(.*takTertangkap.lp:2\)' "$T/err.txt"
periksa "galat tak tertangkap: jejak memuat pemanggil dan program utama" grep -qE 'di tengah \(.*:5\)' "$T/err.txt"

# 2. builtin yang gagal tanpa coba juga keluar dengan exit code 1 dan jenis yang benar
echo 'cetak(bacaFileTeks("/tidak/ada/sama-sekali"))' > "$T/berkashilang.lp"
"$LOOP" "$T/berkashilang.lp" >/dev/null 2>"$T/err.txt"; rc=$?
periksa "builtin gagal tanpa coba: exit 1" [ $rc -eq 1 ]
periksa "builtin gagal tanpa coba: GalatBerkas" grep -q 'GalatBerkas' "$T/err.txt"

# 3. RNG: dua proses terpisah menghasilkan keluaran berbeda (dulu rand() tidak di-seed)
a=$("$LOOP" tes/rng/cetak_acak.lp); b=$("$LOOP" tes/rng/cetak_acak.lp)
periksa "RNG: dua proses menghasilkan keluaran berbeda" [ "$a" != "$b" ]
periksa "RNG: panjang acakAman(16) = 32 hex" [ "$(printf '%s' "${a%% *}" | wc -c)" -eq 32 ]

# 4. batas ukuran yang dapat dikonfigurasi: melewati batas = GalatBatas, bukan pemotongan
cat > "$T/batas.lp" <<'LP'
milik t = ""
ulang 300 { t = t + "0123456789" }
coba {
    milik u = t + t
    cetak("TIDAK MELEMPAR")
} tangkap {
    cetak(jenisGalat(e))
}
LP
periksa "LOOP_MAKS_UKURAN: melewati batas melempar GalatBatas" [ "$(LOOP_MAKS_UKURAN=5000 "$LOOP" "$T/batas.lp")" = "GalatBatas" ]
periksa "batas bawaan 64 MB: 6000 karakter tidak masalah" [ "$("$LOOP" "$T/batas.lp")" = "TIDAK MELEMPAR" ]

# 5. literal teks di sumber yang melebihi batas lexer: galat, bukan buffer overflow
python3 - "$T/literal.lp" <<'PY' 2>/dev/null || printf 'cetak("%s")\n' "$(head -c 5000 /dev/zero | tr '\0' x)" > "$T/literal.lp"
import sys
open(sys.argv[1], 'w').write('cetak("' + 'x' * 5000 + '")\n')
PY
"$LOOP" "$T/literal.lp" >/dev/null 2>"$T/err.txt"; rc=$?
periksa "literal teks 5000 karakter: GalatBatas, exit 1" bash -c "[ $rc -eq 1 ] && grep -q GalatBatas '$T/err.txt'"

# 6. rekursi sangat dalam: GalatBatas yang bisa ditangkap, bukan segfault
cat > "$T/rekursi.lp" <<'LP'
fungsi f(n) {
    kembali f(n + 1)
}
coba { f(0) } tangkap { cetak(jenisGalat(e)) }
LP
periksa "rekursi tanpa batas: GalatBatas, bukan segfault" [ "$("$LOOP" "$T/rekursi.lp" 2>&1)" = "GalatBatas" ]

echo
echo "Ringkasan: $lulus lulus, $gagal gagal"
if [ "$gagal" -ne 0 ]; then exit 1; fi
echo "SEMUA TES LOOP SELESAI"

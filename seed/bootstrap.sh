#!/usr/bin/env bash
# seed/bootstrap.sh — rantai self-host Loop:
#
#   gen0  loop.c (C)        menjalankan compiler.lp  ->  gen1.nasm  -> biner compiler gen1
#   gen1  compiler native   mengkompilasi compiler.lp ->  gen2.nasm
#   cek   gen1.nasm == gen2.nasm   (fixpoint: compiler native menghasilkan keluaran
#                                   yang sama dengan compiler yang dijalankan interpreter)
#   cek   program di tes/selfhost/*.lp: keluaran native == keluaran interpreter
#
# Butuh: gcc, nasm, ld (binutils), libssl-dev. Jalankan dari mana saja.
#   seed/bootstrap.sh                # membangun seed/loop bila belum ada
#   LOOP=./seed/loop_asan seed/bootstrap.sh
# Catatan: compiler.lp membaca Library/_bawaan.lp relatif terhadap direktori kerja,
# jadi skrip ini selalu berjalan dari akar repo.
set -eu
cd "$(dirname "$0")/.."
LOOP="${LOOP:-./seed/loop}"
export LOOP_STDLIB="$PWD/Library"
for alat in nasm ld; do
  command -v "$alat" >/dev/null 2>&1 || { echo "bootstrap: '$alat' tidak ditemukan"; exit 2; }
done
if [ ! -x "$LOOP" ]; then
  echo "bootstrap: membangun $LOOP"
  gcc -O2 -o "$LOOP" seed/loop.c -lm -lssl -lcrypto -lpthread
fi
T="$(mktemp -d)"; trap 'rm -rf "$T"' EXIT

rakit() {  # rakit <nasm> <biner>
  nasm -f elf64 "$1" -o "$1.o" && ld "$1.o" -o "$2"
}

echo "[gen0] interpreter menjalankan compiler.lp -> compiler.lp"
"$LOOP" seed/compiler.lp seed/compiler.lp -o "$T/gen1.nasm" >/dev/null
rakit "$T/gen1.nasm" "$T/compiler_gen1"
echo "[gen1] biner compiler native: $(wc -c < "$T/compiler_gen1") byte"

echo "[gen1] compiler native mengkompilasi compiler.lp"
"$T/compiler_gen1" seed/compiler.lp -o "$T/gen2.nasm" >/dev/null
if cmp -s "$T/gen1.nasm" "$T/gen2.nasm"; then
  echo "FIXPOINT: gen1.nasm == gen2.nasm ($(wc -c < "$T/gen1.nasm") byte)"
else
  echo "GAGAL: gen1.nasm != gen2.nasm (compiler native tidak mereproduksi dirinya)"; exit 1
fi

gagal=0
for f in tes/selfhost/*.lp; do
  "$LOOP" "$f" > "$T/interp.txt" 2>&1 || true
  "$T/compiler_gen1" "$f" -o "$T/p.nasm" >/dev/null
  rakit "$T/p.nasm" "$T/p.bin"
  "$T/p.bin" > "$T/native.txt" 2>&1 || true
  if cmp -s "$T/interp.txt" "$T/native.txt"; then echo "LULUS  $f (native == interpreter)"
  else echo "GAGAL  $f"; diff "$T/interp.txt" "$T/native.txt" | head -5; gagal=1; fi
done

# fitur yang hanya ada di native (mis. arr[i] = x): keluaran dibandingkan dengan berkas .keluaran
for f in tes/selfhost/native/*.lp; do
  "$T/compiler_gen1" "$f" -o "$T/n.nasm" >/dev/null
  rakit "$T/n.nasm" "$T/n.bin"
  "$T/n.bin" > "$T/n.txt" 2>&1 || true
  if cmp -s "$T/n.txt" "${f%.lp}.keluaran"; then echo "LULUS  $f (sesuai ${f%.lp}.keluaran)"
  else echo "GAGAL  $f"; diff "$T/n.txt" "${f%.lp}.keluaran" | head -5; gagal=1; fi
done

# galat tak tertangkap di biner native: pesan di stderr dan exit code 1
# (berkas *.contoh tidak ikut loop di atas karena stderr-nya sengaja berbeda dari interpreter)
for f in tes/selfhost/*.lp.contoh; do
  "$T/compiler_gen1" "$f" -o "$T/u.nasm" >/dev/null
  rakit "$T/u.nasm" "$T/u.bin"
  set +e; "$T/u.bin" > "$T/u.out" 2> "$T/u.err"; rc=$?; set -e
  if [ "$rc" -eq 1 ] && [ "$(cat "$T/u.out")" = "sebelum" ] && grep -q '^\[GALAT\] GalatJaringan: gagal konek' "$T/u.err"; then
    echo "LULUS  $f (galat tak tertangkap: exit 1 + stderr)"
  else
    echo "GAGAL  $f (rc=$rc)"; cat "$T/u.err"; gagal=1
  fi
done

[ "$gagal" -eq 0 ] || exit 1
echo "SEMUA TES SELF-HOST SELESAI"

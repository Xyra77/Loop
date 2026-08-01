# Catatan Progress — Loop

Status: **self-hosting penuh tercapai + interpreter punya mode interaktif (REPL) + builtin core disamakan dengan syntax highlighter (vsix v1.4.0).**

## 0. Sinkronisasi builtin core dengan `loop-lang-1_4_0.vsix` — BARU

`syntaxes/loop.tmLanguage.json` di extension VS Code (v1.4.0) mendokumentasikan builtin
"core" (kategori io/string/larik/math/bitwise/meta di `builtin-fungsi`) yang sebagian besar
belum ada di `loop.c`. Sudah ditambahkan ~57 builtin baru supaya `loop.c` sinkron:

- **IO**: `tambahFileTeks`, `appendFile`, `tulis`, `tulisInline`, `tulisStderr`, `tulisError`,
  `barisBaru`, `waktumilisdetik`, `waktuSekarang`, `waktuISO`, `tidur`.
- **String**: `keAngka`, `keDesimal`, `keTeks`, `dariKode`, `potong`, `cariTeks`,
  `gantiTeks`/`ganti`, `hurufBesar`/`naik`, `hurufKecil`/`turunkan`, `pisah`,
  `trimTeks`/`trim`, `mulaiDengan`, `akhiriDengan`, `regexGanti`, `regexCocok`
  (pakai POSIX `<regex.h>`, `REG_EXTENDED`), `terakhirIndexDari`.
- **Larik**: `dorong`, `hapusTerakhir`, `hapusKunci`, `hapusLarik`, `iris` (larik & teks),
  `adaDalam`, `saring`, `peta`, `kurangi` (tiga terakhir higher-order — panggil balik nilai
  fungsi lewat helper baru `panggil_nilai_fungsi()`), `jenisNilai` (alias `tipe`),
  `kunciObjek`, `salin` (deep copy).
- **Matematika**: `lantai`, `langit`, `absolut`, `acakAngka`, `log`, `kosinus`, `sinus`,
  `akarKuadrat`, `PI`.
- **Bitwise**: `bitAND`, `bitOR`, `bitXOR`, `bitKiri`, `bitKanan`, `bitNOT`.
- **Meta/sistem**: `_sistemEnvGet/Set/Hapus/Semua`, `_sistemArgumen`, `_sistemDirKerja`,
  `_sistemPindahDir`, `_sistemPathSep`, `_sistemNamaOS`, `_sistemArsitektur`,
  `_sistemJumlahCPU`, `_sistemPID`, `_sistemKeluar`, `_sistemJalankan`.

**Sengaja DILEWATI** (belum diimplementasikan): `syscall` dan `syscallStr`. Alasan:
`spesifikasi/level-rendah.lp` sudah menyatakan syscall langsung "masih desain, belum
diimplementasi — milestone besar berikutnya", jadi menambahkannya sekarang akan mendahului
keputusan desain itu.

**Fungsi Library (crypto/net/os/format/log/json/struktur/larik-lanjutan/teksutil/
matika-lanjutan/acak/waktu/berkas/pinout) TIDAK disentuh** — itu ekosistem terpisah yang
menurut catatan di bawah memang sengaja dikeluarkan dari paket ini (`Library/`, dst).
Builtin rendah yang mendukungnya (`prima`, `acak`, `ganda`, `sambung`, `tlsSambung`, `kirim`,
`terima`, `tutupKoneksi`, `ambil`, `telusuri`) sudah ada sebelumnya dan tidak diubah.

### Rename `rayapi` ==> `telusuri`

Builtin web crawler (dulu `rayapi`, alias dari `crawlURL`) diganti nama jadi `telusuri` —
lebih jelas maknanya (menelusuri link) dibanding `rayapi`. Diubah konsisten di `loop.c`
(nama builtin terdaftar + pesan error) dan di vsix (`networking.ts`, `loop.tmLanguage.json`,
`readme.md`). Bukan alias ganda — nama lama `rayapi` sudah tidak dikenali lagi.

### Keyword deklarasi `milik` — DITAMBAHKAN

vsix (snippet & hover docs) selalu memakai `milik nama = nilai` untuk deklarasi, tapi
`loop.c` sebelumnya hanya menerima assignment polos (`nama = nilai`, auto-declare). Sekarang
`milik nama = nilai` didukung sebagai token/keyword baru (`TK_MILIK`) — secara semantik
sama persis dengan assignment polos (declare-or-update di scope aktif), tidak menambah
makna const/immutability baru (field `is_const` di `Node` masih belum dipakai di mana pun).
`milik` jadi reserved word, jadi tidak bisa dipakai sebagai nama variabel.

Regresi sudah dicek: `test_orig.lp` lama tetap jalan, dan self-hosting 3 generasi
(`loop.c ==> self.nasm ==> loopc ==> self2.nasm ==> loopc2 ==> self3.nasm`) tetap identik (md5 sama)
setelah semua perubahan ini.

## 1. Self-hosting — TERCAPAI

```
gcc loop.c ==> loop (C, dipakai SEKALI doang buat bootstrap)
loop compiler.lp compiler.lp -o self.nasm   ==> assemble+link ==> loopc (generasi 2, native)
loopc  compiler.lp -o self2.nasm            ==> assemble+link ==> loopc2 (generasi 3, ZERO C)
loopc2 program.lp -o out.nasm               ==> assemble+link ==> jalan, hasil BENAR
```

Diverifikasi jalan sampai generasi 3-4, stabil, dan **sudah dicoba ulang langsung di mesin
lokal (bukan cuma di sandbox)** — hasilnya konsisten.

### Bug-bug yang ditemukan & diperbaiki buat nyampe titik ini (urutan kronologis)

**Di `loop.c` (C, cuma dipakai sekali buat bootstrap awal):**
1. `bootstrap.sh` salah nama file — diganti ke nama file yang beneran ada.
2. 8 builtin yang dipanggil `compiler.lp` tapi belum ada di interpreter (`karakter`,
   `kodeKarakter`, `bulatkan`, `irisLarik`, `gabungTeks`, `bacaFileTeks`, `tulisFileTeks`,
   `cetakTanpaBaris`) — ditambahin.
3. `argumen` (argv) gak pernah di-expose ke script Loop — di-inject ke global env.
4. Bug argv-overwrite: argumen non-flag ke-2/3/dst nimpa `filepath` yang mau dieksekusi.
5. Use-after-free di interpolasi string interpreter (`free()` dipanggil sebelum parser
   selesai baca tokennya).

**Di `compiler.lp` (Loop, compiler self-hosted — bug paling banyak & paling penting di sini):**
6. Array gak punya metadata panjang di backend NASM ==> dibikin heap-allocated + header.
7. `panjang()`/`tambah()` cuma numeric, salah buat string/array ==> dibikin polymorphic.
8. String literal punya newline siluman ke-append otomatis ==> dipisah dari fast-path cetak.
9. String equality (`==`/`!=`) cuma bandingin alamat pointer, bukan isi ==> dibikin
   pembanding byte-by-byte (`__setara`).
10. Binary native gak baca argv dari OS sama sekali ==> ditambahin pembaca argc/argv
    mentah di awal `_start`.
11. **Off-by-one di `cg_selama`/`cg_ulang`/`cg_untuk`** — logic "pop" stack continue/break
    salah ngitung index, bikin `lewati`/`hentikan` di loop bersarang nyasar ke label yang
    salah ==> infinite loop.
12. **Interpolasi string (`#{...}`) gak diimplementasi sama sekali di NASM codegen** —
    cuma jalan di interpreter, padahal `compiler.lp` pakai `#{...}` di mana-mana buat
    generate label/offset dinamis. Diimplementasi penuh (tokenizer split jadi node
    `STRINTERP`, parser re-parse fragmen ekspresi, codegen evaluasi+gabung saat runtime).
13. **BUG PALING BESAR — variabel global gak bisa diakses dari dalam fungsi.** Tabel
    variabel di-reset total tiap masuk fungsi baru, jadi variabel yang dideklarasi di
    top-level (`P_tokens`, `P_pos`, dst) "keputus" begitu diakses dari dalam fungsi manapun.
    Ini SATU-SATUNYA alasan self-hosting gagal total sebelum fix ini. Solusinya: sistem
    variabel global sungguhan yang nyimpen di alamat tetap (`.bss`), bisa diakses dari
    fungsi manapun tanpa peduli frame stack siapa yang lagi aktif.
14. `tambah()` (array append) itu O(n²) — tiap append nyalin ulang SELURUH array. Gak
    kerasa buat file kecil, tapi buat self-compile `compiler.lp` sendiri (puluhan ribu
    baris NASM ke-generate) butuh gigabyte memori dan keliatan kayak infinite loop
    (padahal cuma lambat banget). Fix: array sekarang punya field KAPASITAS terpisah
    dari PANJANG, growth dobel kapasitas kalau penuh (amortized O(1), standar dynamic
    array/vector).

## 2. Mode interaktif (REPL) — DITAMBAHKAN

`loop` (tanpa argumen, dijalankan di terminal interaktif) sekarang punya REPL persis
kayak `python`/`ruby`:

```
$ loop
Loop 0.6 (main, Aug  1 2026 10:27:59) [GCC 16.1.1 20260430] on linux
Ketik pernyataan Loop. Ctrl-D buat keluar.
>>> nama = "Ngawi"
>>> cetak "Halo saya " + nama
Halo saya Ngawi
```

Detail implementasi:
- Banner versi otomatis ambil tanggal/jam/versi GCC compile pakai macro `__DATE__`,
  `__TIME__`, `__VERSION__` (persis kayak Python nunjukin info build-nya).
- Variabel & fungsi yang dideklarasi nempel antar baris (satu `Env` global dipakai
  terus-menerus selama sesi).
- Deteksi blok belum lengkap: `fungsi ... {` atau `jika ... maka` otomatis ganti prompt
  jadi `...` (continuation) sampai `}`/`akhir` penutupnya ketemu.
- Error runtime/parse **gak lagi nge-kill proses**. Sebelumnya `loop_error()` langsung
  `exit()`. Sekarang di mode REPL dia `longjmp` balik ke prompt (pakai `setjmp`), jadi
  typo kecil gak bikin sesi harus dibuka ulang.
- Kalau dipanggil non-interaktif (di-pipe/di-script, `isatty()` false), tetap nunjukin
  pesan penggunaan biasa — gak ngerusak pemakaian scripted lama.

**Penting:** `loopc`/`loopc2` (hasil self-compile) **BUKAN pengganti** `loop` yang punya
REPL. REPL itu kode C murni di `loop.c`, bukan bagian dari `compiler.lp` — jadi binary
hasil self-compile cuma bisa dipakai buat compile, gak punya mode interaktif.

## 3. Kebersihan kode

Semua garis komentar dekoratif (`═══`, `───`, em dash `—`) di `loop.c` dan `compiler.lp`
udah dibuang, sisa teks penjelasan polos + `*/` — biar konsisten sama gaya komentar ASCII
biasa di seluruh file.

## 4. Struktur folder (per keputusan: cuma bahasa + pondasi compiler)

```
Loop/
├── seed/
│   ├── loop.c        ==> interpreter + REPL + NASM codegen bootstrap (C, sekali pakai)
│   └── compiler.lp   ==> compiler RESMI, ditulis dalam Loop, self-hosted
└── spesifikasi/
    ├── sintaks.lp        ==> desain sintaks dasar
    ├── fitur-target.lp   ==> roadmap fitur
    └── level-rendah.lp   ==> desain tier low-level (belum diimplementasi)
```

`bootstrap.sh` udah dibuang dari paket ini — build manual (lihat bawah) lebih jelas dan
gampang di-debug ketimbang script yang gampang ketinggalan zaman.

Yang SENGAJA dibuang dari paket ini karena bukan bagian inti bahasa+compiler (proyek
ekosistem terpisah yang numpang di atas bahasa ini): `Library/` (wrapper .lp buat
crypto/net/dll), `loop_runtime/` (C runtime buat Library), `tes/`, `testi/` (file
percobaan lama), `src/` (stub placeholder yang udah digantikan `seed/compiler.lp`),
`build/` (binary hasil compile, regenerable), `run.txt` (catatan rencana lama yang
isinya udah kadaluarsa, digantikan file ini).

## Cara build & pakai

Butuh header dev OpenSSL (dipakai builtin TLS: `tlsSambung`, dll). Di Ubuntu/Debian:
`apt-get install libssl-dev`.

```bash
cd seed
gcc -O2 -o loop loop.c -lm -lssl -lcrypto

# Pemakaian sehari-hari — interpreter + REPL:
./loop                    # masuk mode interaktif (>>> ...)
./loop program.lp         # jalanin file langsung

# Opsional: install ke PATH biar bisa dipanggil dari mana aja
sudo cp loop /usr/local/bin/loop

# Self-hosting (opsional, buat verifikasi/eksperimen — bukan buat dipakai harian):
./loop compiler.lp compiler.lp -o self.nasm
nasm -f elf64 self.nasm -o self.o
ld self.o -o loopc              # binary compiler native, NAMA BEDA dari "loop"
./loopc program.lp -o out.nasm  # loopc cuma bisa compile, gak ada mode interaktif
```

## Catatan buat pengembangan lanjut

- Heap arena runtime 200MB (bisa diperkecil lagi setelah fix O(n²) di atas, tapi aman
  dibiarin segitu).
- Type-checking masih longgar (heuristic "nilai < 0x100000 = angka, >= itu = pointer").
  Cukup buat sekarang, tapi rapuh kalau ada angka murni >1 juta.
- Belum ada `free()`/garbage collection di runtime hasil compile — bump allocator doang.
  Gak masalah buat compiler yang jalan sekali lalu exit.
- `spesifikasi/level-rendah.lp` (pointer, `mentah{}`, syscall langsung, `asm{}`) masih
  desain, belum diimplementasi — itu milestone besar berikutnya.
- Kalau mau `loopc`/hasil self-compile punya REPL juga, itu perlu ditulis di
  `compiler.lp` sendiri (bahasa Loop) — REPL sekarang cuma ada di sisi `loop.c` (C).

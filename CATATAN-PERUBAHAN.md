# Catatan perubahan: galat berjenis, tipe bita, kripto native

Menerapkan dokumen desain "Konsep Error dan Library Loop" (28 Okt 2026) ke
`seed/loop.c` dan `Library/`. Enam tahap dikerjakan berurutan; bagian
"Verifikasi" memisahkan apa yang diuji dari apa yang belum.

## Keputusan terbuka di dokumen dan pilihan yang diambil

| Keputusan | Pilihan | Alasan |
|---|---|---|
| Jalur interpreter | **Jalur A** (`setjmp`/`longjmp` di C) | port C++ tidak ada di zip; jalur A dijadikan jembatan |
| Akses galat | fungsi `jenisGalat(e)` dst. | akses properti `e.jenis` belum ada |
| Backend `compiler.lp` | rantai self-host diperbaiki dan galat native dibangun | lihat "Self-host" di bawah |
| Hapus `_bawaan.lp` (tahap 5 dokumen) | **tidak dilakukan** | dibutuhkan `compiler.lp`; tidak bisa dimuat lewat `pakai`, tetapi dibaca langsung oleh compiler |
| Nama baru | `galat`, `bita`, `akhirnya`, `lempar`, `GalatKunci` dipertahankan | mengikuti dokumen |

## Yang dikerjakan per tahap

1. **Galat berjenis.** Satu jalur `loop_lempar()`; stack handler untuk `coba`.
   Kata kunci baru `lempar` dan `akhirnya` (`tangkap` boleh dihilangkan bila ada
   `akhirnya`). Objek galat: jenis, pesan, lokasi, jejak, sebab. Builtin baru:
   `galat`, `jenisGalat`, `pesanGalat`, `lokasiGalat`, `sebabGalat`, `jejakGalat`.
   Mekanisme `g_error` lama dihapus. 175 titik `loop_galat`/`LOOP_ASSERT` menjadi
   `GalatInternal`; lebih dari 120 pemeriksaan argumen ("butuh ...", "harus ...") menjadi `GalatTipe`;
   titik yang dipakai library dipasang jenis tepat (`GalatBerkas`, `GalatJaringan`,
   `GalatNilai`, `GalatIndeks`, `GalatBatas`). Galat tak tertangkap mencetak
   `[GALAT] Jenis: pesan` + jejak (`di fungsi (berkas:baris)`) dan keluar dengan
   kode 1. Rekursi sekarang dibatasi (`GalatBatas`, 4000 tingkat).
2. **Teks dinamis dan tipe bita.** `val_ke_teks()` tanpa batas; `+`, `+=`,
   `tambah`, interpolasi, `gabungTeks`, `cetak*` tidak lagi memotong. Batas atas
   64 MiB (`LOOP_MAKS_UKURAN`) melempar `GalatBatas`. Tipe `bita`
   (`keBita`, `keHex`, `dariHex`, `keBase64`, `dariBase64`, `panjangBita`,
   `potongBita`, `sambungBita`, indeks `b[i]`, `==`, deklarasi `bita x = ...`).
3. **Berkas.** `bacaBita`, `tulisBita`, `tulisAtomik` (temp + fsync + rename),
   `pindahBerkas`, `adaBerkas`, `hapusBerkas`. `bacaFileTeks`/`tulisFileTeks`/
   `appendFile`/`tambahFileTeks` melempar `GalatBerkas`; teks berisi 0x00
   melempar `GalatNilai` alih-alih terpotong.
4. **Kripto native** (OpenSSL EVP): `acakAman`, `turunkanKunci` (PBKDF2-HMAC-SHA256),
   `hmacSha256`, `enkripsiGCM`/`dekripsiGCM` (AES-256-GCM, nonce acak 12 byte,
   tag gagal = `GalatKunci`), `samaWaktuKonstan`. `acakAngka` kini di-seed per proses.
5. **Library dirapikan.** `crypto.lp` menjadi pembungkus; AES murni-Loop pindah ke
   `contoh/aes_murni.lp`. `_bawaan.lp` **dipertahankan** dan diperbaiki (dokumen menyuruh menghapusnya,
   tetapi `seed/compiler.lp` membacanya lewat path sebagai runtime builtin untuk
   binary native; tujuan self-host lebih penting). Duplikat builtin dibuang dari
   `dasar.lp` (76 → 12 fungsi) dan `bridge.lp` (40 → 23), termasuk stub `lempar` dan
   placeholder `formatWaktu`/`tungguSemua`. `io.lp` memakai `adaBerkas`/`tulisAtomik`.
   `net.lp` ditulis ulang: timeout, galat berjenis, TLS terverifikasi.
6. **Tes, CI, README.** `Library/pastikan.lp`, `tes/jalankan_semua.sh`,
   `.github/workflows/ci.yml` (build rilis dan ASan/UBSan), README tanpa klaim
   yang belum terbukti.

## Perubahan yang mematahkan kode lama

| Perubahan | Dulu | Sekarang | Jembatan |
|---|---|---|---|
| `bacaFileTeks` berkas tidak ada | `""` | `GalatBerkas` | `adaBerkas(path)` |
| Isi `e` di `tangkap` | teks pesan | objek galat | `"" + e` = `Jenis: pesan` |
| Format `.aes` | CBC, hex teks, IV tetap | GCM biner: garam(16) nonce(12) ct tag(16) | `dekripsiBerkasV0` |
| `lempar()` | fungsi stub | kata kunci | stub dihapus |
| `enkripsiFileAES`/`dekripsiFileAES` | nama lama | `enkripsiBerkas`/`dekripsiBerkas` | alias dipertahankan dua rilis |
| `buatKunci(nilai)` | membuat kunci prima | `buatKunci(sandi, garam)` menurunkan kunci | nama lama jadi `buatKunciPrima(nilai)` |
| `enkripsi`/`dekripsi`/`enkripsiAngka`/`enkripsiPakaiCipherPrima` | berbasis AES-CBC murni | berbasis GCM; hasil tetap hex teks, tidak kompatibel dengan data lama | tidak ada |
| Fungsi internal AES (`enkripsiAES`, `hexKeBytes`, `ekspansiKunci`, ...) | di `crypto.lp` | di `contoh/aes_murni.lp` | `bacaFile("contoh/aes_murni.lp")` |
| `x /= 0`, `x % 0`, `x / 0` | `0`, `nan`, atau berhenti | `GalatNilai` | |
| Literal teks > 4093 karakter, tidak ditutup, atau berisi `\0` | meluap/terpotong diam-diam | `GalatBatas`/galat | |
| `ambil("ftp://...")` | diperlakukan sebagai http | `GalatNilai` | |
| Sertifikat TLS | tidak diverifikasi | diverifikasi (nama host juga) | `LOOP_TLS_TIDAK_AMAN=1` hanya untuk uji |
| `kirim`/`terima` gagal | `kirim` berhenti dengan exit(1) dan tidak menangani kirim sebagian; `terima` mengembalikan `""` | `GalatJaringan` (timeout juga); `kirim` mengirim semua data; EOF tetap `""` | |
| Pembungkus `.lp` untuk builtin `waktu()`/`tanggal()`/`sleep()` yang tidak ada (`formatWaktu` dkk.) | ada | dihapus | builtin `waktuSekarang`, `tidur` |
| Build | `-lm -lssl -lcrypto` | tambah `-lpthread` | |

## Tambahan di luar dokumen (perlu persetujuan atau dibuang bila tidak dikehendaki)

- `hapusBerkas`, `terimaBita`, `ambilBita`, argumen timeout opsional pada
  `sambung`/`tlsSambung`/`ambil`.
- `dekripsiCBCLama` (builtin sementara agar `dekripsiBerkasV0` cepat dan benar;
  dihapus bersama `dekripsiBerkasV0` setelah dua rilis).
- Interpreter berjalan di thread berstack 768 MiB; dulu rekursi ~250 tingkat sudah
  segfault. Batas `MAX_CALL_STACK` 256 → 4000.
- Kerentanan yang ditemukan saat bekerja dan ditutup: lexer menulis melewati
  `Token.val[4096]` untuk literal panjang, buffer angka 64 byte dan identifier tanpa
  pemeriksaan, buffer codegen dipotong di 512 byte dan bisa meluap, `+` dan `+=`
  memotong dalam buffer tetap, `val_format` memotong elemen larik di 256 karakter.

## Self-host

Sebelum perubahan ini `compiler.lp` tidak bisa mengkompilasi program apa pun (diuji
pada zip asli dengan interpreter asli). Penyebab dan perbaikannya:

1. `compiler.lp` memanggil `p_makan("IFIKASI")` (12 tempat) padahal tokenizer
   menghasilkan `IDENTIFIKASI`: diperbaiki.
2. `Library/_bawaan.lp` (pustaka builtin yang ikut dikompilasi ke binary native)
   berisi stub palsu (`cetak(...args)`, `waktuSekarang`, `lempar`, `tungguSemua`),
   tipe `boolean` yang bukan keyword (seharusnya `bool`), dan memanggil
   `karakterDariKode` yang tidak ada: stub dibuang, `boolean` jadi `bool`, dan
   `karakterDariKode` jadi `dariKode`.
3. Compiler belum punya `dariKode` native dan literal `null`: ditambahkan rutin asm
   `dariKode` (kode angka -> teks 1 karakter) dan literal `null` (dipetakan ke 0, karena
   runtime native tidak punya tipe null terpisah).
4. **Galat native** (keputusan terbuka nomor 3 di dokumen). Mekanisme lama (flag
   `__g_error` yang dicek per pernyataan, tidak pernah melintasi fungsi) diganti:
   - Tumpukan handler di stack: `coba` mendorong catatan 24 byte
     `[prev][rbp][alamat catch]` dan `__handler_top` menunjuk ke sana. `__lempar`
     memulihkan `rsp`/`rbp` dari catatan terbaru dan melompat ke catch dengan objek galat
     di `rax`; tanpa handler ia memanggil `__galatTakTertangkap` (cetak
     `[GALAT] Jenis: pesan` ke stderr, keluar kode 1).
   - Sintaks sama dengan interpreter: `coba { } tangkap { } akhirnya { }`
     (`tangkap` opsional bila ada `akhirnya`), dan `lempar`. Sintaks lama
     (`coba ... tangkap ... akhir`) tetap diterima.
   - `kembali`/`hentikan`/`lewati` yang keluar dari wilayah `coba` melepas handler dan
     menjalankan badan `akhirnya` secara inline (nilai `rax` dijaga).
   - Objek galat adalah larik `["__GALAT__", jenis, pesan, sebab]` buatan fungsi di
     `_bawaan.lp` (`galat`, `jenisGalat`, `pesanGalat`, `sebabGalat`, `lokasiGalat`).
   - Pembagian/modulo dengan nol dan berkas gagal dibuka kini melempar `GalatNilai` dan
     `GalatBerkas`. Indeks larik (baca **dan** tulis) dicek: di luar batas atau bukan
     larik melempar `GalatIndeks`/`GalatTipe`; sebelumnya membaca/menulis memori sembarang.

Hasil (`seed/bootstrap.sh`, diukur di sandbox ini):

- Interpreter menjalankan `compiler.lp` pada dirinya sendiri: 792.446 byte NASM, sekitar
  1 menit. Dirakit dengan `nasm` + `ld` menjadi biner compiler native 280 KB.
- Compiler native mengkompilasi `compiler.lp` dalam beberapa detik dan menghasilkan NASM
  yang **identik byte-per-byte** dengan generasi pertama (fixpoint), juga setelah galat
  native dan pemeriksaan indeks ditambahkan.
- `tes/selfhost/*.lp` (contoh README, rekursi, larik, loop, interpolasi, fungsi dari
  `_bawaan.lp`, dan `galat.lp` dengan 23 baris keluaran): keluaran native identik dengan
  interpreter, termasuk 100.000 iterasi `coba` dengan galat dan `kembali` di dalamnya
  (membuktikan tumpukan handler seimbang). `native/indeks_tulis.lp` (assignment berindeks
  yang tidak ada di interpreter) dan galat tak tertangkap (exit 1 + stderr) juga lulus.

Belum dicoba: bootstrap di bawah ASan (sangat lambat), arsitektur selain x86-64 Linux.

## Verifikasi

Dijalankan di sandbox ini (Ubuntu, gcc, OpenSSL 3.0):

- `tes/jalankan_semua.sh`: **27 lulus, 0 gagal** pada build `-O2` dan pada build
  `-fsanitize=address,undefined -fno-sanitize-recover=undefined` (leak detection mati).
- Vektor standar: RFC 4231 kasus 1, 2, 3, 6; RFC 7914 bagian 11 (dua vektor PBKDF2);
  McGrew-Viega GCM kasus 13, 14, 15; RFC 4648 base64; FIPS-197 Appendix B dan C.1
  untuk AES murni-Loop. Nilai pembanding dihitung ulang dengan Python
  `hashlib`/`cryptography` sebelum dipakai.
- Tes lama (`io`, `string`, `larik`, `matematika`, `sistem`) menghasilkan keluaran
  yang sama dengan interpreter asli (selain PID).
- Migrasi: berkas `.aes` lama yang dibuat dengan `crypto.lp` asli (fixture di
  `tes/fixtures/`) terbaca oleh `dekripsiBerkasV0`. IV berkas lama itu identik
  antar proses, mengonfirmasi temuan audit.
- Angka rekomendasi di dokumen: 600.000 iterasi PBKDF2-HMAC-SHA256 (OWASP) dan batas
  sekitar 2^32 pemanggilan GCM per kunci dengan IV acak (NIST SP 800-38D) cocok
  dengan sumber sekunder yang dicari. Halaman primer OWASP dan NIST **belum dibuka
  langsung**; cek ulang sebelum dikutip sebagai rujukan.

Belum diverifikasi:

- Workflow CI belum pernah berjalan di GitHub (hanya perintah-perintahnya yang
  dijalankan lokal).
- Hanya Linux x86-64. macOS/BSD (`mkstemp`, `MSG_NOSIGNAL`, `pthread`) tidak dicoba.
- Jalur TLS sukses (https ke host nyata) tidak diuji karena sandbox tanpa internet;
  hanya jalur gagal. `tes/manual/internet.lp` untuk dicoba manual.
- Pengukuran kecepatan AES-NI ("megabita dalam hitungan detik") tidak dilakukan
  secara terpisah; enkripsi + dekripsi 1 MiB termasuk dalam `tes/library/crypto.lp`
  dan seluruh suite itu selesai dalam beberapa detik.

## Keterbatasan yang diketahui

- **Galat native lebih sederhana dari interpreter:** tanpa jejak pemanggilan dan
  `lokasiGalat(e)` selalu `""`; `tipe(e)` menghasilkan `"larik"` (interpreter: `"galat"`);
  galat internal runtime selain bagi nol, berkas, dan indeks (mis. keluar memori) belum
  berjenis; `lempar` pada nilai non-galat menjadi `GalatTipe`.
- Interpreter `loop.c` tidak mendukung assignment berindeks (`arr[i] = x`) yang ada di native
  (perbedaan lama antara kedua sisi, bukan dari perubahan ini).
- Beberapa kata dicadangkan compiler tetapi tidak oleh interpreter (mis. `dalam`); program
  yang dipakai di kedua sisi sebaiknya tidak memakainya sebagai nama.
- Backend native hanya mendukung subset bahasa (21 builtin native + fungsi dari
  `_bawaan.lp`); builtin seperti `bacaBita`, kripto, dan jaringan hanya ada di interpreter.
- **Jalur A bocor sedikit per galat tertangkap**: Env milik frame yang dilewati
  `longjmp` dibebaskan, sisa sekitar 200 byte per galat (nilai sementara). Diukur
  dari 1,08 GB menjadi 1,08 MB pada `tes/galat.lp`. Untuk server yang berjalan
  lama tunggu port C++.
- Interpreter belum membebaskan AST/token/variabel global saat keluar, jadi
  LeakSanitizer melaporkan kebocoran dasar (juga pada interpreter asli). CI mematikan
  `detect_leaks`; kesalahan memori dan UB tetap fatal.
- `ambil()` tidak mendekode `Transfer-Encoding: chunked`; body dapat memuat penanda chunk.
- `acak()`, `prima()`, dan tipe `ganda`/`terenkripsi` adalah cipher mainan, bukan
  keamanan. Keamanan datang dari GCM.
- Satu enkripsi/dekripsi butuh sekitar 0,1-0,5 detik karena PBKDF2 600.000 iterasi
  (disengaja). Untuk banyak pesan kecil pakai satu `buatKunci` + `enkripsiGCM`.
- Teks Loop tetap string C: tidak bisa berisi byte 0x00 (pakai `bita`).
- `telusuri(url, kedalaman)`: argumen kedalaman diterima tetapi tidak dipakai (perilaku lama).

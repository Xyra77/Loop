# Tes Loop

Jalankan semuanya dari akar repo:

```bash
gcc -O2 -o seed/loop seed/loop.c -lm -lssl -lcrypto -lpthread
tes/jalankan_semua.sh                       # memakai ./seed/loop
LOOP=./seed/loop_asan tes/jalankan_semua.sh # build ASan/UBSan
```

`jalankan_semua.sh` menjalankan setiap `tes/*.lp`, `tes/library/*.lp`, dan
`tes/contoh/*.lp`. Sebuah tes lulus bila exit code 0 **dan** keluarannya memuat
baris `SEMUA TES ... SELESAI`. Setelah itu ia memeriksa perilaku tingkat proses
(galat tak tertangkap: jejak + exit code 1, RNG berbeda antar proses, batas
`LOOP_MAKS_UKURAN`, literal terlalu panjang, rekursi tanpa batas).

| Berkas | Isi |
|---|---|
| `galat.lp` | lempar, coba/tangkap/akhirnya, galat berjenis dari builtin |
| `bita.lp` | tipe bita, base64 (RFC 4648), teks > 4096 karakter tidak terpotong |
| `berkas.lp` | round-trip biner (0, 1, 16, 4096, 4097 byte, 64 KiB, 10 MB, berisi 0x00), tulisAtomik |
| `library/crypto.lp` | RFC 4231, RFC 7914, AES-GCM McGrew-Viega, jalur gagal, migrasi format lama |
| `library/net.lp` | GalatJaringan tertangkap, server echo lokal |
| `library/io.lp`, `library/dasar.lp` | pembungkus library |
| `contoh/aes_murni.lp` | blok AES murni-Loop vs FIPS-197 Appendix B dan C.1 |
| `io.lp`, `string.lp`, `larik.lp`, `matematika.lp`, `sistem.lp`, `jaringan.lp` | tes builtin (lama) |
| `manual/internet.lp` | HTTP ke internet; manual, tidak ikut CI |
| `fixtures/lama.txt.aes` | berkas format `.aes` lama untuk tes `dekripsiBerkasV0` |

Tes jaringan memerlukan server echo di `127.0.0.1:9999`; `jalankan_semua.sh`
menyalakannya dengan python3 (bila tidak ada, tes itu dilewati dengan peringatan).

Rantai self-host dijalankan terpisah oleh `seed/bootstrap.sh` (butuh nasm).
Input ada di `tes/selfhost/`: `*.lp` dijalankan lewat interpreter dan lewat biner native
lalu keluarannya harus identik; `native/*.lp` (fitur yang hanya ada di native) dibandingkan
dengan berkas `.keluaran`; `*.lp.contoh` memeriksa galat tak tertangkap (exit 1 + stderr).

Alat tes: `Library/pastikan.lp` (`pastikan`, `pastikanSama`, `gagalTes`, `selesaiTes`).

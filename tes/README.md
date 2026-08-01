# tes/ — Tes builtin per kategori

Folder ini isinya file `.lp` yang nge-tes fungsi builtin `loop.c`, dikelompokkan
berdasarkan jenis penggunaannya (bukan cuma satu file gede isi semua).

| File | Kategori | Isi |
|---|---|---|
| `io.lp` | IO | `cetak`, `cetakTanpaBaris`, `tulis`, `tulisInline`, `tulisStderr`, `tulisError`, `barisBaru`, file (`bacaFileTeks`/`tulisFileTeks`/`appendFile`/`tambahFileTeks`/`bacaFile`/`tulisFile`), waktu (`waktumilisdetik`/`waktuSekarang`/`waktuISO`), `tidur`, `jalankan`/`eksekusi` |
| `string.lp` | String | konversi tipe, karakter↔kode, potong/cari/ganti, ubah huruf, pisah/gabung/trim, prefix/suffix, regex, penggabungan teks |
| `larik.lp` | Larik (array) | statistik (`jumlah`/`maks`/`min`/`rerata`), `urut`/`balik`, tambah/hapus elemen, `iris`, pencarian, higher-order (`saring`/`peta`/`kurangi`), `tipe`/`salin`, akses indeks |
| `matematika.lp` | Matematika & bitwise | pembulatan, trig/log/akar, angka acak, operasi bit |
| `sistem.lp` | Meta/sistem | env var, direktori kerja, info OS/CPU/PID, argumen CLI, jalankan perintah shell |
| `jaringan.lp` | Jaringan & kripto dasar | `prima`/`acak`/`ganda`, socket TCP (`sambung`/`kirim`/`terima`/`tutupKoneksi`), HTTP GET (`ambil`/`telusuri`) |

## Cara jalankan

```bash
cd seed
gcc -O2 -o loop loop.c -lm -lssl -lcrypto
cd ..
./seed/loop tes/io.lp
./seed/loop tes/string.lp
./seed/loop tes/larik.lp
./seed/loop tes/matematika.lp
./seed/loop tes/sistem.lp
```

`tes/jaringan.lp` beda sendiri — **perlu 2 hal aktif sebelum dijalankan**:

1. Server TCP echo lokal di `127.0.0.1:9999` (buat tes `sambung`/`kirim`/`terima`/`tutupKoneksi`):
   ```bash
   python3 -c "
   import socket
   s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
   s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
   s.bind(('127.0.0.1', 9999)); s.listen(1)
   conn, addr = s.accept()
   conn.sendall(b'ECHO:' + conn.recv(4096)); conn.close(); s.close()
   " &
   ```
2. Koneksi internet aktif (buat tes `ambil`/`telusuri` — keduanya HTTP GET beneran ke `https://github.com`).

Kalau salah satu syarat itu nggak ada, bagian terkait akan berhenti dengan
`[LOOP ERROR] ... gagal konek` — itu wajar/diharapkan, bukan bug di `loop.c`.

## Yang sengaja TIDAK dites otomatis

- **`masukan()`** — butuh input interaktif dari terminal (stdin), gak bisa
  di-otomasi lewat skrip. Ada contoh pemakaian manual di `tes/io.lp`.
- **`_sistemKeluar()`** — bakal langsung `exit()` proses `loop` itu sendiri,
  jadi kalau dites otomatis bakal motong eksekusi tes-tes berikutnya.
  Ada catatan cara coba manual di `tes/sistem.lp`.
- **`syscall`/`syscallStr`** — belum diimplementasikan di `loop.c` (lihat
  `CATATAN-PROGRESS.md`), jadi belum ada yang bisa dites.

## Temuan selama nulis tes ini

- **`jalankan` (keyword statement)** cuma ALIAS dari `cetak` (nge-print teks
  doang) — TIDAK menjalankan perintah shell walau namanya begitu. Yang
  benar-benar eksekusi shell command adalah fungsi **`eksekusi()`**.
- **`bacaFile(path)` (keyword statement)** bukan pembaca teks file — dia
  benar-benar **MENJALANKAN** file `.lp` lain sebagai skrip (mirip
  import/exec). Kalau cuma mau baca isi file sebagai teks, pakai
  `bacaFileTeks(path)`.
- **`telusuri(url)`** (dulu `rayapi`) mengembalikan **larik berisi semua
  link `href="..."`** yang ditemukan di halaman, bukan body HTML mentah.
  Kalau mau body mentah, pakai `ambil(url)`.
- Dokumentasi hover di vsix bilang `telusuri(url, kedalaman)` (2 argumen),
  tapi implementasi di `loop.c` cuma menerima 1 argumen (`url`) — argumen
  `kedalaman` diabaikan sepenuhnya kalau dikirim.

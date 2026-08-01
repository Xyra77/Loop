// spesifikasi/fitur-target.lp
// Target pengembangan LOOP v0.4

// ── SELESAI ✓ ─────────────────────────────
1.  Variabel: x = 5
2.  Tipe data: teks, angka, bool, larik
3.  Tipe eksplisit opsional: angka x = 5 | teks nama = "x"
4.  Ekspresi: 2 + 3 * 4, perbandingan, logika
5.  Kontrol alur: jika/maka/selain/akhir
6.  Loop: selama, ulang N { }
7.  Fungsi user-defined dengan kembali
8.  String interpolasi: "Halo #{nama}!"
9.  pilih/kasus/lainnya (switch-case)
10. coba/tangkap (error handling)
11. Operator pipa |> : data |> jumlah()
12. hentikan / lewati (break / continue)
13. Built-in: panjang, jumlah, maks, min, rerata, urut, balik, tipe, angkaTeks, teksAngka
14. bacaFile / tulisFile
15. Kompilasi ke NASM x86-64 Linux (integer + string)

// ── ROADMAP ────────────────────────────────
16. Self-hosting: kompiler ditulis dalam LOOP sendiri
17. Tipe data: kamus (dict)
18. Untuk/dalam: untuk x dalam larik { }
19. Modul / impor file .lp
20. Pesan error yang lebih informatif (baris + konteks)

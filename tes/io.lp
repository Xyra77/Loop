// tes/io.lp — tes semua fungsi builtin kategori IO
// Jalankan: ./loop tes/io.lp

cetak "▸ IO: cetak & variasi tulis"
cetak "cetak() baris biasa"
cetakTanpaBaris("cetakTanpaBaris() ")
cetakTanpaBaris("tanpa newline otomatis")
barisBaru()
tulis("tulis() sama seperti cetakTanpaBaris\n")
tulisInline("tulisInline() langsung flush\n")
tulisStderr("tulisStderr() ke stderr")
tulisError("tulisError() ke stderr (ada prefix [error])")

cetak ""
cetak "▸ IO: file (bacaFileTeks / tulisFileTeks / appendFile)"
milik path = "/tmp/tes_loop_io.txt"
tulisFileTeks(path, "baris pertama\n")
tambahFileTeks(path, "baris kedua (append lewat tambahFileTeks)\n")
appendFile(path, "baris ketiga (append lewat appendFile)\n")
milik isi = bacaFileTeks(path)
cetak "isi file setelah 3x tulis:"
cetak isi

cetak "▸ IO: tulisFile (keyword statement, tulis teks ke path literal)"
tulisFile("/tmp/tes_loop_io2.txt", "ditulis lewat tulisFile (keyword)\n")
cetak bacaFileTeks("/tmp/tes_loop_io2.txt")

cetak "▸ IO: bacaFile (keyword statement) — BUKAN pembaca teks, tapi MENJALANKAN file .lp lain!"
tulisFileTeks("/tmp/tes_loop_dijalankan.lp", "cetak \"halo dari file yang dijalankan lewat bacaFile()\"")
bacaFile("/tmp/tes_loop_dijalankan.lp")

cetak "▸ IO: waktu"
milik t1 = waktumilisdetik()
milik t2 = waktuSekarang()
milik t3 = waktuISO()
cetak "waktumilisdetik() > 0? " + keTeks(t1 > 0)
cetak "waktuSekarang() (unix epoch detik): " + keTeks(t2)
cetak "waktuISO() (format ISO 8601 UTC): " + t3

cetak "▸ IO: tidur (jeda 50ms, cuma cek gak error)"
tidur(50)
cetak "selesai tidur"

cetak "▸ IO: jalankan (keyword) & eksekusi() (fungsi)"
cetak "CATATAN: keyword 'jalankan' di parser ternyata cuma ALIAS dari 'cetak' (cuma nge-print,"
cetak "TIDAK menjalankan shell command) — beda dari nama fungsinya sendiri. Yang benar-benar"
cetak "menjalankan shell command adalah fungsi eksekusi() di bawah ini:"
jalankan "ini cuma diprint doang, bukan dieksekusi ke shell"
milik kode = eksekusi("echo dari eksekusi-fungsi > /tmp/tes_loop_eksekusi.txt")
cetak "eksekusi() exit code: " + keTeks(kode)
cetak bacaFileTeks("/tmp/tes_loop_eksekusi.txt")

cetak "▸ IO: masukan() — SKIP otomatis (butuh input interaktif)"
cetak "Coba manual: milik nama = masukan(\"Nama? \")"

cetak "▸ SEMUA TES IO SELESAI"

// tes/jaringan.lp — tes fungsi builtin kategori jaringan (TCP/HTTP) & prima/acak/ganda
// Jalankan: ./seed/loop tes/jaringan.lp (setelah server echo menyala)
//
// CATATAN PENTING:
// - Bagian "server lokal" di bawah PERLU server TCP nyala dulu di 127.0.0.1:9999
//   sebelum file ini dijalankan. Contoh server sederhana pakai Python:
//     python3 -c "
//     import socket
//     s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
//     s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
//     s.bind(('127.0.0.1', 9999)); s.listen(1)
//     conn, addr = s.accept()
//     conn.sendall(b'ECHO:' + conn.recv(4096)); conn.close(); s.close()
//     " &
//   Kalau server itu belum jalan, bagian sambung/kirim/terima/tutupKoneksi akan
//   GalatJaringan "gagal konek" — itu wajar, bukan bug loop.c.
// - Tes HTTP ke internet (ambil/telusuri) dipindah ke tes/manual/internet.lp karena
//   butuh koneksi internet dan tidak ikut CI. tes/jalankan_semua.sh menyalakan
//   server echo ini otomatis bila python3 tersedia.

cetak "▸ Jaringan: prima / acak / ganda (kunci & enkripsi dasar, TANPA jaringan)"
milik kunci = prima(1021)
cetak "prima(1021) -> tipe(): " + tipe(kunci)
milik terenkripsi = acak("data rahasia", kunci)
cetak "acak(\"data rahasia\", kunci) -> tipe(): " + tipe(terenkripsi)
milik g = ganda(42)
cetak "ganda(42) -> tipe(): " + tipe(g)

cetak ""
cetak "▸ Jaringan: TCP lewat socket lokal (sambung/kirim/terima/tutupKoneksi)"
cetak "(butuh server echo di 127.0.0.1:9999 — lihat komentar di atas file ini)"
milik kon = sambung("127.0.0.1", 9999)
cetak "sambung() -> tipe(): " + tipe(kon)
kirim(kon, "halo dari tes jaringan")
milik balasan = terima(kon)
cetak "terima(): " + balasan
tutupKoneksi(kon)
cetak "tutupKoneksi() selesai, tidak error"

cetak "▸ SEMUA TES JARINGAN SELESAI"

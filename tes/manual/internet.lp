// tes/manual/internet.lp — HTTP GET ke internet (manual, tidak ikut CI)
// Jalankan: ./seed/loop tes/manual/internet.lp

cetak ""
cetak "▸ Jaringan: HTTP GET (ambil / telusuri) — butuh internet"
milik isi = ambil("https://github.com")
cetak "ambil(\"https://github.com\") -> panjang body: " + keTeks(panjang(isi))
milik hasilTelusuri = telusuri("https://github.com")
cetak "telusuri(\"https://github.com\") -> tipe(): " + tipe(hasilTelusuri)
cetak "telusuri(\"https://github.com\") -> jumlah link ditemukan: " + keTeks(panjang(hasilTelusuri))
cetak "contoh 3 link pertama: " + keTeks(iris(hasilTelusuri, 0, 2))

cetak "▸ SELESAI TES INTERNET"

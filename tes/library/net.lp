// tes/library/net.lp — Library/net.lp: kegagalan jaringan = GalatJaringan yang bisa ditangkap
// Bagian "echo" butuh server TCP di 127.0.0.1:9999 (tes/jalankan_semua.sh menyalakannya).
// Jalankan dari akar repo: LOOP_STDLIB=Library ./seed/loop tes/library/net.lp
pakai pastikan
pakai net

cetak "▸ jalur gagal: tertangkap, program lanjut"
coba {
    bukaKoneksi("127.0.0.1", 1)
    gagalTes("seharusnya melempar GalatJaringan")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatJaringan", "port tertutup")
    pastikan(cariTeks(pesanGalat(e), "127.0.0.1:1") >= 0, "pesan menyebut host:port")
}
coba {
    bukaKoneksi("host-yang-pasti-tidak-ada.invalid", 80)
    gagalTes("seharusnya melempar GalatJaringan")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatJaringan", "host tidak ada")
}
coba {
    bukaKoneksi("127.0.0.1", 0)
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatNilai", "port 0 ditolak")
}
coba {
    bukaKoneksi("127.0.0.1", "80")
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatTipe", "port bukan angka")
}
coba {
    httpGet("ftp://contoh.test/x")
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatNilai", "skema selain http/https ditolak")
}
coba {
    httpGetTimeout("http://127.0.0.1:1/", 2)
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatJaringan", "HTTP ke port tertutup")
}
coba {
    bukaKoneksiTimeout("127.0.0.1", 1, 0)
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatNilai", "timeout 0 ditolak")
}
pastikan(benar, "program lanjut setelah galat jaringan")

cetak "▸ jalur sukses: server echo lokal"
milik k = bukaKoneksiTimeout("127.0.0.1", 9999, 5)
pastikanSama(tipe(k), "koneksi", "tipe koneksi")
pastikanSama(kirimTeks(k, "halo"), 4, "kirimTeks mengembalikan jumlah byte")
pastikanSama(terimaTeks(k, 100), "ECHO:halo", "balasan server")
pastikanSama(terimaTeks(k, 100), "", "lawan menutup koneksi -> teks kosong")
pastikan(tutup(k), "tutup")
pastikan(tutup(k), "tutup kedua kali aman")
coba {
    kirimTeks(k, "lagi")
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatJaringan", "kirim ke koneksi tertutup")
}

selesaiTes("NET")

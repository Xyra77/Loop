// tes/bita.lp — tipe bita dan teks tanpa batas diam-diam
// Jalankan dari akar repo: LOOP_STDLIB=Library ./seed/loop tes/bita.lp
pakai pastikan

cetak "▸ bita: pembuatan, konversi, indeks"
milik b = keBita([0, 1, 255, 0, 65])
pastikanSama(tipe(b), "bita", "tipe bita")
pastikanSama(panjangBita(b), 5, "panjang menghitung byte 0x00")
pastikanSama(panjang(b), 5, "panjang() juga bekerja untuk bita")
pastikanSama(keHex(b), "0001ff0041", "keHex")
pastikanSama(b[0], 0, "b[0] adalah angka 0")
pastikanSama(b[2], 255, "b[2] adalah angka 255")
pastikan(keBita([1, 2, 3]) == keBita([1, 2, 3]), "kesamaan bita (==)")
pastikan(keBita([1, 2, 3]) != keBita([1, 2, 4]), "ketidaksamaan bita")
pastikanSama(keHex(dariHex("00FF10")), "00ff10", "dariHex menerima huruf besar")
pastikanSama(keHex(sambungBita(keBita([1]), keBita([2, 3]))), "010203", "sambungBita")
pastikanSama(keHex(potongBita(b, 1, 4)), "01ff00", "potongBita")
pastikanSama(panjangBita(potongBita(b, 2, 2)), 0, "potongBita rentang kosong")

cetak "▸ bita: galat berjenis"
coba {
    milik x = b[5]
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatIndeks", "indeks bita di luar batas")
}
coba {
    potongBita(b, 3, 9)
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatIndeks", "potongBita melewati ujung")
}
coba {
    potongBita(b, 4, 2)
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatIndeks", "potongBita awal > akhir")
}
coba {
    milik x = b + b
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatTipe", "bita + bita ditolak (pakai sambungBita)")
}
coba {
    milik x = "a" + b
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatTipe", "teks + bita ditolak")
}
coba {
    dariHex("abc")
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatNilai", "hex panjang ganjil")
}
coba {
    dariHex("zz")
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatNilai", "karakter bukan hex")
}
coba {
    keBita([256])
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatNilai", "elemen di luar 0-255")
}
coba {
    keTeks(b)
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatNilai", "keTeks(bita berisi 0x00)")
}
coba {
    keTeks(dariHex("c328"))
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatNilai", "keTeks(bita bukan UTF-8 valid)")
}
pastikanSama(keTeks(keBita("héllo wörld")), "héllo wörld", "keBita/keTeks UTF-8 bolak-balik")

cetak "▸ base64 (vektor RFC 4648 bagian 10)"
pastikanSama(keBase64(keBita("")), "", "base64 kosong")
pastikanSama(keBase64(keBita("f")), "Zg==", "base64 f")
pastikanSama(keBase64(keBita("fo")), "Zm8=", "base64 fo")
pastikanSama(keBase64(keBita("foo")), "Zm9v", "base64 foo")
pastikanSama(keBase64(keBita("foob")), "Zm9vYg==", "base64 foob")
pastikanSama(keBase64(keBita("fooba")), "Zm9vYmE=", "base64 fooba")
pastikanSama(keBase64(keBita("foobar")), "Zm9vYmFy", "base64 foobar")
pastikanSama(keTeks(dariBase64("Zm9vYmFy")), "foobar", "dariBase64")
pastikan(dariBase64("AAEC/w==") == keBita([0, 1, 2, 255]), "dariBase64 dengan byte 0x00 dan 0xff")
coba {
    dariBase64("Zm9")
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatNilai", "base64 panjang bukan kelipatan 4")
}

cetak "▸ teks: + tidak lagi memotong di 4096 karakter"
milik sepuluh = "0123456789"
milik teks100 = ""
ulang 10 { teks100 = teks100 + sepuluh }
pastikanSama(panjang(teks100), 100, "100 karakter")
milik t = ""
ulang 50 { t = t + teks100 }
pastikanSama(panjang(t), 5000, "+ sampai 5000 karakter (dulu terpotong di 4096)")
pastikanSama(panjang(tambah(t, t)), 10000, "tambah() 10000 karakter")
t = t + t
t = t + t
t = t + t
pastikanSama(panjang(t), 40000, "+ sampai 40000 karakter")
pastikanSama(potong(t, 39990, 10), sepuluh, "ujung teks panjang utuh")
milik tt = "awal-"
tt += t
pastikanSama(panjang(tt), 40005, "+= pada teks panjang")
milik interp = "[#{t}]"
pastikanSama(panjang(interp), 40002, "interpolasi teks panjang")
milik gabungan = gabungTeks([t, t, t], "|")
pastikanSama(panjang(gabungan), 120002, "gabungTeks teks panjang")
milik berisiAngka = "n=" + 5000
pastikanSama(berisiAngka, "n=5000", "teks + angka")

cetak "▸ teks tidak bisa berisi byte 0x00"
coba {
    keTeks(keBita([65, 0, 66]))
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatNilai", "nol di tengah teks ditolak, bukan dipotong")
}

selesaiTes("BITA")

// tes/berkas.lp — berkas: biner, 0x00, ukuran batas, atomik, galat berjenis
// Jalankan dari akar repo: LOOP_STDLIB=Library ./seed/loop tes/berkas.lp
pakai pastikan

milik dir = "/tmp/loop_tes_berkas_" + _sistemPID()
jalankan("mkdir -p " + dir)

cetak "▸ round-trip biner (bacaBita/tulisBita)"
larik ukuran = [0, 1, 16, 4096, 4097, 65536]
angka i = 0
selama i < panjang(ukuran) {
    milik data = acakAman(ukuran[i])
    milik p = dir + "/b" + ukuran[i] + ".bin"
    tulisBita(p, data)
    pastikan(bacaBita(p) == data, "round-trip " + ukuran[i] + " byte")
    i = i + 1
}
milik besar = acakAman(10 * 1024 * 1024)
tulisBita(dir + "/besar.bin", besar)
pastikan(bacaBita(dir + "/besar.bin") == besar, "round-trip 10 MB")
milik denganNol = keBita([1, 0, 2, 0, 0, 3])
tulisBita(dir + "/nol.bin", denganNol)
pastikan(bacaBita(dir + "/nol.bin") == denganNol, "berkas berisi 0x00 utuh")
pastikanSama(panjangBita(bacaBita(dir + "/nol.bin")), 6, "6 byte, bukan terpotong di byte 0x00")

cetak "▸ berkas teks"
tulisFileTeks(dir + "/t.txt", "halo\ndunia")
pastikanSama(bacaFileTeks(dir + "/t.txt"), "halo\ndunia", "tulis/baca teks")
tulisFileTeks(dir + "/kosong.txt", "")
pastikanSama(bacaFileTeks(dir + "/kosong.txt"), "", "berkas kosong terbaca sebagai teks kosong")
pastikan(adaBerkas(dir + "/kosong.txt"), "adaBerkas: berkas kosong tetap ada")
pastikan(adaBerkas(dir + "/tidak-ada.txt") == salah, "adaBerkas: berkas hilang")
milik teksPanjang = ""
ulang 100 { teksPanjang = teksPanjang + "0123456789012345678901234567890123456789012345678901234567890123456789\n" }
tulisFileTeks(dir + "/panjang.txt", teksPanjang)
pastikanSama(panjang(bacaFileTeks(dir + "/panjang.txt")), 7100, "teks > 4096 karakter tidak terpotong")
appendFile(dir + "/t.txt", "!")
pastikanSama(bacaFileTeks(dir + "/t.txt"), "halo\ndunia!", "appendFile") 


cetak "▸ kegagalan berkas dilaporkan sebagai galat berjenis"
coba {
    bacaFileTeks(dir + "/tidak-ada.txt")
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatBerkas", "bacaFileTeks: berkas hilang beda dari berkas kosong")
}
coba {
    bacaBita(dir + "/tidak-ada.bin")
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatBerkas", "bacaBita: berkas hilang")
}
coba {
    bacaFileTeks(dir)
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatBerkas", "membaca direktori sebagai berkas")
}
coba {
    bacaFileTeks(dir + "/nol.bin")
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatNilai", "bacaFileTeks pada berkas berisi 0x00 (dulu terpotong diam-diam)")
}
coba {
    tulisFileTeks("/tidak/ada/dir/x.txt", "isi")
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatBerkas", "tulisFileTeks ke folder yang tidak ada (dulu melapor sukses)")
}
coba {
    tulisBita("/tidak/ada/dir/x.bin", keBita([1]))
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatBerkas", "tulisBita gagal")
}

cetak "▸ tulisAtomik"
tulisFileTeks(dir + "/atom.txt", "versi lama")
tulisAtomik(dir + "/atom.txt", "versi baru")
pastikanSama(bacaFileTeks(dir + "/atom.txt"), "versi baru", "tulisAtomik menimpa utuh")
tulisAtomik(dir + "/atom.bin", keBita([0, 0, 7]))
pastikan(bacaBita(dir + "/atom.bin") == keBita([0, 0, 7]), "tulisAtomik menerima bita berisi 0x00")
jalankan("mkdir " + dir + "/subdir")
coba {
    tulisAtomik(dir + "/subdir", "tidak boleh")
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatBerkas", "tujuan berupa direktori ditolak")
}
coba {
    tulisAtomik("/tidak/ada/dir/x", "y")
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatBerkas", "tulisAtomik ke folder yang tidak ada")
}
jalankan("ls -A " + dir + " | grep -c tmp > " + dir + "/hitung.txt")
pastikanSama(trimTeks(bacaFileTeks(dir + "/hitung.txt")), "0", "tidak ada berkas sementara tersisa")

cetak "▸ pindahBerkas / hapusBerkas"
pindahBerkas(dir + "/atom.txt", dir + "/pindah.txt")
pastikan(adaBerkas(dir + "/atom.txt") == salah, "asal hilang setelah dipindah")
pastikanSama(bacaFileTeks(dir + "/pindah.txt"), "versi baru", "isi tujuan utuh")
coba {
    pindahBerkas(dir + "/tidak-ada.txt", dir + "/x.txt")
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatBerkas", "pindahBerkas dari asal yang hilang")
}
hapusBerkas(dir + "/pindah.txt")
pastikan(adaBerkas(dir + "/pindah.txt") == salah, "hapusBerkas")
coba {
    hapusBerkas(dir + "/pindah.txt")
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatBerkas", "hapusBerkas yang sudah hilang")
}

jalankan("rm -rf " + dir)
selesaiTes("BERKAS")

// tes/library/io.lp — Library/io.lp: fileAda, ukuranFile, salinFile, bacaBaris
// Jalankan dari akar repo: LOOP_STDLIB=Library ./seed/loop tes/library/io.lp
pakai pastikan
pakai io

milik dir = "/tmp/loop_tes_io_" + _sistemPID()
jalankan("mkdir -p " + dir)

tulisFileTeks(dir + "/k.txt", "")
pastikan(fileAda(dir + "/k.txt"), "fileAda: berkas kosong tetap ada (dulu dianggap tidak ada)")
pastikan(fileAda(dir + "/tidak-ada") == salah, "fileAda: berkas hilang")
pastikanSama(ukuranFile(dir + "/k.txt"), 0, "ukuranFile berkas kosong")
tulisBita(dir + "/b.bin", keBita([1, 0, 2, 0, 3]))
pastikanSama(ukuranFile(dir + "/b.bin"), 5, "ukuranFile menghitung byte, termasuk 0x00")
salinFile(dir + "/b.bin", dir + "/salin.bin")
pastikan(bacaBita(dir + "/salin.bin") == keBita([1, 0, 2, 0, 3]), "salinFile aman untuk data biner")
coba {
    salinFile(dir + "/tidak-ada", dir + "/x")
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatBerkas", "salinFile dari berkas hilang")
}
tulisFileTeks(dir + "/baris.txt", "a\nb\nc")
pastikanSama(panjang(bacaBaris(dir + "/baris.txt")), 3, "bacaBaris")
pastikanSama(panjang(bacaBaris(dir + "/k.txt")), 0, "bacaBaris berkas kosong")
coba {
    bacaBaris(dir + "/tidak-ada")
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatBerkas", "bacaBaris berkas hilang")
}

jalankan("rm -rf " + dir)
selesaiTes("LIBRARY IO")

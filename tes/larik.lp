// tes/larik.lp — tes semua fungsi builtin kategori larik (array)
// Jalankan: ./loop tes/larik.lp

fungsi genap(x) {
    kembali x % 2 == 0
}

fungsi kali2(x) {
    kembali x * 2
}

fungsi tambahAcc(acc, x) {
    kembali acc + x
}

milik angka_arr = [5, 3, 8, 1, 9]

cetak "▸ Larik: dasar"
cetak "panjang([5,3,8,1,9]): " + keTeks(panjang(angka_arr))
cetak "jumlah([5,3,8,1,9]): " + keTeks(jumlah(angka_arr))
cetak "maks([5,3,8,1,9]): " + keTeks(maks(angka_arr))
cetak "min([5,3,8,1,9]): " + keTeks(min(angka_arr))
cetak "rerata([5,3,8,1,9]): " + keTeks(rerata(angka_arr))

cetak ""
cetak "▸ Larik: urut & balik"
cetak "urut([5,3,8,1,9]): " + keTeks(urut(angka_arr))
cetak "balik([5,3,8,1,9]): " + keTeks(balik(angka_arr))

cetak ""
cetak "▸ Larik: tambah / dorong / hapus"
cetak "tambah([5,3,8,1,9], 99) (append via tambah, larik asli tak berubah): " + keTeks(tambah(angka_arr, 99))
cetak "dorong([5,3,8,1,9], 99) (alias tambah utk larik): " + keTeks(dorong(angka_arr, 99))
cetak "hapusTerakhir([5,3,8,1,9]): " + keTeks(hapusTerakhir(angka_arr))
cetak "hapusKunci([5,3,8,1,9], 2) (hapus indeks ke-2): " + keTeks(hapusKunci(angka_arr, 2))
cetak "hapusLarik([5,3,8,1,9], 8) (hapus NILAI 8): " + keTeks(hapusLarik(angka_arr, 8))

cetak ""
cetak "▸ Larik: iris / irisLarik"
cetak "iris([5,3,8,1,9], 1, 3): " + keTeks(iris(angka_arr, 1, 3))
cetak "irisLarik([5,3,8,1,9], 1, 3) (sama, versi lama): " + keTeks(irisLarik(angka_arr, 1, 3))
cetak "iris(\"halo dunia\", 0, 3) (iris juga jalan utk teks): " + iris("halo dunia", 0, 3)

cetak ""
cetak "▸ Larik: pencarian"
cetak "adaDalam([5,3,8,1,9], 8): " + keTeks(adaDalam(angka_arr, 8))
cetak "adaDalam([5,3,8,1,9], 100): " + keTeks(adaDalam(angka_arr, 100))

cetak ""
cetak "▸ Larik: higher-order (saring / peta / kurangi)"
cetak "saring([5,3,8,1,9], genap) -> cuma yang genap: " + keTeks(saring(angka_arr, genap))
cetak "peta([5,3,8,1,9], kali2) -> dikali 2: " + keTeks(peta(angka_arr, kali2))
cetak "kurangi([5,3,8,1,9], tambahAcc, 0) -> reduce jadi total: " + keTeks(kurangi(angka_arr, tambahAcc, 0))

cetak ""
cetak "▸ Larik: tipe & metadata"
cetak "tipe([5,3,8,1,9]): " + tipe(angka_arr)
cetak "jenisNilai([5,3,8,1,9]) (alias tipe): " + jenisNilai(angka_arr)
cetak "kunciObjek([5,3,8,1,9]) -> larik indeks: " + keTeks(kunciObjek(angka_arr))

cetak ""
cetak "▸ Larik: salin (deep copy) & gabung"
milik asli = [1, 2, 3]
milik salinan = salin(asli)
cetak "salin([1,2,3]): " + keTeks(salinan)
cetak "gabung(1, \"dua\", benar) -> larik campuran serbaguna: " + keTeks(gabung(1, "dua", benar))

cetak ""
cetak "▸ Larik: akses indeks langsung"
cetak "angka_arr[0]: " + keTeks(angka_arr[0])
cetak "angka_arr[4]: " + keTeks(angka_arr[4])

cetak "▸ SEMUA TES LARIK SELESAI"

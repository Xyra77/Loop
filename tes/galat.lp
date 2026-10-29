// tes/galat.lp — model galat: lempar, coba/tangkap/akhirnya, galat berjenis dari builtin
// Jalankan dari akar repo: LOOP_STDLIB=Library ./seed/loop tes/galat.lp
pakai pastikan

fungsi bagi(a, b) {
    jika b == 0 maka
        lempar galat("GalatNilai", "pembagi tidak boleh nol")
    akhir
    kembali a / b
}

cetak "▸ lempar / tangkap dasar"
coba {
    bagi(1, 0)
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatNilai", "jenis galat")
    pastikanSama(pesanGalat(e), "pembagi tidak boleh nol", "pesan galat")
    pastikan(cariTeks(lokasiGalat(e), "galat.lp:") >= 0, "lokasi memuat nama berkas")
    pastikanSama(tipe(e), "galat", "tipe(e) adalah galat")
    pastikanSama("" + e, "GalatNilai: pembagi tidak boleh nol", "\"\" + e menghasilkan teks Jenis: pesan (jembatan kode lama)")
}
pastikanSama(bagi(6, 3), 2, "jalur sukses tetap jalan")

cetak "▸ akhirnya selalu jalan"
angka urut = 0
coba {
    urut = urut + 1
} akhirnya {
    urut = urut * 10
}
pastikanSama(urut, 10, "akhirnya jalan saat sukses")

urut = 0
coba {
    lempar galat("GalatSistem", "x")
} tangkap {
    urut = urut + 1
} akhirnya {
    urut = urut * 10
}
pastikanSama(urut, 10, "akhirnya jalan setelah tangkap")

fungsi denganKembali() {
    coba {
        kembali 1
    } akhirnya {
        urut = 99
    }
    kembali 2
}
pastikanSama(denganKembali(), 1, "kembali di coba tetap mengembalikan nilainya")
pastikanSama(urut, 99, "akhirnya jalan walau ada kembali")

urut = 0
coba {
    coba {
        lempar galat("GalatBatas", "dalam")
    } akhirnya {
        urut = 5
    }
    gagalTes("galat harus naik lewat coba tanpa tangkap")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatBatas", "coba tanpa tangkap meneruskan galat")
}
pastikanSama(urut, 5, "akhirnya jalan walau tanpa tangkap")

urut = 0
coba {
    coba {
        lempar galat("GalatNilai", "pertama")
    } tangkap {
        lempar galat("GalatSistem", "dari tangkap")
    } akhirnya {
        urut = 7
    }
} tangkap {
    pastikanSama(jenisGalat(e), "GalatSistem", "galat dari blok tangkap tetap naik")
}
pastikanSama(urut, 7, "akhirnya jalan walau tangkap melempar")

cetak "▸ lempar ulang dan sebab"
coba {
    coba {
        lempar galat("GalatKunci", "kunci salah")
    } tangkap {
        jika jenisGalat(e) == "GalatKunci" maka
            lempar e
        akhir
    }
} tangkap {
    pastikanSama(jenisGalat(e), "GalatKunci", "lempar e meneruskan jenis asli")
    pastikanSama(pesanGalat(e), "kunci salah", "lempar e meneruskan pesan asli")
}

coba {
    coba {
        bacaFileTeks("/tidak/ada/sama-sekali.txt")
    } tangkap {
        lempar galat("GalatSistem", "konfigurasi gagal dimuat", e)
    }
} tangkap {
    pastikanSama(jenisGalat(e), "GalatSistem", "galat pembungkus")
    pastikanSama(jenisGalat(sebabGalat(e)), "GalatBerkas", "sebab = galat asli")
}

coba {
    lempar "bukan objek galat"
    gagalTes("seharusnya melempar GalatTipe")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatTipe", "lempar non-galat ditolak")
}

cetak "▸ builtin yang gagal bisa ditangkap (dulu exit(1))"
coba {
    bacaFileTeks("/tidak/ada/sama-sekali.txt")
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatBerkas", "bacaFileTeks berkas hilang")
}
coba {
    milik x = 1 / 0
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatNilai", "pembagian dengan nol")
}
coba {
    milik x = 5 % 0
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatNilai", "modulo dengan nol")
}
coba {
    milik x = 8
    x /= 0
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatNilai", "/= dengan nol (dulu diam-diam 0)")
}
coba {
    keAngka([1, 2])
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatTipe", "keAngka(larik)")
}
coba {
    milik arr = [1, 2, 3]
    milik x = arr[10]
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatIndeks", "indeks larik di luar batas")
}
coba {
    sambung("127.0.0.1", 1)
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatJaringan", "port tertutup")
}
coba {
    sambung("127.0.0.1", 99999)
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatNilai", "port di luar jangkauan")
}
coba {
    tipe()
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatTipe", "jumlah argumen builtin salah")
}

fungsi rekursiTakBerhingga(n) {
    kembali rekursiTakBerhingga(n + 1)
}
coba {
    rekursiTakBerhingga(0)
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatBatas", "rekursi tanpa batas jadi GalatBatas, bukan segfault")
}

cetak "▸ galat di dalam fungsi dan loop"
fungsi cobaDiLoop() {
    angka tertangkap = 0
    angka i = 0
    selama i < 5 {
        coba {
            jika i % 2 == 0 maka
                lempar galat("GalatNilai", "genap")
            akhir
        } tangkap {
            tertangkap = tertangkap + 1
        }
        i = i + 1
    }
    kembali tertangkap
}
pastikanSama(cobaDiLoop(), 3, "coba di dalam loop, galat tertangkap tiap iterasi")

fungsi loopDenganAkhirnya() {
    angka n = 0
    angka i = 0
    selama i < 10 {
        i = i + 1
        coba {
            jika i == 3 maka
                hentikan
            akhir
        } akhirnya {
            n = n + 1
        }
    }
    kembali n
}
pastikanSama(loopDenganAkhirnya(), 3, "hentikan di coba tetap menjalankan akhirnya")

cetak "▸ jalankanAman (bridge) memberi objek galat"
pakai bridge
fungsi gagal() {
    lempar galat("GalatSistem", "uji")
}
milik hasil = jalankanAman(gagal)
pastikanSama(hasil[0], salah, "jalankanAman melaporkan gagal")
pastikanSama(jenisGalat(hasil[1]), "GalatSistem", "jalankanAman mengembalikan objek galat")

selesaiTes("GALAT")

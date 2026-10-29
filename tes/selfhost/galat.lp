// Dikompilasi native DAN dijalankan interpreter; keluaran harus identik.
// Hanya memakai fitur yang ada di kedua sisi (tipe(e)/lokasiGalat beda, sengaja tidak dipakai).

fungsi bagi(a, b) {
    jika b == 0 maka
        lempar galat("GalatNilai", "pembagi tidak boleh nol")
    akhir
    kembali a / b
}

fungsi dasarFungsi() {
    lempar galat("GalatSistem", "dari dalam")
}
fungsi tengah() {
    dasarFungsi()
    cetak "tidak sampai sini"
}

// 1. galat melintasi frame fungsi
coba {
    tengah()
} tangkap {
    cetak "1: #{jenisGalat(e)} #{pesanGalat(e)}"
}

// 2. akhirnya: normal, galat, kembali
coba {
    cetak "2a: badan"
} akhirnya {
    cetak "2a: akhirnya"
}
coba {
    lempar galat("GalatBatas", "x")
} tangkap {
    cetak "2b: tangkap"
} akhirnya {
    cetak "2b: akhirnya"
}
fungsi denganKembali() {
    coba {
        kembali 1
    } akhirnya {
        cetak "2c: akhirnya jalan walau kembali"
    }
    kembali 2
}
cetak "2c: hasil #{denganKembali()}"

// 3. coba tanpa tangkap: akhirnya jalan lalu galat naik
coba {
    coba {
        lempar galat("GalatBatas", "dalam")
    } akhirnya {
        cetak "3: akhirnya dalam"
    }
    cetak "3: TIDAK BOLEH"
} tangkap {
    cetak "3: naik #{jenisGalat(e)}"
}

// 4. galat di blok tangkap + akhirnya
coba {
    coba {
        lempar galat("GalatNilai", "pertama")
    } tangkap {
        lempar galat("GalatSistem", "dari tangkap")
    } akhirnya {
        cetak "4: akhirnya"
    }
} tangkap {
    cetak "4: luar #{jenisGalat(e)}"
}

// 5. lempar ulang dan sebab
coba {
    coba {
        lempar galat("GalatKunci", "kunci salah")
    } tangkap {
        jika jenisGalat(e) == "GalatKunci" maka
            lempar e
        akhir
    }
} tangkap {
    cetak "5: #{jenisGalat(e)} #{pesanGalat(e)}"
}
coba {
    coba {
        bagi(1, 0)
    } tangkap {
        lempar galat("GalatSistem", "dibungkus", e)
    }
} tangkap {
    cetak "5b: #{jenisGalat(e)} sebab=#{jenisGalat(sebabGalat(e))}"
}

// 6. hentikan/lewati menembus coba + akhirnya
angka i = 0
angka hitung = 0
selama i < 6 {
    i += 1
    coba {
        jika i == 2 maka
            lewati
        akhir
        jika i == 5 maka
            hentikan
        akhir
        hitung += 1
    } akhirnya {
        hitung += 10
    }
}
cetak "6: i=#{i} hitung=#{hitung}"

// 7. pembagian dengan nol bisa ditangkap
coba {
    angka x = 10
    angka y = 0
    cetak x / y
} tangkap {
    cetak "7: #{jenisGalat(e)}"
}
coba {
    angka x = 10
    angka y = 0
    cetak x % y
} tangkap {
    cetak "7b: #{jenisGalat(e)}"
}

// 8. berkas tidak ada
coba {
    cetak bacaFileTeks("/tidak/ada/sama-sekali")
} tangkap {
    cetak "8: #{jenisGalat(e)}"
}

// 9. handler seimbang: 100000 iterasi dengan dan tanpa galat, kembali di dalam coba
fungsi cobaKembali(n) {
    coba {
        jika n % 2 == 0 maka
            lempar galat("GalatNilai", "genap")
        akhir
        kembali n
    } tangkap {
        kembali 0 - n
    }
}
angka total = 0
angka k = 0
selama k < 100000 {
    total += cobaKembali(k)
    k += 1
}
cetak "9: total=#{total}"

// 10. objek bukan galat tidak boleh dilempar
coba {
    lempar "bukan galat"
} tangkap {
    cetak "10: #{jenisGalat(e)}"
}


// 11. indeks larik di luar batas (dulu membaca/menulis memori sembarang di native)
larik arr = [10, 20, 30]
coba {
    cetak arr[3]
} tangkap {
    cetak "11a: #{jenisGalat(e)}"
}
coba {
    angka neg = 0 - 1
    cetak arr[neg]
} tangkap {
    cetak "11b: #{jenisGalat(e)}"
}
cetak "11c: #{arr[2]} #{panjang(arr)}"
cetak "selesai"

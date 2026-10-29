// Library/dasar.lp
// Fungsi dasar yang BELUM ada sebagai builtin interpreter.
// Duplikat builtin (potong, pisah, ganti, maks, jumlah, ... ) sudah dibuang:
// builtin selalu didahulukan oleh interpreter, jadi versi .lp-nya tidak pernah
// terpakai. Placeholder yang berpura-pura bekerja (lempar, formatWaktu,
// tungguSemua) juga dibuang: lempar sekarang kata kunci bahasa.

fungsi larikSet(arr, indeks, nilai) {
    // Loop gak punya index assignment (arr[i] = x), jadi kita rebuild array baru
    // dengan elemen di 'indeks' diganti 'nilai'.
    larik hasil = []
    angka n = panjang(arr)
    angka i = 0
    selama i < n {
        jika i == indeks maka
            hasil = tambah(hasil, nilai)
        selain
            hasil = tambah(hasil, arr[i])
        akhir
        i += 1
    }
    kembali hasil
}

fungsi kamusJumlah(d) {
    kembali panjang(d)
}

fungsi _potongRentang(s, mulai, sampai) {
    teks hasil = ""
    angka i = mulai
    selama i < sampai {
        hasil = tambah(hasil, karakter(s, i))
        i += 1
    }
    kembali hasil
}

fungsi adalah_spasi_kosong(c) {
    angka k = kodeKarakter(c)
    kembali k == 32 atau k == 9 atau k == 10 atau k == 13
}

fungsi gantiSemua(s, cari, ganti_dengan) {
    // sama kayak ganti() karena udah ganti semua
    kembali ganti(s, cari, ganti_dengan)
}

fungsi hapusAkhir(arr) {
    angka n = panjang(arr)
    jika n == 0 maka
        kembali arr
    akhir
    larik hasil = []
    angka i = 0
    selama i < n - 1 {
        hasil = tambah(hasil, arr[i])
        i += 1
    }
    kembali hasil
}

fungsi adalahPrima(n) {
    jika n < 2 maka
        kembali salah
    akhir
    angka i = 2
    selama i * i <= n {
        jika n % i == 0 maka
            kembali salah
        akhir
        i += 1
    }
    kembali benar
}

fungsi cari(arr, nilai) {
    angka i = 0
    angka n = panjang(arr)
    selama i < n {
        jika arr[i] == nilai maka
            kembali i
        akhir
        i += 1
    }
    kembali -1
}

fungsi isi(arr, indeks, nilai) {
    angka n = panjang(arr)
    jika indeks < 0 maka
        indeks = n + indeks
    akhir
    jika indeks < 0 atau indeks >= n maka
        kembali arr
    akhir
    arr = larikSet(arr, indeks, nilai)
    kembali arr
}

fungsi rataRata(arr) {
    kembali rerata(arr)
}

fungsi floor(n) {
    // Pembulatan ke bawah
    jika n >= 0 maka
        angka i = 0
        selama i <= n {
            jika i + 1 > n maka
                kembali i
            akhir
            i += 1
        }
    selain
        angka i = 0
        selama i >= n {
            jika i - 1 < n maka
                kembali i - 1
            akhir
            i -= 1
        }
    akhir
    kembali 0
}

fungsi ceil(n) {
    // Pembulatan ke atas
    jika n <= 0 maka
        angka i = 0
        selama i >= n {
            jika i - 1 < n maka
                kembali i
            akhir
            i -= 1
        }
    selain
        angka i = 0
        selama i <= n {
            jika i + 1 > n maka
                kembali i + 1
            akhir
            i += 1
        }
    akhir
    kembali 0
}


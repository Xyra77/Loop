fungsi kamusBaru() {
    kembali []
}

fungsi kamusSet(d, key, val) {
    angka i = 0
    angka n = panjang(d)
    selama i < n {
        jika d[i][0] == key maka
            d[i] = [key, val]
            kembali d
        akhir
        i += 1
    }
    kembali tambah(d, [key, val])
}

fungsi kamusAmbil(d, key) {
    angka i = 0
    angka n = panjang(d)
    selama i < n {
        jika d[i][0] == key maka
            kembali d[i][1]
        akhir
        i += 1
    }
    kembali ""
}

fungsi kamusAda(d, key) {
    angka i = 0
    angka n = panjang(d)
    selama i < n {
        jika d[i][0] == key maka
            kembali benar
        akhir
        i += 1
    }
    kembali salah
}

fungsi kamusHapus(d, key) {
    larik hasil = []
    angka i = 0
    angka n = panjang(d)
    selama i < n {
        jika d[i][0] != key maka
            hasil = tambah(hasil, d[i])
        akhir
        i += 1
    }
    kembali hasil
}

fungsi kamusKunci(d) {
    larik hasil = []
    angka i = 0
    angka n = panjang(d)
    selama i < n {
        hasil = tambah(hasil, d[i][0])
        i += 1
    }
    kembali hasil
}

fungsi kamusNilai(d) {
    larik hasil = []
    angka i = 0
    angka n = panjang(d)
    selama i < n {
        hasil = tambah(hasil, d[i][1])
        i += 1
    }
    kembali hasil
}

fungsi kamusJumlah(d) {
    kembali panjang(d)
}

fungsi kamusKosong(d) {
    kembali panjang(d) == 0
}

fungsi kamusGabung(d1, d2) {
    larik hasil = d1
    angka i = 0
    angka n = panjang(d2)
    selama i < n {
        hasil = kamusSet(hasil, d2[i][0], d2[i][1])
        i += 1
    }
    kembali hasil
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

fungsi potong(s, awal, banyak) {
    angka len = panjang(s)
    angka mulai = awal
    jika mulai < 0 maka
        mulai = len + mulai
    akhir
    jika mulai < 0 maka
        mulai = 0
    akhir
    angka ambil = banyak
    jika mulai >= len atau ambil <= 0 maka
        kembali ""
    akhir
    jika mulai + ambil > len maka
        ambil = len - mulai
    akhir
    kembali _potongRentang(s, mulai, mulai + ambil)
}

fungsi pisah(s, pemisah) {
    larik hasil = []
    teks buf = ""
    angka i = 0
    angka n = panjang(s)
    angka plen = panjang(pemisah)
    selama i < n {
        jika plen > 0 dan i + plen <= n dan _potongRentang(s, i, i + plen) == pemisah maka
            hasil = tambah(hasil, buf)
            buf = ""
            i = i + plen
        selain
            buf = tambah(buf, karakter(s, i))
            i += 1
        akhir
    }
    hasil = tambah(hasil, buf)
    kembali hasil
}

fungsi gabung(arr, penghubung) {
    teks hasil = ""
    angka i = 0
    angka n = panjang(arr)
    selama i < n {
        hasil = tambah(hasil, arr[i])
        jika i < n - 1 maka
            hasil = tambah(hasil, penghubung)
        akhir
        i += 1
    }
    kembali hasil
}

fungsi adalah_spasi_kosong(c) {
    angka k = kodeKarakter(c)
    kembali k == 32 atau k == 9 atau k == 10 atau k == 13
}

fungsi trim(s) {
    angka n = panjang(s)
    angka awal = 0
    angka sampai = n
    selama awal < sampai dan adalah_spasi_kosong(karakter(s, awal)) {
        awal += 1
    }
    selama sampai > awal dan adalah_spasi_kosong(karakter(s, sampai - 1)) {
        sampai -= 1
    }
    kembali _potongRentang(s, awal, sampai)
}

fungsi trimKiri(s) {
    angka n = panjang(s)
    angka awal = 0
    selama awal < n dan adalah_spasi_kosong(karakter(s, awal)) {
        awal += 1
    }
    kembali _potongRentang(s, awal, n)
}

fungsi trimKanan(s) {
    angka n = panjang(s)
    angka sampai = n
    selama sampai > 0 dan adalah_spasi_kosong(karakter(s, sampai - 1)) {
        sampai -= 1
    }
    kembali _potongRentang(s, 0, sampai)
}

fungsi ganti(s, cari, ganti_dengan) {
    teks hasil = ""
    angka i = 0
    angka n = panjang(s)
    angka clen = panjang(cari)
    selama i < n {
        jika clen > 0 dan i + clen <= n dan _potongRentang(s, i, i + clen) == cari maka
            hasil = tambah(hasil, ganti_dengan)
            i = i + clen
        selain
            hasil = tambah(hasil, karakter(s, i))
            i += 1
        akhir
    }
    kembali hasil
}

fungsi gantiSemua(s, cari, ganti_dengan) {
    // sama kayak ganti() karena udah ganti semua
    kembali ganti(s, cari, ganti_dengan)
}

fungsi hurufBesar(s) {
    teks hasil = ""
    angka i = 0
    angka n = panjang(s)
    selama i < n {
        angka k = kodeKarakter(karakter(s, i))
        jika k >= 97 dan k <= 122 maka
            hasil = tambah(hasil, dariKode(k - 32))
        selain
            hasil = tambah(hasil, karakter(s, i))
        akhir
        i += 1
    }
    kembali hasil
}

fungsi hurufKecil(s) {
    teks hasil = ""
    angka i = 0
    angka n = panjang(s)
    selama i < n {
        angka k = kodeKarakter(karakter(s, i))
        jika k >= 65 dan k <= 90 maka
            hasil = tambah(hasil, dariKode(k + 32))
        selain
            hasil = tambah(hasil, karakter(s, i))
        akhir
        i += 1
    }
    kembali hasil
}

fungsi hurufKapital(s) {
    teks hasil = ""
    angka i = 0
    angka n = panjang(s)
    bool kapitalBerikutnya = benar
    selama i < n {
        teks c = karakter(s, i)
        jika kapitalBerikutnya maka
            hasil = tambah(hasil, hurufBesar(c))
            kapitalBerikutnya = salah
        selain
            hasil = tambah(hasil, hurufKecil(c))
        akhir
        jika c == " " atau c == "." atau c == "!" atau c == "?" maka
            kapitalBerikutnya = benar
        akhir
        i += 1
    }
    kembali hasil
}

fungsi balik(arr) {
    larik hasil = []
    angka i = panjang(arr) - 1
    selama i >= 0 {
        hasil = tambah(hasil, arr[i])
        i -= 1
    }
    kembali hasil
}

fungsi balikTeks(s) {
    teks hasil = ""
    angka i = panjang(s) - 1
    selama i >= 0 {
        hasil = tambah(hasil, karakter(s, i))
        i -= 1
    }
    kembali hasil
}

fungsi dorong(arr, nilai) {
    kembali tambah(arr, nilai)
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

fungsi hapusIndeks(arr, indeks) {
    angka n = panjang(arr)
    jika indeks < 0 atau indeks >= n maka
        kembali arr
    akhir
    larik hasil = []
    angka i = 0
    selama i < n {
        jika i != indeks maka
            hasil = tambah(hasil, arr[i])
        akhir
        i += 1
    }
    kembali hasil
}

fungsi sisip(arr, indeks, nilai) {
    angka n = panjang(arr)
    jika indeks < 0 maka
        indeks = 0
    akhir
    jika indeks > n maka
        indeks = n
    akhir
    larik hasil = []
    angka i = 0
    selama i < indeks {
        hasil = tambah(hasil, arr[i])
        i += 1
    }
    hasil = tambah(hasil, nilai)
    selama i < n {
        hasil = tambah(hasil, arr[i])
        i += 1
    }
    kembali hasil
}

fungsi maks(arr) {
    angka n = panjang(arr)
    jika n == 0 maka
        kembali 0
    akhir
    angka m = arr[0]
    angka i = 1
    selama i < n {
        jika arr[i] > m maka
            m = arr[i]
        akhir
        i += 1
    }
    kembali m
}

fungsi min(arr) {
    angka n = panjang(arr)
    jika n == 0 maka
        kembali 0
    akhir
    angka m = arr[0]
    angka i = 1
    selama i < n {
        jika arr[i] < m maka
            m = arr[i]
        akhir
        i += 1
    }
    kembali m
}

fungsi jumlah(arr) {
    angka total = 0
    angka i = 0
    angka n = panjang(arr)
    selama i < n {
        total += arr[i]
        i += 1
    }
    kembali total
}

fungsi rerata(arr) {
    angka n = panjang(arr)
    jika n == 0 maka
        kembali 0
    akhir
    kembali jumlah(arr) / n
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

fungsi peta(arr, f) {
    larik hasil = []
    angka i = 0
    angka n = panjang(arr)
    selama i < n {
        hasil = tambah(hasil, f(arr[i]))
        i += 1
    }
    kembali hasil
}

fungsi saring(arr, f) {
    larik hasil = []
    angka i = 0
    angka n = panjang(arr)
    selama i < n {
        jika f(arr[i]) maka
            hasil = tambah(hasil, arr[i])
        akhir
        i += 1
    }
    kembali hasil
}

fungsi kurangi(arr, f, awal) {
    milik akumulator = awal
    angka i = 0
    angka n = panjang(arr)
    selama i < n {
        akumulator = f(akumulator, arr[i])
        i += 1
    }
    kembali akumulator
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

fungsi cariTerakhir(arr, nilai) {
    angka i = panjang(arr) - 1
    selama i >= 0 {
        jika arr[i] == nilai maka
            kembali i
        akhir
        i -= 1
    }
    kembali -1
}

fungsi ada(arr, nilai) {
    kembali cari(arr, nilai) != -1
}

fungsi ambil(arr, indeks) {
    angka n = panjang(arr)
    jika indeks < 0 maka
        indeks = n + indeks
    akhir
    jika indeks < 0 atau indeks >= n maka
        kembali null
    akhir
    kembali arr[indeks]
}

fungsi isi(arr, indeks, nilai) {
    angka n = panjang(arr)
    jika indeks < 0 maka
        indeks = n + indeks
    akhir
    jika indeks < 0 atau indeks >= n maka
        kembali arr
    akhir
    arr[indeks] = nilai
    kembali arr
}

fungsi potongArray(arr, mulai, banyak) {
    angka n = panjang(arr)
    angka m = mulai
    jika m < 0 maka
        m = n + m
    akhir
    jika m < 0 maka
        m = 0
    akhir
    angka ambil = banyak
    jika m >= n atau ambil <= 0 maka
        kembali []
    akhir
    jika m + ambil > n maka
        ambil = n - m
    akhir
    larik hasil = []
    angka i = m
    selama i < m + ambil {
        hasil = tambah(hasil, arr[i])
        i += 1
    }
    kembali hasil
}

fungsi salin(arr) {
    larik hasil = []
    angka i = 0
    angka n = panjang(arr)
    selama i < n {
        hasil = tambah(hasil, arr[i])
        i += 1
    }
    kembali hasil
}

fungsi rataRata(arr) {
    kembali rerata(arr)
}

fungsi median(arr) {
    angka n = panjang(arr)
    jika n == 0 maka
        kembali 0
    akhir
    larik sorted = salin(arr)
    // bubble sort sederhana
    angka i = 0
    selama i < n - 1 {
        angka j = 0
        selama j < n - i - 1 {
            jika sorted[j] > sorted[j + 1] maka
                angka temp = sorted[j]
                sorted[j] = sorted[j + 1]
                sorted[j + 1] = temp
            akhir
            j += 1
        }
        i += 1
    }
    jika n % 2 == 1 maka
        kembali sorted[(n - 1) / 2]
    selain
        angka tengah1 = sorted[n / 2 - 1]
        angka tengah2 = sorted[n / 2]
        kembali (tengah1 + tengah2) / 2
    akhir
}

fungsi modus(arr) {
    angka n = panjang(arr)
    jika n == 0 maka
        kembali 0
    akhir
    larik frekuensi = []
    larik unik = []
    angka i = 0
    selama i < n {
        angka idx = cari(unik, arr[i])
        jika idx == -1 maka
            unik = tambah(unik, arr[i])
            frekuensi = tambah(frekuensi, 1)
        selain
            frekuensi[idx] = frekuensi[idx] + 1
        akhir
        i += 1
    }
    angka maksFreq = maks(frekuensi)
    angka idxMax = cari(frekuensi, maksFreq)
    kembali unik[idxMax]
}

fungsi urut(arr) {
    larik hasil = salin(arr)
    angka n = panjang(hasil)
    angka i = 0
    selama i < n - 1 {
        angka j = 0
        selama j < n - i - 1 {
            jika hasil[j] > hasil[j + 1] maka
                angka temp = hasil[j]
                hasil[j] = hasil[j + 1]
                hasil[j + 1] = temp
            akhir
            j += 1
        }
        i += 1
    }
    kembali hasil
}

fungsi urutTurun(arr) {
    larik hasil = urut(arr)
    kembali balik(hasil)
}

fungsi unik(arr) {
    larik hasil = []
    angka i = 0
    angka n = panjang(arr)
    selama i < n {
        jika cari(hasil, arr[i]) == -1 maka
            hasil = tambah(hasil, arr[i])
        akhir
        i += 1
    }
    kembali hasil
}

fungsi acakAngka(rendah, tinggi) {
    // Asumsi ada builtin acak() yang ngasih 0-1
    angka range = tinggi - rendah + 1
    angka hasil = rendah + (acak() * range)
    kembali bulatkan(hasil)
}

fungsi bulatkan(n) {
    // Pembulatan ke integer terdekat
    angka frac = n - floor(n)
    jika frac >= 0.5 maka
        kembali floor(n) + 1
    akhir
    kembali floor(n)
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

fungsi absolut(n) {
    jika n < 0 maka
        kembali -n
    akhir
    kembali n
}

fungsi pangkat(alas, eksponen) {
    angka hasil = 1
    angka i = 0
    selama i < eksponen {
        hasil = hasil * alas
        i += 1
    }
    kembali hasil
}

fungsi akarKuadrat(n) {
    // Metode Newton (sederhana)
    jika n < 0 maka
        kembali 0
    akhir
    jika n == 0 maka
        kembali 0
    akhir
    angka x = n
    angka y = 1
    angka epsilon = 0.000001
    selama x - y > epsilon {
        x = (x + y) / 2
        y = n / x
    }
    kembali x
}

fungsi faktorial(n) {
    jika n < 0 maka
        kembali 1
    akhir
    angka hasil = 1
    angka i = 2
    selama i <= n {
        hasil = hasil * i
        i += 1
    }
    kembali hasil
}

fungsi fibonacci(n) {
    jika n <= 0 maka
        kembali 0
    akhir
    jika n == 1 maka
        kembali 1
    akhir
    angka a = 0
    angka b = 1
    angka i = 2
    selama i <= n {
        angka c = a + b
        a = b
        b = c
        i += 1
    }
    kembali b
}

fungsi isAngka(n) {
    kembali tipe(n) == "angka"
}

fungsi isTeks(s) {
    kembali tipe(s) == "teks"
}

fungsi isLarik(arr) {
    kembali tipe(arr) == "larik"
}

fungsi isKamus(d) {
    // Kamus diimplementasikan sebagai larik berisi pasangan [key, val]
    jika tipe(d) != "larik" maka
        kembali salah
    akhir
    angka i = 0
    angka n = panjang(d)
    selama i < n {
        jika tipe(d[i]) != "larik" atau panjang(d[i]) != 2 maka
            kembali salah
        akhir
        i += 1
    }
    kembali benar
}

fungsi isBoolean(b) {
    kembali tipe(b) == "boolean"
}

fungsi isNull(n) {
    kembali tipe(n) == "null"
}

fungsi tipeData(x) {
    kembali tipe(x)
}

fungsi keAngka(s) {
    // Ubah teks ke angka (sederhana)
    angka hasil = 0
    angka i = 0
    angka n = panjang(s)
    bool negatif = salah
    jika n > 0 dan karakter(s, 0) == "-" maka
        negatif = benar
        i = 1
    akhir
    selama i < n {
        teks c = karakter(s, i)
        jika c >= "0" dan c <= "9" maka
            angka digit = kodeKarakter(c) - 48
            hasil = hasil * 10 + digit
        selain
            kembali 0
        akhir
        i += 1
    }
    jika negatif maka
        kembali -hasil
    akhir
    kembali hasil
}

fungsi keTeks(n) {
    // Ubah angka ke teks
    jika n == 0 maka
        kembali "0"
    akhir
    bool negatif = salah
    jika n < 0 maka
        negatif = benar
        n = -n
    akhir
    teks hasil = ""
    selama n > 0 {
        angka sisa = n % 10
        teks digit = dariKode(48 + sisa)
        hasil = digit + hasil
        n = (n - sisa) / 10
    }
    jika negatif maka
        hasil = "-" + hasil
    akhir
    kembali hasil
}

fungsi keBoolean(n) {
    jika n == 0 atau n == "" atau n == [] maka
        kembali salah
    akhir
    kembali benar
}

// ---- galat berjenis (runtime native untuk coba/tangkap/akhirnya/lempar) ----
// Objek galat = larik ["__GALAT__", jenis, pesan, sebab]. Di binary native
// tipe(e) menghasilkan "larik" (interpreter: "galat"). `galat(jenis, pesan)`
// dengan 2 argumen mengisi sebab = null (diatur di compiler).
fungsi galat(jenis, pesan, sebab) {
    kembali ["__GALAT__", jenis, pesan, sebab]
}

fungsi jenisGalat(g) {
    kembali g[1]
}

fungsi pesanGalat(g) {
    kembali g[2]
}

fungsi sebabGalat(g) {
    kembali g[3]
}

// lokasi/jejak belum dilacak di binary native
fungsi lokasiGalat(g) {
    kembali ""
}

// dipanggil oleh `lempar`: memastikan nilai adalah objek galat
fungsi __lemparCek(x) {
    jika tipe(x) != "larik" maka
        kembali galat("GalatTipe", "lempar butuh objek galat, buat dengan galat(jenis, pesan)", null)
    akhir
    jika panjang(x) != 4 maka
        kembali galat("GalatTipe", "lempar butuh objek galat, buat dengan galat(jenis, pesan)", null)
    akhir
    jika x[0] != "__GALAT__" maka
        kembali galat("GalatTipe", "lempar butuh objek galat, buat dengan galat(jenis, pesan)", null)
    akhir
    kembali x
}

// dipanggil runtime bila tidak ada handler: cetak ke stderr, keluar kode 1
fungsi __galatTakTertangkap(g) {
    tulisStderr("[GALAT] #{g[1]}: #{g[2]}")
    _sistemKeluar(1)
}

fungsi __galatNol() {
    kembali galat("GalatNilai", "pembagian dengan nol", null)
}

fungsi __galatBerkas() {
    kembali galat("GalatBerkas", "tidak bisa membuka atau menulis berkas", null)
}

fungsi __galatIndeks() {
    kembali galat("GalatIndeks", "indeks di luar batas larik", null)
}

fungsi __galatIndeksTipe() {
    kembali galat("GalatTipe", "indeks hanya untuk larik", null)
}

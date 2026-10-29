// Library/io.lp
// Library Input/Output untuk Loop
// Wrapper dan utilitas di atas builtin I/O interpreter
// Pakai dengan: pakai io


// 1. OUTPUT KONSOL


fungsi cetakBaris(pesan) {
    // Print ke stdout + newline
    cetak(pesan)
}

fungsi cetakInline(pesan) {
    // Print ke stdout tanpa newline
    tulis(pesan)
}

fungsi cetakKosong() {
    // Print baris kosong
    barisBaru()
}

fungsi cetakError(pesan) {
    // Print ke stderr dengan prefix [error]
    tulisError(pesan)
}

fungsi cetakPeringatan(pesan) {
    // Print ke stderr dengan prefix [peringatan]
    tulisStderr("[peringatan] " + pesan)
}

fungsi cetakDebug(pesan) {
    // Print ke stderr dengan prefix [debug]
    tulisStderr("[debug] " + pesan)
}

fungsi cetakFormat(template, nilai) {
    // Ganti placeholder {0} dengan nilai dari larik
    // Contoh: cetakFormat("Halo {0}, kamu {1} tahun", ["Budi", 20])
    teks hasil = template
    angka i = 0
    angka n = panjang(nilai)
    selama i < n {
        teks placeholder = "{" + i + "}"
        teks val = "" + nilai[i]
        // ganti() adalah builtin
        hasil = ganti(hasil, placeholder, val)
        i = i + 1
    }
    cetak(hasil)
}

fungsi cetakLarik(arr) {
    // Print setiap elemen larik per baris
    angka i = 0
    angka n = panjang(arr)
    selama i < n {
        cetak("[" + i + "] " + arr[i])
        i = i + 1
    }
}

fungsi cetakGaris(karakter, panjangGaris) {
    // Print garis pemisah, misal: cetakGaris("-", 40)
    teks hasil = ""
    angka i = 0
    selama i < panjangGaris {
        hasil = hasil + karakter
        i = i + 1
    }
    cetak(hasil)
}


// 2. INPUT KONSOL


fungsi baca() {
    // Baca satu baris dari stdin
    kembali masukan()
}

fungsi bacaDenganPrompt(prompt) {
    // Baca input dengan teks prompt
    kembali masukan(prompt)
}

fungsi bacaAngka(prompt) {
    // Baca input dan konversi ke angka
    teks raw = masukan(prompt)
    kembali keAngka(raw)
}

fungsi bacaAngkaBulat(prompt) {
    // Baca input, konversi ke angka, bulatkan
    angka n = bacaAngka(prompt)
    kembali lantai(n)
}

fungsi bacaTeks(prompt) {
    // Baca input teks (trim whitespace)
    teks raw = masukan(prompt)
    kembali trim(raw)
}

fungsi bacaYaTidak(prompt) {
    // Baca y/n dari user, return bool
    // y/ya/yes = benar, n/tidak/no = salah
    teks raw = hurufKecil(trim(masukan(prompt + " (y/n): ")))
    jika raw == "y" atau raw == "ya" atau raw == "yes" maka
        kembali benar
    akhir
    kembali salah
}

fungsi bacaRahasia(prompt) {
    // Baca input sensitif (password) — tidak bisa hide input di Loop,
    // tapi setidaknya memberi tahu user
    cetak("[!] Input berikut akan terlihat di terminal")
    kembali masukan(prompt)
}

fungsi bacaLarik(prompt, pemisah) {
    // Baca input dan split jadi larik berdasarkan pemisah
    // Contoh: bacaLarik("Masukkan angka: ", ",") → [1, 2, 3]
    teks raw = masukan(prompt)
    kembali pisah(raw, pemisah)
}

fungsi konfirmasi(pesan) {
    // Minta konfirmasi user sebelum lanjut
    kembali bacaYaTidak(pesan)
}


// 3. FILE — BACA


fungsi bacaFileContent(path) {
    // Baca seluruh isi file sebagai teks
    kembali bacaFileTeks(path)
}

fungsi bacaBaris(path) {
    // Baca file dan return larik per baris.
    // Galat: GalatBerkas (file tidak ada / tidak terbaca).
    // File kosong menghasilkan larik kosong.
    teks isi = bacaFileTeks(path)
    jika isi == "" maka
        kembali []
    akhir
    kembali pisah(isi, "\n")
}

fungsi bacaJSONContent(path) {
    // Baca file, return isinya sebagai teks mentah
    // (parsing JSON belum didukung native — return raw string)
    kembali bacaFileTeks(path)
}

fungsi fileAda(path) {
    // benar bila path ada (file kosong tetap dianggap ada)
    kembali adaBerkas(path)
}

fungsi ukuranFile(path) {
    // Ukuran file dalam BYTE (bukan karakter). Galat: GalatBerkas.
    kembali panjangBita(bacaBita(path))
}


// 4. FILE — TULIS


fungsi tulisFileContent(path, isi) {
    // Tulis teks ke file (overwrite)
    kembali tulisFileTeks(path, isi)
}

fungsi tambahKeFile(path, isi) {
    // Append teks ke file
    kembali tambahFileTeks(path, isi)
}

fungsi tulisBarisKeFile(path, baris) {
    // Tulis satu baris ke file (overwrite) + newline
    kembali tulisFileTeks(path, baris + "\n")
}

fungsi tambahBarisKeFile(path, baris) {
    // Append satu baris ke file + newline
    kembali tambahFileTeks(path, baris + "\n")
}

fungsi tulisLarikKeFile(path, arr) {
    // Tulis setiap elemen larik sebagai baris di file
    teks isi = ""
    angka i = 0
    angka n = panjang(arr)
    selama i < n {
        isi = isi + arr[i] + "\n"
        i = i + 1
    }
    kembali tulisFileTeks(path, isi)
}

fungsi salinFile(pathAsal, pathTujuan) {
    // Salin file apa adanya (aman untuk data biner, byte 0x00 ikut tersalin).
    // Penulisan atomik: tujuan lama tidak rusak bila gagal di tengah.
    // Kembali: path tujuan. Galat: GalatBerkas.
    kembali tulisAtomik(pathTujuan, bacaBita(pathAsal))
}


// 5. STDIN — BACA MULTI BARIS


fungsi bacaSemuaBaris(prompt, stopKata) {
    // Baca banyak baris sampai user ketik stopKata
    // Contoh: bacaSemuaBaris("Ketik teks (EOF untuk selesai):", "EOF")
    larik baris = []
    cetak(prompt)
    selama benar {
        teks line = masukan("")
        jika line == stopKata maka
            kembali baris
        akhir
        baris = tambah(baris, line)
    }
    kembali baris
}

fungsi bacaNBaris(n) {
    // Baca tepat n baris dari stdin
    larik hasil = []
    angka i = 0
    selama i < n {
        hasil = tambah(hasil, masukan(""))
        i = i + 1
    }
    kembali hasil
}


// 6. FORMAT OUTPUT


fungsi kananRata(str, lebar) {
    // Pad teks ke kanan sampai lebar tertentu
    angka n = panjang(str)
    teks hasil = str
    selama n < lebar {
        hasil = " " + hasil
        n = n + 1
    }
    kembali hasil
}

fungsi kiriRata(str, lebar) {
    // Pad teks ke kiri sampai lebar tertentu
    angka n = panjang(str)
    teks hasil = str
    selama n < lebar {
        hasil = hasil + " "
        n = n + 1
    }
    kembali hasil
}

fungsi tengahRata(str, lebar) {
    // Pusatkan teks dalam lebar tertentu
    angka n = panjang(str)
    jika n >= lebar maka
        kembali str
    akhir
    angka sisaTotal = lebar - n
    angka sisaKiri = sisaTotal / 2
    angka sisaKanan = sisaTotal - sisaKiri
    teks hasil = ""
    angka i = 0
    selama i < sisaKiri {
        hasil = hasil + " "
        i = i + 1
    }
    hasil = hasil + str
    i = 0
    selama i < sisaKanan {
        hasil = hasil + " "
        i = i + 1
    }
    kembali hasil
}

fungsi ulangiTeks(str, kali) {
    // Ulangi teks sebanyak kali
    teks hasil = ""
    angka i = 0
    selama i < kali {
        hasil = hasil + str
        i = i + 1
    }
    kembali hasil
}

fungsi cetakTabel(header, baris, lebarKolom) {
    // Print tabel sederhana
    // header: larik nama kolom
    // baris: larik dari larik (tiap baris = larik nilai)
    // lebarKolom: lebar tiap kolom (angka)
    angka nKol = panjang(header)
    // Print header
    teks barisHeader = ""
    angka i = 0
    selama i < nKol {
        barisHeader = barisHeader + kiriRata("" + header[i], lebarKolom) + " "
        i = i + 1
    }
    cetak(barisHeader)
    // Print garis pemisah
    cetakGaris("-", nKol * (lebarKolom + 1))
    // Print tiap baris
    angka r = 0
    angka nBaris = panjang(baris)
    selama r < nBaris {
        teks barisStr = ""
        angka c = 0
        selama c < nKol {
            barisStr = barisStr + kiriRata("" + baris[r][c], lebarKolom) + " "
            c = c + 1
        }
        cetak(barisStr)
        r = r + 1
    }
}


// 7. LOG


fungsi logInfo(pesan) {
    tulisStderr("[INFO] " + pesan)
}

fungsi logPeringatan(pesan) {
    tulisStderr("[WARN] " + pesan)
}

fungsi logError(pesan) {
    tulisError(pesan)
}

fungsi logDebug(label, nilai) {
    tulisStderr("[DEBUG] " + label + " = " + nilai)
}


// 8. EKSPOR



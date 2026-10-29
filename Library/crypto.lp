// Library/crypto.lp
// Pembungkus tipis. Algoritma (AES-256-GCM, PBKDF2-HMAC-SHA256, HMAC, RNG) ada
// di inti native lewat OpenSSL EVP; file ini hanya merapikan API, memvalidasi
// argumen, dan melempar galat berjenis. Library tidak mencetak apa pun.
//
// Format paket data : garam(16) || nonce(12) || ciphertext || tag(16)
// Format berkas .aes: sama dengan paket data (biner).
//
// Galat yang mungkin dilempar:
//   GalatNilai  argumen kosong / bentuk salah (sandi kosong, nama bukan .aes)
//   GalatTipe   tipe argumen salah
//   GalatBerkas berkas tidak ada / tidak bisa dibaca atau ditulis / bukan hasil enkripsiBerkas
//   GalatKunci  sandi salah ATAU data diubah (keduanya sengaja tidak dibedakan)
//
// Catatan biaya: setiap enkripsi/dekripsi menurunkan kunci dengan PBKDF2 600.000
// iterasi (sekitar 0,1-0,5 detik). Itu disengaja; jangan dipanggil dalam loop
// untuk ribuan pesan kecil, pakai satu kunci (buatKunci) lalu enkripsiGCM.
// Setiap berkas memakai garam acak baru, jadi kunci GCM-nya selalu unik per
// berkas dan batas 2^32 pesan per kunci (NIST SP 800-38D) tidak tercapai.

angka ITERASI_PBKDF2 = 600000
angka UKURAN_GARAM = 16
angka UKURAN_NONCE = 12
angka UKURAN_TAG = 16

fungsi _wajibTeks(nilai, nama) {
    jika tipe(nilai) != "teks" maka
        lempar galat("GalatTipe", nama + " harus teks, dapat " + tipe(nilai))
    akhir
}

fungsi _wajibSandi(sandi) {
    jika tipe(sandi) != "teks" dan tipe(sandi) != "bita" maka
        lempar galat("GalatTipe", "sandi harus teks atau bita, dapat " + tipe(sandi))
    akhir
    jika panjang(sandi) == 0 maka
        lempar galat("GalatNilai", "sandi tidak boleh kosong")
    akhir
}

// buatKunci(sandi, garam) -> bita (32 byte)
// Galat: GalatNilai (sandi atau garam kosong), GalatTipe
fungsi buatKunci(sandi, garam) {
    _wajibSandi(sandi)
    kembali turunkanKunci(sandi, garam, ITERASI_PBKDF2)
}

// buatKunciPrima(nilai) -> kunci prima untuk acak() / enkripsiPakaiCipherPrima.
// Nama lama buatKunci(nilai) dipindah ke sini karena buatKunci sekarang
// menurunkan kunci kripto dari sandi.
fungsi buatKunciPrima(nilai) {
    kembali prima(nilai)
}

// enkripsiData(data, sandi) -> bita
// data: bita atau teks. Hasil: garam(16) || nonce(12) || ciphertext || tag(16).
// Galat: GalatNilai (sandi kosong), GalatTipe
fungsi enkripsiData(data, sandi) {
    _wajibSandi(sandi)
    bita garam = acakAman(UKURAN_GARAM)
    bita kunci = buatKunci(sandi, garam)
    kembali sambungBita(garam, enkripsiGCM(kunci, data))
}

// dekripsiData(paket, sandi) -> bita
// Galat: GalatNilai (paket terlalu pendek), GalatKunci (sandi salah / data diubah)
fungsi dekripsiData(paket, sandi) {
    _wajibSandi(sandi)
    jika tipe(paket) != "bita" maka
        lempar galat("GalatTipe", "paket harus bita, dapat " + tipe(paket))
    akhir
    jika panjangBita(paket) < UKURAN_GARAM + UKURAN_NONCE + UKURAN_TAG maka
        lempar galat("GalatNilai", "paket terlalu pendek untuk hasil enkripsiData")
    akhir
    bita kunci = buatKunci(sandi, potongBita(paket, 0, UKURAN_GARAM))
    kembali dekripsiGCM(kunci, potongBita(paket, UKURAN_GARAM, panjangBita(paket)))
}

// enkripsiBerkas(path, sandi) -> teks (path hasil: path + ".aes")
// Galat: GalatBerkas (berkas tidak ada / tujuan tidak bisa ditulis), GalatNilai, GalatTipe
fungsi enkripsiBerkas(path, sandi) {
    _wajibTeks(path, "path")
    _wajibSandi(sandi)
    bita garam = acakAman(UKURAN_GARAM)
    bita kunci = buatKunci(sandi, garam)
    bita paket = enkripsiGCM(kunci, bacaBita(path))
    teks tujuan = path + ".aes"
    tulisAtomik(tujuan, sambungBita(garam, paket))
    kembali tujuan
}

// dekripsiBerkas(path, sandi) -> teks (path hasil, tanpa ".aes")
// Galat: GalatNilai (nama bukan .aes), GalatBerkas (berkas hilang / bukan hasil
//        enkripsiBerkas), GalatKunci (sandi salah atau berkas diubah)
// Verifikasi tag selesai SEBELUM tulisAtomik, jadi sandi salah tidak menyentuh
// berkas tujuan yang sudah ada.
fungsi dekripsiBerkas(path, sandi) {
    _wajibTeks(path, "path")
    _wajibSandi(sandi)
    jika akhiriDengan(path, ".aes") == salah maka
        lempar galat("GalatNilai", "nama berkas harus berakhiran .aes: " + path)
    akhir
    bita data = bacaBita(path)
    jika panjangBita(data) < UKURAN_GARAM + UKURAN_NONCE + UKURAN_TAG maka
        lempar galat("GalatBerkas", "bukan hasil enkripsiBerkas: " + path)
    akhir
    bita kunci = buatKunci(sandi, potongBita(data, 0, UKURAN_GARAM))
    milik asli = null
    coba {
        asli = dekripsiGCM(kunci, potongBita(data, UKURAN_GARAM, panjangBita(data)))
    } tangkap {
        jika jenisGalat(e) == "GalatKunci" dan _tampakFormatLama(data) maka
            lempar galat("GalatKunci", "autentikasi gagal; berkas tampak berformat lama, coba dekripsiBerkasV0", e)
        akhir
        lempar e
    }
    teks tujuan = potong(path, 0, panjang(path) - 4)
    tulisAtomik(tujuan, asli)
    kembali tujuan
}

// Berkas .aes lama berisi teks hex (karakter 0-9 a-f saja).
fungsi _tampakFormatLama(data) {
    jika panjangBita(data) < 64 atau panjangBita(data) % 2 != 0 maka
        kembali salah
    akhir
    coba {
        kembali regexCocok(keTeks(data), "^[0-9a-fA-F]+$")
    } tangkap {
        kembali salah
    }
}

// dekripsiBerkasV0(path, sandi) -> teks (path hasil)
// SEMENTARA, untuk migrasi berkas .aes format lama (AES-128-CBC, hex teks, tanpa
// autentikasi, sandi dipotong/diisi nol ke 16 byte). Dihapus dua rilis lagi;
// setelah migrasi, enkripsi ulang dengan enkripsiBerkas.
// Galat: GalatNilai, GalatBerkas, GalatKunci (padding tidak valid)
fungsi dekripsiBerkasV0(path, sandi) {
    _wajibTeks(path, "path")
    _wajibSandi(sandi)
    jika akhiriDengan(path, ".aes") == salah maka
        lempar galat("GalatNilai", "nama berkas harus berakhiran .aes: " + path)
    akhir
    teks hex = trimTeks(bacaFileTeks(path))
    bita data = dariHex(hex)
    bita kunci = keBita(sandi)
    jika panjangBita(kunci) > 16 maka
        kunci = potongBita(kunci, 0, 16)
    akhir
    larik nol = []
    angka i = panjangBita(kunci)
    selama i < 16 {
        nol = tambah(nol, 0)
        i = i + 1
    }
    kunci = sambungBita(kunci, keBita(nol))
    bita asli = dekripsiCBCLama(kunci, data)
    teks tujuan = potong(path, 0, panjang(path) - 4)
    tulisAtomik(tujuan, asli)
    kembali tujuan
}

// Nama lama, dipertahankan dua rilis. Sekarang memakai AES-256-GCM dan melempar
// galat (dulu mengembalikan "" saat gagal).
fungsi enkripsiFileAES(namaFile, kunci) {
    kembali enkripsiBerkas(namaFile, kunci)
}

fungsi dekripsiFileAES(namaFile, kunci) {
    kembali dekripsiBerkas(namaFile, kunci)
}

// enkripsi(teks, sandi) -> teks hex dari enkripsiData(). Pembungkus kompatibilitas.
fungsi enkripsi(data, sandi) {
    kembali keHex(enkripsiData(data, sandi))
}

// dekripsi(hex, sandi) -> teks. Galat: GalatKunci, GalatNilai (hex rusak/bukan UTF-8)
fungsi dekripsi(data, sandi) {
    _wajibTeks(data, "data")
    kembali keTeks(dekripsiData(dariHex(data), sandi))
}

// Lapisan "cipher prima": ciphertext baru di-XOR per byte dengan k = angkaPrima & 255.
// Data bita, jadi byte 0x00 aman; hasil selalu hex (tidak ada lagi kasus khusus untuk
// prima 53/97/101). Ini obfuskasi tambahan, bukan keamanan: keamanan datang dari GCM.
fungsi _kunciXORPrima(angkaPrima) {
    kembali bitAND(angkaPrima, 255)
}

fungsi _lapisPrima(data, k) {
    larik hasil = []
    angka i = 0
    angka n = panjangBita(data)
    selama i < n {
        hasil = tambah(hasil, bitXOR(data[i], k))
        i = i + 1
    }
    kembali keBita(hasil)
}

fungsi enkripsiPakaiCipherPrima(data, sandi, angkaPrima) {
    kembali keHex(_lapisPrima(enkripsiData(data, sandi), _kunciXORPrima(angkaPrima)))
}

fungsi dekripsiPakaiCipherPrima(data, sandi, angkaPrima) {
    _wajibTeks(data, "data")
    kembali keTeks(dekripsiData(_lapisPrima(dariHex(data), _kunciXORPrima(angkaPrima)), sandi))
}

// enkripsiAngka / dekripsiAngka
// Angka dikodekan eksak sebagai m * 2^e (round trip tepat untuk semua angka
// berhingga), lalu dienkripsi AES-256-GCM dengan kunci tetap bawaan. Kunci tetap
// berarti ini penyamaran, bukan perlindungan: siapa pun yang membaca library bisa
// mendekripsi. Hasil: teks hex.
// enkripsiAngka: masukan bukan angka / NaN / tak hingga menghasilkan "".
// dekripsiAngka: masukan bukan teks / bukan hasil enkripsiAngka menghasilkan 0
// (kontrak lama, seperti keAngka("abc")).
fungsi _angkaKeKode(x) {
    angka m = x
    angka e = 0
    selama absolut(m) >= 9007199254740992 {
        m = m / 2
        e = e + 1
    }
    selama m != lantai(m) {
        m = m * 2
        e = e - 1
    }
    kembali "" + m + "|" + e
}

fungsi _kodeKeAngka(t) {
    angka idx = cariTeks(t, "|")
    jika idx < 1 maka
        kembali 0
    akhir
    angka m = keAngka(potong(t, 0, idx))
    angka e = keAngka(potong(t, idx + 1))
    jika e > 1100 atau e < -1100 maka
        kembali 0
    akhir
    selama e > 0 {
        m = m * 2
        e = e - 1
    }
    selama e < 0 {
        m = m / 2
        e = e + 1
    }
    kembali m
}

fungsi _kunciAngka() {
    kembali hmacSha256("kunci_angka_default", "loop:enkripsiAngka:v1")
}

fungsi enkripsiAngka(nilai) {
    jika tipe(nilai) == "teks" maka
        nilai = keAngka(nilai)
    akhir
    jika tipe(nilai) != "angka" maka
        kembali ""
    akhir
    jika nilai - nilai != 0 maka
        kembali ""
    akhir
    kembali keHex(enkripsiGCM(_kunciAngka(), _angkaKeKode(nilai)))
}

fungsi dekripsiAngka(nilai) {
    jika tipe(nilai) != "teks" maka
        kembali 0
    akhir
    coba {
        kembali _kodeKeAngka(keTeks(dekripsiGCM(_kunciAngka(), dariHex(nilai))))
    } tangkap {
        jika jenisGalat(e) == "GalatNilai" atau jenisGalat(e) == "GalatKunci" maka
            kembali 0
        akhir
        lempar e
    }
}

// Pemeriksaan tipe
fungsi isEnkripsi(nilai) {
    kembali tipe(nilai) == "terenkripsi"
}

fungsi isKunciPrima(nilai) {
    kembali tipe(nilai) == "kunci_prima"
}

fungsi isGanda(nilai) {
    kembali tipe(nilai) == "ganda"
}

// isAES(nilai): benar bila nilai tampak seperti hasil enkripsi() (hex dari
// garam+nonce+ciphertext+tag, minimal 44 byte = 88 karakter hex).
fungsi isAES(nilai) {
    jika tipe(nilai) != "teks" maka
        kembali salah
    akhir
    jika panjang(nilai) < 88 atau panjang(nilai) % 2 != 0 maka
        kembali salah
    akhir
    kembali regexCocok(nilai, "^[0-9a-fA-F]+$")
}

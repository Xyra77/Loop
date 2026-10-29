// contoh/aes_murni.lp — BAHAN BELAJAR, bukan library.
//
// Implementasi AES-128 (blok, CBC, PKCS7) murni dalam Loop, dipindahkan dari
// Library/crypto.lp lama. Ini sengaja tidak diekspor sebagai library:
//   - lambat (sekitar 135 byte/detik terukur untuk enkripsi + dekripsi),
//   - bukan konstan-waktu (tabel indeks bergantung rahasia),
//   - tanpa autentikasi (CBC) dan tanpa PBKDF2.
// Untuk kode nyata pakai Library/crypto.lp (AES-256-GCM via OpenSSL).
// Blok AES-nya diuji terhadap vektor FIPS-197 di tes/contoh/aes_murni.lp.
// Muat dari akar repo dengan: bacaFile("contoh/aes_murni.lp")

// 1. KONSTANTA

angka BLOK_UKURAN = 16
angka KUNCI_UKURAN = 16
angka ITERASI_KUNCI = 10
angka IV_PANJANG = 16

// 2. TABEL S-BOX AES

larik SBOX = [
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
]

larik INV_SBOX = [
    0x52,0x09,0x6a,0xd5,0x30,0x36,0xa5,0x38,0xbf,0x40,0xa3,0x9e,0x81,0xf3,0xd7,0xfb,
0x7c,0xe3,0x39,0x82,0x9b,0x2f,0xff,0x87,0x34,0x8e,0x43,0x44,0xc4,0xde,0xe9,0xcb,
0x54,0x7b,0x94,0x32,0xa6,0xc2,0x23,0x3d,0xee,0x4c,0x95,0x0b,0x42,0xfa,0xc3,0x4e,
0x08,0x2e,0xa1,0x66,0x28,0xd9,0x24,0xb2,0x76,0x5b,0xa2,0x49,0x6d,0x8b,0xd1,0x25,
0x72,0xf8,0xf6,0x64,0x86,0x68,0x98,0x16,0xd4,0xa4,0x5c,0xcc,0x5d,0x65,0xb6,0x92,
0x6c,0x70,0x48,0x50,0xfd,0xed,0xb9,0xda,0x5e,0x15,0x46,0x57,0xa7,0x8d,0x9d,0x84,
0x90,0xd8,0xab,0x00,0x8c,0xbc,0xd3,0x0a,0xf7,0xe4,0x58,0x05,0xb8,0xb3,0x45,0x06,
0xd0,0x2c,0x1e,0x8f,0xca,0x3f,0x0f,0x02,0xc1,0xaf,0xbd,0x03,0x01,0x13,0x8a,0x6b,
0x3a,0x91,0x11,0x41,0x4f,0x67,0xdc,0xea,0x97,0xf2,0xcf,0xce,0xf0,0xb4,0xe6,0x73,
0x96,0xac,0x74,0x22,0xe7,0xad,0x35,0x85,0xe2,0xf9,0x37,0xe8,0x1c,0x75,0xdf,0x6e,
0x47,0xf1,0x1a,0x71,0x1d,0x29,0xc5,0x89,0x6f,0xb7,0x62,0x0e,0xaa,0x18,0xbe,0x1b,
0xfc,0x56,0x3e,0x4b,0xc6,0xd2,0x79,0x20,0x9a,0xdb,0xc0,0xfe,0x78,0xcd,0x5a,0xf4,
0x1f,0xdd,0xa8,0x33,0x88,0x07,0xc7,0x31,0xb1,0x12,0x10,0x59,0x27,0x80,0xec,0x5f,
0x60,0x51,0x7f,0xa9,0x19,0xb5,0x4a,0x0d,0x2d,0xe5,0x7a,0x9f,0x93,0xc9,0x9c,0xef,
0xa0,0xe0,0x3b,0x4d,0xae,0x2a,0xf5,0xb0,0xc8,0xeb,0xbb,0x3c,0x83,0x53,0x99,0x61,
0x17,0x2b,0x04,0x7e,0xba,0x77,0xd6,0x26,0xe1,0x69,0x14,0x63,0x55,0x21,0x0c,0x7d
]

// 3. FUNGSI UTILITY

fungsi teksKeBytes(data) {
    larik hasil = []
    angka i = 0
    angka n = panjang(data)
    selama i < n {
        hasil = tambah(hasil, kodeKarakter(karakter(data, i)))
        i = i + 1
    }
    kembali hasil
}

fungsi bytesKeTeks(bytes) {
    teks hasil = ""
    angka i = 0
    angka n = panjang(bytes)
    selama i < n {
        hasil = tambah(hasil, dariKode(bytes[i]))
        i = i + 1
    }
    kembali hasil
}

// bytesKeTeks/dariKode gak aman buat data biner: teks di Loop adalah C string
// null-terminated (Val.str, lihat loop.c), jadi byte 0x00 di tengah data bakal
// motong string diam-diam (strlen berhenti duluan). Ciphertext AES pasti
// ngandung byte random termasuk 0x00 buat pesan >1 blok, makanya wajib lewat
// hex dulu sebelum disimpan sebagai teks hex cuma pakai karakter ASCII
// printable (0-9a-f), aman disimpan/dicetak/dikirim tanpa resiko truncation.

fungsi byteKeHex(b) {
    teks digit = "0123456789abcdef"
    angka tinggi = lantai(b / 16)
    angka rendah = b - (tinggi * 16)
    kembali tambah(karakter(digit, tinggi), karakter(digit, rendah))
}

fungsi bytesKeHex(bytes) {
    teks hasil = ""
    angka i = 0
    angka n = panjang(bytes)
    selama i < n {
        hasil = tambah(hasil, byteKeHex(bytes[i]))
        i = i + 1
    }
    kembali hasil
}

fungsi hexKeByte(c) {
    angka kode = kodeKarakter(c)
    jika kode >= 48 dan kode <= 57 maka
    kembali kode - 48
    akhir
    jika kode >= 97 dan kode <= 102 maka
    kembali kode - 97 + 10
    akhir
    jika kode >= 65 dan kode <= 70 maka
    kembali kode - 65 + 10
    akhir
    kembali 0
}

fungsi hexKeBytes(hex) {
    larik hasil = []
    angka i = 0
    angka n = panjang(hex)
    selama i + 1 < n {
        angka tinggi = hexKeByte(karakter(hex, i))
        angka rendah = hexKeByte(karakter(hex, i + 1))
        hasil = tambah(hasil, tinggi * 16 + rendah)
        i = i + 2
    }
    kembali hasil
}

fungsi xorArray(arr1, arr2) {
    larik hasil = []
    angka i = 0
    angka n1 = panjang(arr1)
    angka n2 = panjang(arr2)
    selama i < n1 {
        angka val1 = arr1[i]
        angka val2 = arr2[i % n2]
        hasil = tambah(hasil, bitXOR(val1, val2))
        i = i + 1
    }
    kembali hasil
}

// 4. PADDING PKCS#7

fungsi padPKCS7(data, ukuranBlok) {
    angka n = panjang(data)
    angka padding = ukuranBlok - (n % ukuranBlok)
    jika padding == 0 maka
    padding = ukuranBlok
    akhir
    teks hasil = data
    angka i = 0
    selama i < padding {
        hasil = tambah(hasil, dariKode(padding))
        i = i + 1
    }
    kembali hasil
}

fungsi unpadPKCS7(data) {
    angka n = panjang(data)
    jika n == 0 maka
    kembali data
    akhir
    angka padding = kodeKarakter(karakter(data, n - 1))
    jika padding > n atau padding < 1 maka
    kembali data
    akhir
    angka i = n - padding
    selama i < n {
        jika kodeKarakter(karakter(data, i)) != padding maka
        kembali data
        akhir
        i = i + 1
    }
    kembali potong(data, 0, n - padding)
}

// 5. PERKALIAN GF(2^8) UNTUK AES MIXCOLUMNS

fungsi gf_mul(a, b) {
    angka hasil = 0
    angka aa = bitAND(a, 0xff)
    angka bb = bitAND(b, 0xff)
    angka i = 0
    selama i < 8 {
        jika bitAND(aa, 1) != 0 maka
        hasil = bitXOR(hasil, bb)
        akhir
        aa = bitKanan(aa, 1)
        aa = bitAND(aa, 0x7f)
        bb = bitKiri(bb, 1)
        jika bitAND(bb, 0x100) != 0 maka
        bb = bitXOR(bb, 0x11b)
        akhir
        bb = bitAND(bb, 0xff)
        i = i + 1
    }
    kembali hasil
}

// 6. FUNGSI INTI AES

fungsi subBytes(blok) {
    larik hasil = []
    angka i = 0
    angka n = panjang(blok)
    selama i < n {
        hasil = tambah(hasil, SBOX[blok[i]])
        i = i + 1
    }
    kembali hasil
}

fungsi invSubBytes(blok) {
    larik hasil = []
    angka i = 0
    angka n = panjang(blok)
    selama i < n {
        hasil = tambah(hasil, INV_SBOX[blok[i]])
        i = i + 1
    }
    kembali hasil
}

fungsi shiftRows(blok) {
    // State column-major (standar FIPS-197): indeks = baris + 4*kolom
    larik hasil = []
    hasil = tambah(hasil, blok[0])
    hasil = tambah(hasil, blok[5])
    hasil = tambah(hasil, blok[10])
    hasil = tambah(hasil, blok[15])
    hasil = tambah(hasil, blok[4])
    hasil = tambah(hasil, blok[9])
    hasil = tambah(hasil, blok[14])
    hasil = tambah(hasil, blok[3])
    hasil = tambah(hasil, blok[8])
    hasil = tambah(hasil, blok[13])
    hasil = tambah(hasil, blok[2])
    hasil = tambah(hasil, blok[7])
    hasil = tambah(hasil, blok[12])
    hasil = tambah(hasil, blok[1])
    hasil = tambah(hasil, blok[6])
    hasil = tambah(hasil, blok[11])
    kembali hasil
}

fungsi invShiftRows(blok) {
    larik hasil = []
    hasil = tambah(hasil, blok[0])
    hasil = tambah(hasil, blok[13])
    hasil = tambah(hasil, blok[10])
    hasil = tambah(hasil, blok[7])
    hasil = tambah(hasil, blok[4])
    hasil = tambah(hasil, blok[1])
    hasil = tambah(hasil, blok[14])
    hasil = tambah(hasil, blok[11])
    hasil = tambah(hasil, blok[8])
    hasil = tambah(hasil, blok[5])
    hasil = tambah(hasil, blok[2])
    hasil = tambah(hasil, blok[15])
    hasil = tambah(hasil, blok[12])
    hasil = tambah(hasil, blok[9])
    hasil = tambah(hasil, blok[6])
    hasil = tambah(hasil, blok[3])
    kembali hasil
}

fungsi _setIndeks(arr, indeks, nilai) {
    // Loop gak punya index assignment (arr[i]=x), jadi rebuild array baru
    // dengan elemen di 'indeks' diganti 'nilai'. Dipakai internal crypto.lp
    // biar mixColumns/invMixColumns gak gantung ke dasar.lp.
    larik hasil = []
    angka n = panjang(arr)
    angka i = 0
    selama i < n {
        jika i == indeks maka
        hasil = tambah(hasil, nilai)
        selain
        hasil = tambah(hasil, arr[i])
        akhir
        i = i + 1
    }
    kembali hasil
}

fungsi mixColumns(blok) {
    // State row-major: setiap kolom = blok[i], blok[i+4], blok[i+8], blok[i+12]
    // BUG LAMA: hasil ditulis berurutan pakai tambah() (r0,r1,r2,r3 numpuk di
    // 4 slot beruntun per kolom), padahal harus disebar balik ke posisi
    // i, i+4, i+8, i+12 biar konsisten row-major sama shiftRows/addRoundKey.
    // Itu bikin invMixColumns(mixColumns(x)) != x -> AES round-trip gagal total.
    larik hasil = [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]
    angka i = 0
    selama i < 4 {
        angka a0 = blok[i * 4]
        angka a1 = blok[i * 4 + 1]
        angka a2 = blok[i * 4 + 2]
        angka a3 = blok[i * 4 + 3]
        angka r0 = bitXOR(bitXOR(bitXOR(gf_mul(a0, 2), gf_mul(a1, 3)), a2), a3)
        angka r1 = bitXOR(bitXOR(bitXOR(a0, gf_mul(a1, 2)), gf_mul(a2, 3)), a3)
        angka r2 = bitXOR(bitXOR(bitXOR(a0, a1), gf_mul(a2, 2)), gf_mul(a3, 3))
        angka r3 = bitXOR(bitXOR(bitXOR(gf_mul(a0, 3), a1), a2), gf_mul(a3, 2))
        hasil = _setIndeks(hasil, i * 4, r0)
        hasil = _setIndeks(hasil, i * 4 + 1, r1)
        hasil = _setIndeks(hasil, i * 4 + 2, r2)
        hasil = _setIndeks(hasil, i * 4 + 3, r3)
        i = i + 1
    }
    kembali hasil
}

fungsi invMixColumns(blok) {
    larik hasil = [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]
    angka i = 0
    selama i < 4 {
        angka a0 = blok[i * 4]
        angka a1 = blok[i * 4 + 1]
        angka a2 = blok[i * 4 + 2]
        angka a3 = blok[i * 4 + 3]
        angka r0 = bitXOR(bitXOR(bitXOR(gf_mul(a0, 14), gf_mul(a1, 11)), gf_mul(a2, 13)), gf_mul(a3, 9))
        angka r1 = bitXOR(bitXOR(bitXOR(gf_mul(a0, 9), gf_mul(a1, 14)), gf_mul(a2, 11)), gf_mul(a3, 13))
        angka r2 = bitXOR(bitXOR(bitXOR(gf_mul(a0, 13), gf_mul(a1, 9)), gf_mul(a2, 14)), gf_mul(a3, 11))
        angka r3 = bitXOR(bitXOR(bitXOR(gf_mul(a0, 11), gf_mul(a1, 13)), gf_mul(a2, 9)), gf_mul(a3, 14))
        hasil = _setIndeks(hasil, i * 4, r0)
        hasil = _setIndeks(hasil, i * 4 + 1, r1)
        hasil = _setIndeks(hasil, i * 4 + 2, r2)
        hasil = _setIndeks(hasil, i * 4 + 3, r3)
        i = i + 1
    }
    kembali hasil
}

fungsi addRoundKey(blok, kunci) {
    kembali xorArray(blok, kunci)
}

// 7. KEY SCHEDULE AES

fungsi rotWord(kata) {
    larik hasil = []
    hasil = tambah(hasil, kata[1])
    hasil = tambah(hasil, kata[2])
    hasil = tambah(hasil, kata[3])
    hasil = tambah(hasil, kata[0])
    kembali hasil
}

fungsi subWord(kata) {
    larik hasil = []
    angka i = 0
    selama i < 4 {
        hasil = tambah(hasil, SBOX[kata[i]])
        i = i + 1
    }
    kembali hasil
}

fungsi rcon(ronde) {
    angka rc = 1
    angka i = 1
    selama i < ronde {
        rc = gf_mul(rc, 2)
        i = i + 1
    }
    larik hasil = [rc, 0, 0, 0]
    kembali hasil
}

fungsi ekspansiKunci(kunciAwal, jumlahRonde) {
    // kunciAwal: array 16 byte
    // Output: array flat (jumlahRonde+1)*16 byte
    larik w = []
    angka i = 0
    selama i < 4 {
        larik kata = []
        angka j = 0
        selama j < 4 {
            kata = tambah(kata, kunciAwal[i * 4 + j])
            j = j + 1
        }
        w = tambah(w, kata)
        i = i + 1
    }
    angka ronde = 1
    selama ronde <= jumlahRonde {
        larik temp = w[panjang(w) - 1]
        jika (panjang(w) % 4) == 0 maka
        temp = rotWord(temp)
        temp = subWord(temp)
        larik rconKata = rcon(ronde)
        temp = xorArray(temp, rconKata)
        akhir
        larik wPrev = w[panjang(w) - 4]
        larik wBaru = xorArray(wPrev, temp)
        w = tambah(w, wBaru)
        angka k = 1
        selama k < 4 {
            wPrev = w[panjang(w) - 4]
            temp = w[panjang(w) - 1]
            wBaru = xorArray(wPrev, temp)
            w = tambah(w, wBaru)
            k = k + 1
        }
        ronde = ronde + 1
    }
    larik hasil = []
    i = 0
    angka total = panjang(w)
    selama i < total {
        angka j = 0
        selama j < 4 {
            hasil = tambah(hasil, w[i][j])
            j = j + 1
        }
        i = i + 1
    }
    kembali hasil
}

// 8. ENKRIPSI / DEKRIPSI SATU BLOK AES

fungsi enkripsiBlokAES(blok, kunciEkspansi, jumlahRonde) {
    larik state = addRoundKey(blok, potongArray(kunciEkspansi, 0, 16))
    angka ronde = 1
    selama ronde < jumlahRonde {
        state = subBytes(state)
        state = shiftRows(state)
        state = mixColumns(state)
        angka mulai = ronde * 16
        larik kunciRonde = potongArray(kunciEkspansi, mulai, 16)
        state = addRoundKey(state, kunciRonde)
        ronde = ronde + 1
    }
    // Ronde terakhir: tanpa mixColumns
    state = subBytes(state)
    state = shiftRows(state)
    angka mulaiTerakhir = jumlahRonde * 16
    larik kunciTerakhir = potongArray(kunciEkspansi, mulaiTerakhir, 16)
    state = addRoundKey(state, kunciTerakhir)
    kembali state
}

fungsi dekripsiBlokAES(blok, kunciEkspansi, jumlahRonde) {
    angka mulaiTerakhir = jumlahRonde * 16
    larik kunciTerakhir = potongArray(kunciEkspansi, mulaiTerakhir, 16)
    larik state = addRoundKey(blok, kunciTerakhir)
    state = invShiftRows(state)
    state = invSubBytes(state)
    angka ronde = jumlahRonde - 1
    selama ronde > 0 {
        angka mulai = ronde * 16
        larik kunciRonde = potongArray(kunciEkspansi, mulai, 16)
        state = addRoundKey(state, kunciRonde)
        state = invMixColumns(state)
        state = invShiftRows(state)
        state = invSubBytes(state)
        ronde = ronde - 1
    }
    larik kunciAwal = potongArray(kunciEkspansi, 0, 16)
    state = addRoundKey(state, kunciAwal)
    kembali state
}

// 9. KONVERSI KUNCI (HELPER)

fungsi kunciKeBytes(kunci) {
    larik kunciBytes = []
    jika tipe(kunci) == "teks" maka
    kunciBytes = teksKeBytes(kunci)
    selain
    angka temp = kunci
    angka i = 0
    selama i < 16 {
        kunciBytes = tambah(kunciBytes, bitAND(temp, 0xff))
        temp = bitKanan(temp, 8)
        i = i + 1
    }
    akhir
    selama panjang(kunciBytes) < 16 {
        kunciBytes = tambah(kunciBytes, 0)
    }
    jika panjang(kunciBytes) > 16 maka
    kunciBytes = potongArray(kunciBytes, 0, 16)
    akhir
    kembali kunciBytes
}

// 10. AES CBC MODE

fungsi enkripsiAES(data, kunci) {
    larik kunciBytes = kunciKeBytes(kunci)
    larik kunciEkspansi = ekspansiKunci(kunciBytes, ITERASI_KUNCI)
    // Generate IV random
    larik iv = []
    angka i = 0
    selama i < IV_PANJANG {
        iv = tambah(iv, lantai(acakAngka(0, 256)))
        i = i + 1
    }
    // Padding data
    teks dataPad = padPKCS7(data, BLOK_UKURAN)
    larik dataBytes = teksKeBytes(dataPad)
    // CBC enkripsi — simpan IV di awal output
    larik hasil = iv
    larik blokSebelumnya = iv
    i = 0
    angka n = panjang(dataBytes)
    selama i < n {
        larik blok = []
        angka j = 0
        selama j < BLOK_UKURAN {
            jika (i + j) < n maka
            blok = tambah(blok, dataBytes[i + j])
            selain
            blok = tambah(blok, 0)
            akhir
            j = j + 1
        }
        larik blokXOR = xorArray(blok, blokSebelumnya)
        larik blokEnc = enkripsiBlokAES(blokXOR, kunciEkspansi, ITERASI_KUNCI)
        j = 0
        selama j < BLOK_UKURAN {
            hasil = tambah(hasil, blokEnc[j])
            j = j + 1
        }
        blokSebelumnya = blokEnc
        i = i + BLOK_UKURAN
    }
    kembali bytesKeHex(hasil)
}

fungsi dekripsiAES(data, kunci) {
    larik dataBytes = hexKeBytes(data)
    // Ambil IV dari 16 byte pertama
    larik iv = []
    angka i = 0
    selama i < IV_PANJANG {
        iv = tambah(iv, dataBytes[i])
        i = i + 1
    }
    // Sisa = ciphertext
    larik ciphertext = []
    i = IV_PANJANG
    angka n = panjang(dataBytes)
    selama i < n {
        ciphertext = tambah(ciphertext, dataBytes[i])
        i = i + 1
    }
    larik kunciBytes = kunciKeBytes(kunci)
    larik kunciEkspansi = ekspansiKunci(kunciBytes, ITERASI_KUNCI)
    // CBC dekripsi
    larik hasil = []
    larik blokSebelumnya = iv
    i = 0
    n = panjang(ciphertext)
    selama i < n {
        larik blok = []
        angka j = 0
        selama j < BLOK_UKURAN {
            jika (i + j) < n maka
            blok = tambah(blok, ciphertext[i + j])
            selain
            blok = tambah(blok, 0)
            akhir
            j = j + 1
        }
        larik blokDec = dekripsiBlokAES(blok, kunciEkspansi, ITERASI_KUNCI)
        larik blokPlain = xorArray(blokDec, blokSebelumnya)
        j = 0
        selama j < BLOK_UKURAN {
            hasil = tambah(hasil, blokPlain[j])
            j = j + 1
        }
        blokSebelumnya = blok
        i = i + BLOK_UKURAN
    }
    teks hasilTeks = bytesKeTeks(hasil)
    kembali unpadPKCS7(hasilTeks)
}

// 11. WRAPPER KOMPATIBILITAS

fungsi buatKunci(nilai) {
    kembali prima(nilai)
}

fungsi enkripsi(data, kunci) {
    kembali enkripsiAES(data, kunci)
}

fungsi dekripsi(data, kunci) {
    kembali dekripsiAES(data, kunci)
}

// Lapisan "cipher prima": ciphertext AES (teks hex) di-XOR per karakter dengan satu
// byte kunci k = angkaPrima & 255. dariKode() hanya menyimpan 8 bit rendah, jadi prima
// >= 256 efektif dilipat ke (angkaPrima & 255): 353 sama dengan 97, 613 sama dengan 101.
//
// Bahaya NUL: karakter hex hanya 0-9 (kode 48-57) dan a-f (kode 97-102). Bila k sama
// dengan salah satu kode itu, karakter tersebut di-XOR jadi byte 0x00 dan teks C
// terpotong diam-diam, sehingga dekripsi gagal (53, 97, 101, 353, 613, ...). Untuk k
// seperti itu keluaran ditulis sebagai hex (2 karakter per byte). Untuk SEMUA k lain
// format lama dipertahankan persis, jadi ciphertext lama tetap bisa didekripsi.
// Fungsi dekripsi tahu k, jadi tidak ada ambiguitas format.

fungsi _kunciXORPrima(angkaPrima) {
    kembali bitAND(angkaPrima, 255)
}

fungsi _xorPrimaButuhHex(k) {
    kembali (k >= 48 dan k <= 57) atau (k >= 97 dan k <= 102)
}

fungsi _terapkanLapisPrima(hexAES, k) {
    larik hasilXOR = xorArray(teksKeBytes(hexAES), [k])
    jika _xorPrimaButuhHex(k) maka
    kembali bytesKeHex(hasilXOR)
    akhir
    kembali bytesKeTeks(hasilXOR)
}

fungsi _lepasLapisPrima(data, k) {
    larik bytes = []
    jika _xorPrimaButuhHex(k) maka
    bytes = hexKeBytes(data)
    selain
    bytes = teksKeBytes(data)
    akhir
    kembali bytesKeTeks(xorArray(bytes, [k]))
}

fungsi enkripsiPakaiCipherPrima(data, kunci, angkaPrima) {
    teks encAES = enkripsiAES(data, kunci)
    kembali _terapkanLapisPrima(encAES, _kunciXORPrima(angkaPrima))
}

fungsi dekripsiPakaiCipherPrima(data, kunci, angkaPrima) {
    teks dataAES = _lepasLapisPrima(data, _kunciXORPrima(angkaPrima))
    kembali dekripsiAES(dataAES, kunci)
}

// enkripsiAngka / dekripsiAngka
//
// Ciphertext AES minimal 32 byte, sedangkan satu angka (double) hanya menampung 53 bit.
// Versi lama menjejalkan ciphertext ke satu angka dan kehilangan hampir semuanya, jadi
// dekripsiAngka(enkripsiAngka(42)) tidak pernah bisa berhasil. Sekarang hasil enkripsi
// berupa TEKS hex (sama seperti enkripsiAES), dan dekripsiAngka mengembalikan angka.
//
// Konversi angka ke teks di Loop hanya 6 digit signifikan untuk pecahan (1/3 menjadi
// "0.333333"), jadi angka dikodekan eksak sebagai m * 2^e (m bilangan bulat, |m| < 2^53).
// Perkalian/pembagian dengan 2 eksak di floating point, sehingga round trip tepat untuk
// semua angka berhingga: bulat, pecahan, negatif, 1e300, 5e-324.
//
// Masukan bukan angka (atau NaN/tak hingga) menghasilkan "". Masukan dekripsi yang bukan
// teks / terlalu pendek / bukan hasil enkripsiAngka menghasilkan 0 (seperti keAngka("abc")).
// Kunci tetap "kunci_angka_default" (rancangan lama): ini penyamaran, bukan perlindungan,
// karena siapa pun yang membaca library bisa mendekripsi.

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
    kembali enkripsiAES(_angkaKeKode(nilai), "kunci_angka_default")
}

fungsi dekripsiAngka(nilai) {
    jika tipe(nilai) != "teks" maka
    kembali 0
    akhir
    jika panjang(nilai) < 64 maka
    kembali 0
    akhir
    kembali _kodeKeAngka(dekripsiAES(nilai, "kunci_angka_default"))
}

// 12. TYPE CHECKING

fungsi isEnkripsi(nilai) {
    kembali tipe(nilai) == "terenkripsi"
}

fungsi isKunciPrima(nilai) {
    kembali tipe(nilai) == "kunci_prima"
}

fungsi isGanda(nilai) {
    kembali tipe(nilai) == "ganda"
}

fungsi isAES(nilai) {
    jika tipe(nilai) == "teks" maka
    angka n = panjang(nilai)
    kembali n >= 32
    akhir
    kembali salah
}

// 13. ENKRIPSI FILE

fungsi enkripsiFileAES(namaFile, kunci) {
    // Enkripsi isi namaFile, tulis ke namaFile + ".aes".
    // Sukses: kembalikan path hasil. Gagal: kembalikan "" (tanpa pesan).
    // Gagal = file tidak ada atau kosong (bacaFileTeks tidak bisa membedakan keduanya).
    teks isiFile = bacaFileTeks(namaFile)
    jika isiFile == "" maka
        kembali ""
    akhir
    teks terenkripsi = enkripsiAES(isiFile, kunci)
    teks pathHasil = namaFile + ".aes"
    tulisFileTeks(pathHasil, terenkripsi)
    kembali pathHasil
}

fungsi dekripsiFileAES(namaFile, kunci) {
    // Dekripsi isi namaFile, tulis ke namaFile tanpa ".aes".
    // Sukses: kembalikan path hasil. Gagal: kembalikan "" (tanpa pesan).
    // Gagal = nama bukan berakhiran .aes (supaya file asli tidak tertimpa),
    // atau isi file lebih pendek dari IV + 1 blok (64 karakter hex).
    teks pathHasil = ganti(namaFile, ".aes", "")
    jika pathHasil == namaFile atau pathHasil == "" maka
        kembali ""
    akhir
    teks isiFile = bacaFileTeks(namaFile)
    jika panjang(isiFile) < 64 maka
        kembali ""
    akhir
    teks asli = dekripsiAES(isiFile, kunci)
    tulisFileTeks(pathHasil, asli)
    kembali pathHasil
}

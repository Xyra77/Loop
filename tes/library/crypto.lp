// tes/library/crypto.lp — kripto native + Library/crypto.lp
// Vektor standar: RFC 4231 (HMAC-SHA256), RFC 7914 bagian 11 (PBKDF2-HMAC-SHA256),
// McGrew & Viega "GCM spec" Test Case 13/14/15 (AES-256-GCM).
// Jalankan dari akar repo: LOOP_STDLIB=Library ./seed/loop tes/library/crypto.lp
pakai pastikan
pakai crypto
pakai io

milik dir = "/tmp/loop_tes_crypto_" + _sistemPID()
jalankan("mkdir -p " + dir)

cetak "▸ HMAC-SHA256 (RFC 4231)"
pastikanSama(keHex(hmacSha256(dariHex("0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b"), "Hi There")),
    "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7", "RFC 4231 kasus 1")
pastikanSama(keHex(hmacSha256("Jefe", "what do ya want for nothing?")),
    "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843", "RFC 4231 kasus 2")
fungsi ulangByte(nilai, jumlah) {
    larik a = []
    angka i = 0
    selama i < jumlah {
        a = tambah(a, nilai)
        i = i + 1
    }
    kembali keBita(a)
}
pastikanSama(keHex(hmacSha256(ulangByte(170, 20), ulangByte(221, 50))),
    "773ea91e36800e46854db8ebd09181a72959098b3ef8c122d9635514ced565fe", "RFC 4231 kasus 3")
pastikanSama(keHex(hmacSha256(ulangByte(170, 131), "Test Using Larger Than Block-Size Key - Hash Key First")),
    "60e431591ee0b67f0d8a26aacbf5b77f8e0bc6213728c5140546040f0ee37f54", "RFC 4231 kasus 6 (kunci > ukuran blok)")
coba {
    hmacSha256("", "data")
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatNilai", "HMAC dengan kunci kosong ditolak")
}

cetak "▸ PBKDF2-HMAC-SHA256 (RFC 7914 bagian 11; 32 byte pertama dari 64)"
pastikanSama(keHex(turunkanKunci("passwd", "salt", 1)),
    "55ac046e56e3089fec1691c22544b605f94185216dde0465e68b9d57c20dacbc", "passwd/salt c=1")
pastikanSama(keHex(turunkanKunci("Password", "NaCl", 80000)),
    "4ddcd8f60b98be21830cee5ef22701f9641a4418d04c0414aeff08876b34ab56", "Password/NaCl c=80000")
pastikanSama(panjangBita(turunkanKunci("x", "garam", 10)), 32, "keluaran 32 byte")
coba {
    turunkanKunci("", "garam", 10)
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatNilai", "sandi kosong ditolak")
}
coba {
    turunkanKunci("sandi", "garam", 0)
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatNilai", "iterasi 0 ditolak")
}

cetak "▸ AES-256-GCM (McGrew & Viega, kasus 13-15, tanpa AAD)"
milik nol32 = dariHex("0000000000000000000000000000000000000000000000000000000000000000")
pastikanSama(keHex(dekripsiGCM(nol32, dariHex("000000000000000000000000" + "530f8afbc74536b9a963b4f1c4cb738b"))),
    "", "kasus 13: plaintext kosong, tag cocok")
pastikanSama(keHex(dekripsiGCM(nol32, dariHex("000000000000000000000000" + "cea7403d4d606b6e074ec5d3baf39d18" + "d0d1c8a799996bf0265b98b5d48ab919"))),
    "00000000000000000000000000000000", "kasus 14: 16 byte nol")
milik k15 = dariHex("feffe9928665731c6d6a8f9467308308feffe9928665731c6d6a8f9467308308")
milik p15 = dariHex("cafebabefacedbaddecaf888" +
    "522dc1f099567d07f47f37a32a84427d643a8cdcbfe5c0c97598a2bd2555d1aa8cb08e48590dbb3da7b08b1056828838c5f61e6393ba7a0abcc9f662898015ad" +
    "b094dac5d93471bdec1a502270e3cc6c")
milik asli15 = "d9313225f88406e5a55909c5aff5269a86a7a9531534f7da2e4c303d8a318a721c3c0c95956809532fcf0e2449a6b525b16aedf5aa0de657ba637b391aafd255"
pastikanSama(keHex(dekripsiGCM(k15, p15)), asli15, "kasus 15: AES-256, 64 byte")

cetak "▸ GCM: tag diubah, kunci salah, paket pendek -> galat, bukan plaintext sampah"
milik n15 = panjangBita(p15)
milik ctDiubah = sambungBita(potongBita(p15, 0, 20), sambungBita(keBita([255]), potongBita(p15, 21, n15)))
coba {
    dekripsiGCM(k15, ctDiubah)
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatKunci", "ciphertext diubah terdeteksi")
}
milik tagDiubah = sambungBita(potongBita(p15, 0, n15 - 1), keBita([0]))
coba {
    dekripsiGCM(k15, tagDiubah)
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatKunci", "tag diubah terdeteksi")
}
coba {
    dekripsiGCM(nol32, p15)
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatKunci", "kunci salah ditolak")
}
coba {
    dekripsiGCM(k15, keBita([1, 2, 3]))
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatNilai", "paket terlalu pendek")
}
coba {
    enkripsiGCM(keBita([1, 2, 3]), "data")
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatNilai", "kunci bukan 32 byte ditolak")
}
milik paketA = enkripsiGCM(k15, "pesan yang sama")
milik paketB = enkripsiGCM(k15, "pesan yang sama")
pastikan(paketA != paketB, "nonce acak: dua enkripsi pesan sama berbeda")
pastikanSama(panjangBita(paketA), 12 + 15 + 16, "panjang paket = nonce + data + tag")
pastikanSama(keTeks(dekripsiGCM(k15, paketA)), "pesan yang sama", "round-trip GCM")

cetak "▸ samaWaktuKonstan dan RNG"
pastikan(samaWaktuKonstan(keBita([1, 2, 3]), keBita([1, 2, 3])), "sama")
pastikan(samaWaktuKonstan(keBita([1, 2, 3]), keBita([1, 2, 4])) == salah, "beda isi")
pastikan(samaWaktuKonstan(keBita([1, 2, 3]), keBita([1, 2])) == salah, "beda panjang")
pastikanSama(panjangBita(acakAman(0)), 0, "acakAman(0)")
pastikanSama(panjangBita(acakAman(32)), 32, "acakAman(32) panjang benar")
pastikan(acakAman(16) != acakAman(16), "dua keluaran acakAman berbeda")
coba {
    acakAman(-1)
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatNilai", "acakAman(-1)")
}

cetak "▸ enkripsiBerkas / dekripsiBerkas: round-trip berbagai ukuran"
larik ukuran = [0, 1, 16, 4096, 4097, 1048576]
angka i = 0
selama i < panjang(ukuran) {
    milik asli = acakAman(ukuran[i])
    milik p = dir + "/r" + ukuran[i]
    tulisBita(p, asli)
    milik hasilEnc = enkripsiBerkas(p, "sandi saya")
    pastikanSama(hasilEnc, p + ".aes", "enkripsiBerkas mengembalikan path hasil (" + ukuran[i] + " byte)")
    hapusBerkas(p)
    milik hasilDec = dekripsiBerkas(p + ".aes", "sandi saya")
    pastikanSama(hasilDec, p, "dekripsiBerkas mengembalikan path hasil")
    pastikan(bacaBita(p) == asli, "round-trip identik " + ukuran[i] + " byte")
    i = i + 1
}
milik denganNol = keBita([0, 0, 1, 0, 2, 0, 0])
tulisBita(dir + "/nol", denganNol)
enkripsiBerkas(dir + "/nol", "pw")
hapusBerkas(dir + "/nol")
dekripsiBerkas(dir + "/nol.aes", "pw")
pastikan(bacaBita(dir + "/nol") == denganNol, "berkas berisi 0x00 round-trip identik")

cetak "▸ kegagalan: tujuan lama tidak berubah, jenis galat tepat"
tulisFileTeks(dir + "/rahasia", "ISI-ASLI")
enkripsiBerkas(dir + "/rahasia", "sandi benar")
tulisFileTeks(dir + "/rahasia", "TUJUAN-LAMA-JANGAN-BERUBAH")
coba {
    dekripsiBerkas(dir + "/rahasia.aes", "sandi SALAH")
    gagalTes("seharusnya melempar GalatKunci")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatKunci", "sandi salah ditolak")
}
pastikanSama(bacaFileTeks(dir + "/rahasia"), "TUJUAN-LAMA-JANGAN-BERUBAH", "sandi salah tidak menimpa tujuan")
milik aes = bacaBita(dir + "/rahasia.aes")
milik aesRusak = sambungBita(potongBita(aes, 0, 40), sambungBita(keBita([(aes[40] + 1) % 256]), potongBita(aes, 41, panjangBita(aes))))
tulisBita(dir + "/rusak.aes", aesRusak)
coba {
    dekripsiBerkas(dir + "/rusak.aes", "sandi benar")
    gagalTes("seharusnya melempar GalatKunci")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatKunci", "berkas diubah terdeteksi")
}
coba {
    dekripsiBerkas(dir + "/tidak-ada.aes", "pw")
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatBerkas", "berkas tidak ada")
}
coba {
    enkripsiBerkas(dir + "/tidak-ada", "pw")
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatBerkas", "enkripsi berkas yang tidak ada")
}
coba {
    dekripsiBerkas(dir + "/rahasia", "pw")
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatNilai", "nama bukan .aes")
}
tulisFileTeks(dir + "/pendek.aes", "x")
coba {
    dekripsiBerkas(dir + "/pendek.aes", "pw")
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatBerkas", "bukan hasil enkripsiBerkas")
}
coba {
    enkripsiBerkas(dir + "/rahasia", "")
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatNilai", "sandi kosong ditolak")
}
jalankan("mkdir " + dir + "/blok.aes")
coba {
    enkripsiBerkas(dir + "/blok", "pw")
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatBerkas", "tujuan tidak bisa ditulis")
}
pastikan(buatKunci("sandi panjang sekali lebih dari enam belas karakter A", keBita("garam-garam-1234")) !=
    buatKunci("sandi panjang sekali lebih dari enam belas karakter B", keBita("garam-garam-1234")), "sandi tidak lagi dipotong ke 16 karakter")

cetak "▸ bukti: IV/nonce tidak sama antar enkripsi"
tulisFileTeks(dir + "/iv", "data")
enkripsiBerkas(dir + "/iv", "pw")
milik e1 = bacaBita(dir + "/iv.aes")
enkripsiBerkas(dir + "/iv", "pw")
milik e2 = bacaBita(dir + "/iv.aes")
pastikan(potongBita(e1, 0, 28) != potongBita(e2, 0, 28), "garam+nonce berbeda tiap enkripsi")

cetak "▸ enkripsiData / dekripsiData / enkripsi / dekripsi"
milik pk = enkripsiData(keBita([0, 1, 2, 0]), "pw")
pastikan(dekripsiData(pk, "pw") == keBita([0, 1, 2, 0]), "enkripsiData bolak-balik dengan byte 0x00")
pastikanSama(dekripsi(enkripsi("teks rahasia", "pw"), "pw"), "teks rahasia", "enkripsi/dekripsi teks")
pastikanSama(dekripsiAngka(enkripsiAngka(3.14159)), 3.14159, "enkripsiAngka desimal tepat")
pastikanSama(dekripsiAngka(enkripsiAngka(-42)), -42, "enkripsiAngka negatif")
pastikanSama(dekripsiAngka("bukan hex"), 0, "dekripsiAngka masukan salah -> 0 (kontrak lama)")
milik cp = enkripsiPakaiCipherPrima("data uji", "pw", 53)
pastikanSama(dekripsiPakaiCipherPrima(cp, "pw", 53), "data uji", "cipher prima 53")
pastikanSama(dekripsiPakaiCipherPrima(enkripsiPakaiCipherPrima("data uji", "pw", 97), "pw", 97), "data uji", "cipher prima 97")
pastikanSama(dekripsiPakaiCipherPrima(enkripsiPakaiCipherPrima("data uji", "pw", 101), "pw", 101), "data uji", "cipher prima 101")

cetak "▸ migrasi berkas .aes format lama"
salinFile("tes/fixtures/lama.txt.aes", dir + "/lama.txt.aes")
coba {
    dekripsiBerkas(dir + "/lama.txt.aes", "kunciLama123")
    gagalTes("format lama tidak boleh lolos dekripsiBerkas")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatKunci", "format lama ditolak dekripsiBerkas dengan petunjuk migrasi")
    pastikan(cariTeks(pesanGalat(e), "dekripsiBerkasV0") >= 0, "pesan menunjuk dekripsiBerkasV0")
}
dekripsiBerkasV0(dir + "/lama.txt.aes", "kunciLama123")
pastikanSama(bacaFileTeks(dir + "/lama.txt"), "data lama dari format v0", "dekripsiBerkasV0 membaca berkas lama")
coba {
    dekripsiBerkasV0(dir + "/lama.txt.aes", "sandi salah")
    gagalTes("seharusnya melempar")
} tangkap {
    pastikanSama(jenisGalat(e), "GalatKunci", "dekripsiBerkasV0 sandi salah")
}
pastikanSama(enkripsiFileAES(dir + "/rahasia", "pw"), dir + "/rahasia.aes", "alias lama enkripsiFileAES")

jalankan("rm -rf " + dir)
selesaiTes("CRYPTO")

// tes/contoh/aes_murni.lp — blok AES murni-Loop cocok bit demi bit dengan FIPS-197
// Vektor: FIPS-197 Appendix B dan Appendix C.1 (AES-128).
// Jalankan dari akar repo: LOOP_STDLIB=Library ./seed/loop tes/contoh/aes_murni.lp
pakai pastikan
bacaFile("contoh/aes_murni.lp")

fungsi uji(kunciHex, teksHex, cipherHex, nama) {
    larik ke = ekspansiKunci(hexKeBytes(kunciHex), 10)
    larik ct = enkripsiBlokAES(hexKeBytes(teksHex), ke, 10)
    pastikanSama(bytesKeHex(ct), cipherHex, nama + ": enkripsi")
    pastikanSama(bytesKeHex(dekripsiBlokAES(ct, ke, 10)), teksHex, nama + ": dekripsi")
}

uji("2b7e151628aed2a6abf7158809cf4f3c", "3243f6a8885a308d313198a2e0370734",
    "3925841d02dc09fbdc118597196a0b32", "FIPS-197 Appendix B")
uji("000102030405060708090a0b0c0d0e0f", "00112233445566778899aabbccddeeff",
    "69c4e0d86a7b0430d8cdb78070b4c55a", "FIPS-197 Appendix C.1")
selesaiTes("AES MURNI (FIPS-197)")

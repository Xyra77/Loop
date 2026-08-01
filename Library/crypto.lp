// Library/crypto.lp — wrapper level tinggi di atas builtin rendah (prima, acak, ganda)
// Builtin rendah TIDAK diubah, cuma dibungkus jadi nama/API yang lebih jelas maknanya.

fungsi buatKunci(nilai) {
    // Bikin kunci prima dari satu atau lebih angka.
    // Contoh: buatKunci(1021)
    kembali prima(nilai)
}

fungsi enkripsi(data, kunci) {
    // data: teks yang mau dienkripsi
    // kunci: hasil buatKunci(), ATAU passphrase teks biasa
    kembali acak(data, kunci)
}

fungsi enkripsiPakaiCipherPrima(data, kunci, angkaPrima) {
    // Lapisan XOR tambahan pakai angka prima eksplisit (arg ke-3 di acak()).
    // angkaPrima HARUS prima, kalau enggak builtin acak() bakal error sendiri.
    kembali acak(data, kunci, angkaPrima)
}

fungsi enkripsiAngka(nilai) {
    // Enkripsi sederhana khusus nilai numerik (pakai builtin ganda()).
    kembali ganda(nilai)
}

fungsi isEnkripsi(nilai) {
    kembali tipe(nilai) == "terenkripsi"
}

fungsi isKunciPrima(nilai) {
    kembali tipe(nilai) == "kunci_prima"
}

fungsi isGanda(nilai) {
    kembali tipe(nilai) == "ganda"
}

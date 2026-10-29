// Library/net.lp
// Pembungkus tipis di atas builtin jaringan (sambung, tlsSambung, kirim, terima,
// terimaBita, tutupKoneksi, ambil, ambilBita, telusuri). Semua kegagalan
// (host tidak ketemu, port tertutup, timeout, TLS gagal) dilempar sebagai
// GalatJaringan; argumen yang salah sebagai GalatNilai/GalatTipe. Library ini
// tidak mencetak apa pun.
//
// Timeout bawaan 10 detik (ubah dengan env LOOP_TIMEOUT_JARINGAN atau pakai
// varian *Timeout di bawah). Sertifikat TLS diverifikasi; untuk server uji
// dengan sertifikat swa-tanda-tangan set LOOP_TLS_TIDAK_AMAN=1 (jangan di produksi).

// bukaKoneksi(host, port) -> koneksi
// Galat: GalatJaringan, GalatNilai (port di luar 1..65535)
fungsi bukaKoneksi(host, port) {
    kembali sambung(host, port)
}

// bukaKoneksiTimeout(host, port, detik) -> koneksi
fungsi bukaKoneksiTimeout(host, port, detik) {
    kembali sambung(host, port, detik)
}

// bukaKoneksiAman(host, port) -> koneksi TLS (sertifikat diverifikasi)
fungsi bukaKoneksiAman(host, port) {
    kembali tlsSambung(host, port)
}

fungsi bukaKoneksiAmanTimeout(host, port, detik) {
    kembali tlsSambung(host, port, detik)
}

// kirimTeks(koneksi, isi) -> angka byte terkirim. Galat: GalatJaringan
fungsi kirimTeks(koneksi, isi) {
    kembali kirim(koneksi, isi)
}

// kirimBita(koneksi, bita) -> angka byte terkirim
fungsi kirimBita(koneksi, data) {
    kembali kirim(koneksi, data)
}

// terimaTeks(koneksi, maks) -> teks ("" bila lawan menutup koneksi)
// Galat: GalatJaringan (termasuk timeout), GalatNilai (data berisi 0x00)
fungsi terimaTeks(koneksi, maks) {
    kembali terima(koneksi, maks)
}

// terimaData(koneksi, maks) -> bita (kosong bila lawan menutup koneksi)
fungsi terimaData(koneksi, maks) {
    kembali terimaBita(koneksi, maks)
}

// tutup(koneksi) -> benar. Aman dipanggil dua kali.
fungsi tutup(koneksi) {
    tutupKoneksi(koneksi)
    kembali benar
}

// httpGet(url) -> teks body. Galat: GalatJaringan, GalatNilai (URL salah)
fungsi httpGet(url) {
    kembali ambil(url)
}

fungsi httpGetTimeout(url, detik) {
    kembali ambil(url, detik)
}

// httpGetBita(url) -> bita (untuk body biner)
fungsi httpGetBita(url) {
    kembali ambilBita(url)
}

fungsi crawlSitus(url) {
    kembali telusuri(url)
}

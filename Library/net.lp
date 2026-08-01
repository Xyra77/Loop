// Library/net.lp — wrapper level tinggi di atas builtin rendah (sambung, tlsSambung,
// kirim, terima, tutupKoneksi, ambil, telusuri). Builtin rendah TIDAK diubah.

fungsi bukaKoneksi(host, port) {
    kembali sambung(host, port)
}

fungsi bukaKoneksiAman(host, port) {
    // Versi TLS dari bukaKoneksi().
    kembali tlsSambung(host, port)
}

fungsi kirimTeks(koneksi, isi) {
    kembali kirim(koneksi, isi)
}

fungsi terimaTeks(koneksi, maks) {
    kembali terima(koneksi, maks)
}

fungsi tutup(koneksi) {
    tutupKoneksi(koneksi)
    kembali benar
}

fungsi httpGet(url) {
    kembali ambil(url)
}

fungsi crawlSitus(url) {
    kembali telusuri(url)
}

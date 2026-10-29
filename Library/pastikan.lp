// Library/pastikan.lp
// Alat tes kecil. Pakai dengan: pakai pastikan
//
//   pastikan(kondisi, nama)         gagal bila kondisi bukan benar
//   pastikanSama(a, b, nama)        gagal bila a != b (pesan memuat kedua nilai)
//   gagalTes(pesan)                 gagal seketika (melempar GagalTes)
//   selesaiTes(nama)                cetak baris "SEMUA TES <nama> SELESAI"
//
// Tes yang gagal melempar galat berjenis GagalTes; bila tidak ditangkap proses
// keluar dengan kode 1, sehingga CI (tes/jalankan_semua.sh) ikut gagal.
//
// Pola jalur gagal (harus melempar galat dengan jenis tertentu):
//
//   coba {
//       dekripsiBerkas("data.txt.aes", "sandi salah")
//       gagalTes("seharusnya melempar GalatKunci")
//   } tangkap {
//       pastikanSama(jenisGalat(e), "GalatKunci", "sandi salah ditolak")
//   }
//
// Bila coba tidak melempar, gagalTes melempar GagalTes dan pastikanSama gagal
// dengan pesan yang jelas. (Loop belum punya fungsi anonim, jadi pola coba
// ditulis langsung di tes.)

angka _jumlahPemeriksaan = 0

fungsi pastikan(kondisi, nama) {
    _jumlahPemeriksaan = _jumlahPemeriksaan + 1
    jika kondisi != benar maka
        lempar galat("GagalTes", "GAGAL: " + nama)
    akhir
    cetak("  ok  " + nama)
}

fungsi pastikanSama(a, b, nama) {
    _jumlahPemeriksaan = _jumlahPemeriksaan + 1
    jika a != b maka
        lempar galat("GagalTes", "GAGAL: " + nama + " (dapat [" + a + "], seharusnya [" + b + "])")
    akhir
    cetak("  ok  " + nama)
}

fungsi gagalTes(pesan) {
    lempar galat("GagalTes", pesan)
}

fungsi selesaiTes(nama) {
    cetak("SEMUA TES " + nama + " SELESAI (" + _jumlahPemeriksaan + " pemeriksaan)")
}

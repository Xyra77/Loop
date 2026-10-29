// spesifikasi/sintaks.lp
// DESAIN FUTURISTIK - JANGAN DIJALANKAN!

fungsi utama() {
    x = 10
    arr = [1, 2, "tiga"]
    
    jika x > 5 maka
        cetak "besar"
    akhir

    selama x > 0 {
        x = x - 1
    }

    hasil = tambah(2, 3)
    kembali hasil
}

fungsi tambah(a, b) {
    kembali a + b
}
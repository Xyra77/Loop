// Pustaka Penghubung Sistem & Utilitas Rendah-Tinggi
// Versi 1.0

// 1. SISTEM & PROSES
fungsi sistem(perintah) {
    // Jalankan perintah shell, kembalikan kode keluar
    kembali _sistemJalankan(perintah)
}

fungsi pid() {
    // ID proses saat ini
    kembali _sistemPID()
}

fungsi keluar(kode) {
    // Hentikan proses dengan kode keluar
    _sistemKeluar(kode)
}

fungsi argumen() {
    // Larik argumen baris perintah (tanpa nama program)
    kembali _sistemArgumen()
}

fungsi dirKerja() {
    // Direktori kerja saat ini
    kembali _sistemDirKerja()
}

fungsi pindahDir(path) {
    // Pindah direktori kerja
    kembali _sistemPindahDir(path)
}

// 2. LINGKUNGAN

fungsi ambilEnv(kunci) {
    // Ambil nilai variabel lingkungan, kosong jika tidak ada
    kembali _sistemEnvGet(kunci)
}

fungsi aturEnv(kunci, nilai) {
    // Set variabel lingkungan, kembalikan benar jika berhasil
    kembali _sistemEnvSet(kunci, nilai)
}

fungsi hapusEnv(kunci) {
    // Hapus variabel lingkungan
    kembali _sistemEnvHapus(kunci)
}

fungsi envSemua() {
    // Dapatkan semua variabel lingkungan sebagai larik string "KEY=value"
    kembali _sistemEnvSemua()
}

// 3. WAKTU

fungsi waktuUnix() {
    // Detik sejak 1970-01-01 UTC
    kembali waktuSekarang()
}

fungsi waktuMilis() {
    // Milidetik sejak epoch
    kembali waktumilisdetik()
}

// 4. INFORMASI SISTEM

fungsi namaOS() {
    // Nama sistem operasi (selalu "linux" untuk saat ini)
    kembali _sistemNamaOS()
}

fungsi arsitektur() {
    // Arsitektur CPU (misal "x86_64")
    kembali _sistemArsitektur()
}

fungsi jumlahCPU() {
    // Jumlah core CPU yang tersedia
    kembali _sistemJumlahCPU()
}

// 5. BITWISE (integer 64-bit)

fungsi bitDan(a, b) {
    kembali bitAND(a, b)
}

fungsi bitAtau(a, b) {
    kembali bitOR(a, b)
}

fungsi bitXor(a, b) {
    kembali bitXOR(a, b)
}

fungsi bitGeserKiri(a, b) {
    kembali bitKiri(a, b)
}

fungsi bitGeserKanan(a, b) {
    kembali bitKanan(a, b)
}

fungsi bitBukan(a) {
    kembali bitNOT(a)
}

// 6. KONVERSI TIPE

// 7. UTILITAS MATEMATIKA

fungsi logNatural(x) {
    // Logaritma natural (basis e)
    kembali log(x)
}

// 8. EKSEKUSI AMAN

fungsi jalankanAman(f) {
    // Jalankan fungsi dan tangkap galat.
    // Kembali: [benar, null] bila sukses, atau [salah, e] dengan e objek galat
    // (pakai jenisGalat(e), pesanGalat(e)).
    coba {
        f()
        kembali [benar, null]
    } tangkap {
        kembali [salah, e]
    }
}

// 9. EKSPOR


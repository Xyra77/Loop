// tes/sistem.lp — tes semua fungsi builtin kategori meta/sistem
// Jalankan: ./loop tes/sistem.lp
// CATATAN: _sistemKeluar() SENGAJA tidak dites di sini karena akan menghentikan
// proses interpreter itu sendiri (exit()). Coba manual kalau perlu:
//   _sistemKeluar(0)

cetak "▸ Sistem: env"
cetak "_sistemEnvSet(\"TES_LOOP_VAR\", \"halo\"): " + keTeks(_sistemEnvSet("TES_LOOP_VAR", "halo"))
cetak "_sistemEnvGet(\"TES_LOOP_VAR\"): " + _sistemEnvGet("TES_LOOP_VAR")
milik semua_env = _sistemEnvSemua()
cetak "_sistemEnvSemua() jumlah variabel env: " + keTeks(panjang(semua_env))
cetak "_sistemEnvHapus(\"TES_LOOP_VAR\"): " + keTeks(_sistemEnvHapus("TES_LOOP_VAR"))
cetak "_sistemEnvGet(\"TES_LOOP_VAR\") setelah dihapus (harus kosong): [" + _sistemEnvGet("TES_LOOP_VAR") + "]"

cetak ""
cetak "▸ Sistem: direktori & path"
milik dir_awal = _sistemDirKerja()
cetak "_sistemDirKerja(): " + dir_awal
cetak "_sistemPathSep(): " + _sistemPathSep()
cetak "_sistemPindahDir(\"/tmp\"): " + keTeks(_sistemPindahDir("/tmp"))
cetak "_sistemDirKerja() setelah pindah: " + _sistemDirKerja()
_sistemPindahDir(dir_awal)
cetak "_sistemDirKerja() setelah balik lagi: " + _sistemDirKerja()

cetak ""
cetak "▸ Sistem: info OS & argumen"
cetak "_sistemNamaOS(): " + _sistemNamaOS()
cetak "_sistemArsitektur(): " + _sistemArsitektur()
cetak "_sistemJumlahCPU(): " + keTeks(_sistemJumlahCPU())
cetak "_sistemPID(): " + keTeks(_sistemPID())
milik argv = _sistemArgumen()
cetak "_sistemArgumen() (argv dari OS): " + keTeks(argv)

cetak ""
cetak "▸ Sistem: jalankan perintah shell"
milik kode = _sistemJalankan("echo halo dari _sistemJalankan > /tmp/tes_loop_sistem.txt")
cetak "_sistemJalankan() exit code: " + keTeks(kode)
cetak bacaFileTeks("/tmp/tes_loop_sistem.txt")

cetak "▸ SEMUA TES SISTEM SELESAI (kecuali _sistemKeluar, sengaja diskip)"

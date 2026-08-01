// tes/matematika.lp — tes semua fungsi builtin kategori matematika & bitwise
// Jalankan: ./loop tes/matematika.lp

cetak "▸ Matematika: pembulatan"
cetak "bulatkan(3.4): " + keTeks(bulatkan(3.4))
cetak "bulatkan(3.6): " + keTeks(bulatkan(3.6))
cetak "lantai(3.9) (floor): " + keTeks(lantai(3.9))
cetak "langit(3.1) (ceil): " + keTeks(langit(3.1))
cetak "absolut(-7): " + keTeks(absolut(-7))

cetak ""
cetak "▸ Matematika: trig & log & akar"
cetak "akarKuadrat(16): " + keTeks(akarKuadrat(16))
cetak "log(akarKuadrat(1) + 1) (ln(2.718...) ~ mendekati 1, cek gak error): " + keTeks(log(2.718281828))
cetak "log(8, 2) (log basis 2 dari 8): " + keTeks(log(8, 2))
cetak "kosinus(0): " + keTeks(kosinus(0))
cetak "sinus(0): " + keTeks(sinus(0))
cetak "PI(): " + keTeks(PI())

cetak ""
cetak "▸ Matematika: angka acak"
milik r1 = acakAngka()
milik r2 = acakAngka(10, 20)
cetak "acakAngka() antara 0-1? " + keTeks(r1 >= 0 dan r1 < 1)
cetak "acakAngka(10, 20) antara 10-20? " + keTeks(r2 >= 10 dan r2 < 20)

cetak ""
cetak "▸ Bitwise"
cetak "bitAND(6, 3): " + keTeks(bitAND(6, 3))
cetak "bitOR(6, 3): " + keTeks(bitOR(6, 3))
cetak "bitXOR(6, 3): " + keTeks(bitXOR(6, 3))
cetak "bitKiri(1, 4) (1 << 4): " + keTeks(bitKiri(1, 4))
cetak "bitKanan(16, 2) (16 >> 2): " + keTeks(bitKanan(16, 2))
cetak "bitNOT(0): " + keTeks(bitNOT(0))

cetak "▸ SEMUA TES MATEMATIKA & BITWISE SELESAI"

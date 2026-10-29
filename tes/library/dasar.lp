// tes/library/dasar.lp — Library/dasar.lp dan Library/bridge.lp tetap termuat dan bekerja
// Jalankan dari akar repo: LOOP_STDLIB=Library ./seed/loop tes/library/dasar.lp
pakai pastikan
pakai dasar
pakai bridge

pastikan(adalahPrima(13), "adalahPrima(13)")
pastikan(adalahPrima(15) == salah, "adalahPrima(15)")
pastikanSama(rataRata([2, 4, 6]), 4, "rataRata")
pastikanSama(gantiSemua("a-b-c", "-", "+"), "a+b+c", "gantiSemua")
pastikanSama(floor(2.7), 2, "floor")
pastikanSama(ceil(2.1), 3, "ceil")
pastikanSama(pid() > 0, benar, "bridge: pid()")
pastikanSama(bitXor(6, 3), 5, "bridge: bitXor")
selesaiTes("LIBRARY DASAR")

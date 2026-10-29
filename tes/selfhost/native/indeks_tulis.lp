// Hanya native: assignment berindeks (arr[i] = x) belum ada di interpreter loop.c.
larik arr = [1, 2, 3]
arr[1] = 20
cetak "ok: #{arr[1]}"
coba {
    arr[7] = 1
} tangkap {
    cetak "tulis di luar batas: #{jenisGalat(e)}"
}
angka neg = 0 - 2
coba {
    arr[neg] = 1
} tangkap {
    cetak "tulis indeks negatif: #{jenisGalat(e)}"
}
cetak "array utuh: #{arr[0]} #{arr[1]} #{arr[2]} panjang #{panjang(arr)}"

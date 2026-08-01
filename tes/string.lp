// tes/string.lp — tes semua fungsi builtin kategori string
// Jalankan: ./loop tes/string.lp

cetak "▸ String: konversi tipe"
cetak "panjang(\"halo\"): " + keTeks(panjang("halo"))
cetak "angkaTeks(42): " + angkaTeks(42)
cetak "teksAngka(\"42\"): " + keTeks(teksAngka("42"))
cetak "keAngka(\"3.5\"): " + keTeks(keAngka("3.5"))
cetak "keDesimal(\"3.5\"): " + keTeks(keDesimal("3.5"))
cetak "keTeks(123): " + keTeks(123)

cetak ""
cetak "▸ String: karakter <-> kode"
cetak "kodeKarakter(\"A\"): " + keTeks(kodeKarakter("A"))
cetak "dariKode(65): " + dariKode(65)
cetak "karakter(\"halo\", 1): " + karakter("halo", 1)

cetak ""
cetak "▸ String: potong / cari / ganti"
cetak "potong(\"halo dunia\", 5): " + potong("halo dunia", 5)
cetak "potong(\"halo dunia\", 0, 4): " + potong("halo dunia", 0, 4)
cetak "cariTeks(\"halo dunia\", \"dunia\"): " + keTeks(cariTeks("halo dunia", "dunia"))
cetak "gantiTeks(\"halo dunia\", \"dunia\", \"loop\"): " + gantiTeks("halo dunia", "dunia", "loop")
cetak "ganti(\"halo dunia\", \"halo\", \"hai\") (alias gantiTeks): " + ganti("halo dunia", "halo", "hai")
cetak "terakhirIndexDari(\"abcabc\", \"a\"): " + keTeks(terakhirIndexDari("abcabc", "a"))

cetak ""
cetak "▸ String: ubah huruf"
cetak "hurufBesar(\"halo\"): " + hurufBesar("halo")
cetak "naik(\"halo\") (alias hurufBesar): " + naik("halo")
cetak "hurufKecil(\"HALO\"): " + hurufKecil("HALO")
cetak "turunkan(\"HALO\") (alias hurufKecil): " + turunkan("HALO")

cetak ""
cetak "▸ String: pisah / gabung / trim"
milik potongan = pisah("a,b,c", ",")
cetak "pisah(\"a,b,c\", \",\"): " + keTeks(potongan)
cetak "gabungTeks([\"a\",\"b\",\"c\"], \"-\"): " + gabungTeks(["a", "b", "c"], "-")
cetak "trimTeks(\"  spasi  \"): [" + trimTeks("  spasi  ") + "]"
cetak "trim(\"  spasi  \") (alias trimTeks): [" + trim("  spasi  ") + "]"

cetak ""
cetak "▸ String: prefix / suffix"
cetak "mulaiDengan(\"loop-lang\", \"loop\"): " + keTeks(mulaiDengan("loop-lang", "loop"))
cetak "akhiriDengan(\"loop-lang\", \"lang\"): " + keTeks(akhiriDengan("loop-lang", "lang"))

cetak ""
cetak "▸ String: regex"
cetak "regexCocok(\"halo123\", \"[0-9]+\"): " + keTeks(regexCocok("halo123", "[0-9]+"))
cetak "regexGanti(\"halo123dunia\", \"[0-9]+\", \"-\"): " + regexGanti("halo123dunia", "[0-9]+", "-")

cetak ""
cetak "▸ String: penggabungan (operator + / tambah())"
cetak "tambah(\"halo \", \"dunia\"): " + tambah("halo ", "dunia")
cetak "\"halo\" + \" \" + \"dunia\": " + "halo" + " " + "dunia"

cetak "▸ SEMUA TES STRING SELESAI"

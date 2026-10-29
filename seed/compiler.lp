fungsi adalah_angka(c) {
    angka k = kodeKarakter(c)
    kembali k >= 48 dan k <= 57
}

fungsi adalah_huruf(c) {
    angka k = kodeKarakter(c)
    jika k >= 97 dan k <= 122 maka
        kembali 1
    akhir
    jika k >= 65 dan k <= 90 maka
        kembali 1
    akhir
    jika c == "_" maka
        kembali 1
    akhir
    kembali 0
}

fungsi adalah_huruf_angka(c) {
    kembali adalah_huruf(c) atau adalah_angka(c)
}

fungsi adalah_spasi(c) {
    kembali c == " " atau c == "\t" atau c == "\r" atau c == "\n"
}

fungsi proses_escape(esc) {
    jika esc == "n" maka kembali "\n" akhir
    jika esc == "t" maka kembali "\t" akhir
    jika esc == "r" maka kembali "\r" akhir
    kembali esc
}

fungsi lookup_keyword(buf) {
    jika buf == "fungsi"   maka kembali "FUNGSI"   akhir
    jika buf == "kembali"  maka kembali "KEMBALI"  akhir
    jika buf == "jika"     maka kembali "JIKA"     akhir
    jika buf == "maka"     maka kembali "MAKA"     akhir
    jika buf == "selain"   maka kembali "SELAIN"   akhir
    jika buf == "akhir"    maka kembali "AKHIR"    akhir
    jika buf == "selama"   maka kembali "SELAMA"   akhir
    jika buf == "ulang"    maka kembali "ULANG"    akhir
    jika buf == "hentikan" maka kembali "HENTIKAN" akhir
    jika buf == "lewati"   maka kembali "LEWATI"   akhir
    jika buf == "untuk"    maka kembali "UNTUK"    akhir
    jika buf == "dalam"    maka kembali "DALAM"    akhir
    jika buf == "dan"      maka kembali "DAN"      akhir
    jika buf == "atau"     maka kembali "ATAU"     akhir
    jika buf == "tidak"    maka kembali "TIDAK"    akhir
    jika buf == "benar"    maka kembali "BENAR"    akhir
    jika buf == "salah"    maka kembali "SALAH"    akhir
    jika buf == "null"     maka kembali "NULL"     akhir
    jika buf == "cetak"    maka kembali "CETAK"    akhir
    jika buf == "angka"    maka kembali "KW_ANGKA" akhir
    jika buf == "teks"     maka kembali "KW_TEKS"  akhir
    jika buf == "larik"    maka kembali "KW_LARIK" akhir
    jika buf == "bool"     maka kembali "KW_BOOL"  akhir
    jika buf == "milik"    maka kembali "MILIK"    akhir
    jika buf == "pakai"    maka kembali "PAKAI"    akhir
    jika buf == "sebagai"  maka kembali "SEBAGAI"  akhir
    jika buf == "jalankan" maka kembali "JALANKAN" akhir
    jika buf == "pilih"    maka kembali "PILIH"    akhir
    jika buf == "switch"   maka kembali "PILIH"    akhir
    jika buf == "kasus"    maka kembali "KASUS"    akhir
    jika buf == "kelompok" maka kembali "KASUS"    akhir
    jika buf == "coba"     maka kembali "COBA"     akhir
    jika buf == "tangkap"  maka kembali "TANGKAP"  akhir
    jika buf == "kalau"    maka kembali "TANGKAP"  akhir
    jika buf == "akhirnya" maka kembali "AKHIRNYA" akhir
    jika buf == "lempar"   maka kembali "LEMPAR"   akhir
    kembali "IDENTIFIKASI"
}

fungsi cek_op2(c, c2) {
    jika c == "|" dan c2 == ">" maka kembali "PIPA"    akhir
    jika c == "-" dan c2 == ">" maka kembali "PANAH"   akhir
    jika c == "=" dan c2 == "=" maka kembali "EQ"      akhir
    jika c == "!" dan c2 == "=" maka kembali "NEQ"     akhir
    jika c == "<" dan c2 == "=" maka kembali "LEQ"     akhir
    jika c == ">" dan c2 == "=" maka kembali "GEQ"     akhir
    jika c == "+" dan c2 == "=" maka kembali "PLUSEQ"  akhir
    jika c == "-" dan c2 == "=" maka kembali "MINUSEQ" akhir
    jika c == "*" dan c2 == "=" maka kembali "STAREQ"  akhir
    jika c == "/" dan c2 == "=" maka kembali "SLASHEQ" akhir
    kembali ""
}

fungsi baca_token_op1(c, baris) {
    jika c == "+" maka kembali ["PLUS",     "+", baris] akhir
    jika c == "-" maka kembali ["MINUS",    "-", baris] akhir
    jika c == "*" maka kembali ["BINTANG",  "*", baris] akhir
    jika c == "/" maka kembali ["SLASH",    "/", baris] akhir
    jika c == "%" maka kembali ["PERSEN",   "%", baris] akhir
    jika c == "=" maka kembali ["ASSIGN",   "=", baris] akhir
    jika c == "<" maka kembali ["LT",       "<", baris] akhir
    jika c == ">" maka kembali ["GT",       ">", baris] akhir
    jika c == "(" maka kembali ["LPAREN",   "(", baris] akhir
    jika c == ")" maka kembali ["RPAREN",   ")", baris] akhir
    jika c == "{" maka kembali ["LBRACE",   "{", baris] akhir
    jika c == "}" maka kembali ["RBRACE",   "}", baris] akhir
    jika c == "[" maka kembali ["LBRACKET", "[", baris] akhir
    jika c == "]" maka kembali ["RBRACKET", "]", baris] akhir
    jika c == "," maka kembali ["COMMA",    ",", baris] akhir
    jika c == "." maka kembali ["DOT",      ".", baris] akhir
    kembali ["UNKNOWN", c, baris]
}

fungsi tokenisasi(src) {
    angka pos   = 0
    angka baris = 1
    angka pjg   = panjang(src)
    larik hasil = []

    selama pos < pjg {
        teks c = karakter(src, pos)

        // Spasi
        jika adalah_spasi(c) maka
            jika c == "\n" maka baris += 1 akhir
            pos += 1
            lewati
        akhir

        jika c == "/" dan pos + 1 < pjg maka
            jika karakter(src, pos + 1) == "/" maka
                selama pos < pjg dan karakter(src, pos) != "\n" {
                    pos += 1
                }
                lewati
            akhir
            jika karakter(src, pos + 1) == "*" maka
                pos += 2
                selama pos < pjg - 1 {
                    jika karakter(src, pos) == "*" dan karakter(src, pos + 1) == "/" maka
                        pos += 2
                        hentikan
                    akhir
                    jika karakter(src, pos) == "\n" maka baris += 1 akhir
                    pos += 1
                }
                lewati
            akhir
        akhir

        jika c == "\"" maka
            pos += 1
            teks buf = ""
            angka ditutup = 0
            selama pos < pjg {
                teks ch = karakter(src, pos)
                jika ch == "\"" maka
                    ditutup = 1
                    hentikan
                akhir
                jika ch == "\\" maka
                    pos += 1
                    buf = tambah(buf, proses_escape(karakter(src, pos)))
                selain
                    buf = tambah(buf, ch)
                akhir
                pos += 1
            }
            jika ditutup == 0 maka
                p_gagal("string tidak ditutup - kurang tanda kutip penutup (\")", baris)
            akhir
            pos += 1
            hasil = tambah(hasil, ["STR", buf, baris])
            lewati
        akhir

        jika adalah_angka(c) maka
            teks buf = ""
            selama pos < pjg {
                teks ch = karakter(src, pos)
                jika adalah_angka(ch) maka
                    buf = tambah(buf, ch)
                    pos += 1
                selain
                    jika ch == "." dan pos + 1 < pjg dan adalah_angka(karakter(src, pos + 1)) maka
                        buf = tambah(buf, ch)
                        pos += 1
                    selain
                        hentikan
                    akhir
                akhir
            }
            hasil = tambah(hasil, ["NUM", buf, baris])
            lewati
        akhir

        jika adalah_huruf(c) maka
            teks buf = ""
            selama pos < pjg dan adalah_huruf_angka(karakter(src, pos)) {
                buf = tambah(buf, karakter(src, pos))
                pos += 1
            }
            hasil = tambah(hasil, [lookup_keyword(buf), buf, baris])
            lewati
        akhir

        teks c2 = ""
        jika pos + 1 < pjg maka c2 = karakter(src, pos + 1) akhir
        teks op2 = cek_op2(c, c2)
        jika op2 != "" maka
            hasil = tambah(hasil, [op2, tambah(c, c2), baris])
            pos += 2
            lewati
        akhir

        hasil = tambah(hasil, baca_token_op1(c, baris))
        pos += 1
    }

    hasil = tambah(hasil, ["EOF", "", baris])
    kembali hasil
}

angka P_pos    = 0
larik P_tokens = []
teks  G_src    = ""

fungsi potong_substr(s, mulai, akhir_) {
    teks hasil = ""
    angka i = mulai
    selama i < akhir_ {
        hasil = tambah(hasil, karakter(s, i))
        i += 1
    }
    kembali hasil
}

fungsi ambil_baris(s, target) {
    angka n = panjang(s)
    angka baris_ke = 1
    angka mulai = 0
    angka i = 0
    selama i < n {
        jika karakter(s, i) == "\n" maka
            jika baris_ke == target maka
                kembali potong_substr(s, mulai, i)
            akhir
            baris_ke += 1
            mulai = i + 1
        akhir
        i += 1
    }
    jika baris_ke == target maka
        kembali potong_substr(s, mulai, n)
    akhir
    kembali ""
}

fungsi p_gagal(pesan, baris) {
    cetak "PARSE ERROR baris #{baris}: #{pesan}"
    jika baris >= 1 maka
        teks teks_baris = ambil_baris(G_src, baris)
        jika teks_baris != "" maka
            cetak "  #{teks_baris}"
        akhir
    akhir
    _sistemKeluar(1)
}

fungsi p_tipe()  { kembali P_tokens[P_pos][0] }
fungsi p_val()   { kembali P_tokens[P_pos][1] }
fungsi p_baris() { kembali P_tokens[P_pos][2] }

fungsi p_maju() {
    angka old = P_pos
    P_pos += 1
    kembali P_tokens[old]
}

fungsi p_makan(tipe_exp) {
    jika p_tipe() != tipe_exp maka
        p_gagal("harap '#{tipe_exp}' dapat '#{p_tipe()}' ('#{p_val()}')", p_baris())
    akhir
    kembali p_maju()
}

fungsi p_peek_tipe() {
    jika P_pos + 1 < panjang(P_tokens) maka
        kembali P_tokens[P_pos + 1][0]
    akhir
    kembali "EOF"
}

fungsi ada_interpolasi(s) {
    angka i = 0
    angka slen = panjang(s)
    selama i < slen - 1 {
        jika karakter(s, i) == "#" dan karakter(s, i + 1) == "{" maka
            kembali 1
        akhir
        i += 1
    }
    kembali 0
}

fungsi parse_fragmen(frag) {
    angka simpan_pos    = P_pos
    larik simpan_tokens = P_tokens
    P_tokens = tokenisasi(frag)
    P_pos    = 0
    larik hasil = parse_expr()
    P_tokens = simpan_tokens
    P_pos    = simpan_pos
    kembali hasil
}

fungsi pecah_interpolasi(s) {
    larik parts = []
    teks buf = ""
    angka i = 0
    angka slen = panjang(s)
    selama i < slen {
        teks c = karakter(s, i)
        jika c == "#" dan i + 1 < slen dan karakter(s, i + 1) == "{" maka
            jika panjang(buf) > 0 maka
                parts = tambah(parts, ["LIT", buf])
                buf = ""
            akhir
            i += 2
            teks frag = ""
            angka depth = 1
            selama i < slen dan depth > 0 {
                teks fc = karakter(s, i)
                jika fc == "{" maka
                    depth += 1
                    frag = tambah(frag, fc)
                    i += 1
                selain
                    jika fc == "}" maka
                        depth -= 1
                        jika depth > 0 maka
                            frag = tambah(frag, fc)
                        akhir
                        i += 1
                    selain
                        frag = tambah(frag, fc)
                        i += 1
                    akhir
                akhir
            }
            parts = tambah(parts, ["EXPR", parse_fragmen(frag)])
        selain
            buf = tambah(buf, c)
            i += 1
        akhir
    }
    jika panjang(buf) > 0 maka
        parts = tambah(parts, ["LIT", buf])
    akhir
    kembali parts
}

fungsi parse_blok_brace() {
    p_makan("LBRACE")
    larik stmts = []
    selama p_tipe() != "RBRACE" dan p_tipe() != "EOF" {
        stmts = tambah(stmts, parse_stmt())
    }
    p_makan("RBRACE")
    kembali stmts
}

fungsi parse_jika_blok() {
    larik stmts = []
    selama p_tipe() != "EOF" dan p_tipe() != "AKHIR" dan p_tipe() != "SELAIN" {
        stmts = tambah(stmts, parse_stmt())
    }
    kembali stmts
}

fungsi parse_stmt() {
    teks t  = p_tipe()
    angka bl = p_baris()

    jika t == "FUNGSI" maka
        p_makan("FUNGSI")
        teks nama = p_val()
        p_makan("IDENTIFIKASI")
        p_makan("LPAREN")
        larik params = []
        selama p_tipe() != "RPAREN" dan p_tipe() != "EOF" {
            params = tambah(params, p_val())
            p_makan("IDENTIFIKASI")
            jika p_tipe() == "COMMA" maka p_makan("COMMA") akhir
        }
        p_makan("RPAREN")
        larik body = parse_blok_brace()
        kembali ["FUNGSI", nama, params, body, bl]
    akhir

    jika t == "KEMBALI" maka
        p_makan("KEMBALI")
        kembali ["KEMBALI", parse_expr(), bl]
    akhir

    jika t == "CETAK" maka
        p_makan("CETAK")
        kembali ["CETAK", parse_expr(), bl]
    akhir

    jika t == "HENTIKAN" maka
        p_makan("HENTIKAN")
        kembali ["HENTIKAN", bl]
    akhir

    jika t == "LEWATI" maka
        p_makan("LEWATI")
        kembali ["LEWATI", bl]
    akhir

    jika t == "JIKA" maka
        p_makan("JIKA")
        larik kond   = parse_expr()
        p_makan("MAKA")
        larik body_t = parse_jika_blok()
        larik body_f = []
        jika p_tipe() == "SELAIN" maka
            p_makan("SELAIN")
            jika p_tipe() == "JIKA" maka
                body_f = tambah(body_f, parse_stmt())
            selain
                body_f = parse_jika_blok()
            akhir
        akhir
        p_makan("AKHIR")
        kembali ["JIKA", kond, body_t, body_f, bl]
    akhir

    jika t == "SELAMA" maka
        p_makan("SELAMA")
        larik kond = parse_expr()
        larik body = parse_blok_brace()
        kembali ["SELAMA", kond, body, bl]
    akhir

    jika t == "ULANG" maka
        p_makan("ULANG")
        larik hitungan = parse_expr()
        larik body     = parse_blok_brace()
        kembali ["ULANG", hitungan, body, bl]
    akhir

    jika t == "UNTUK" maka
        p_makan("UNTUK")
        teks var = p_val()
        p_makan("IDENTIFIKASI")
        p_makan("DALAM")
        larik iter = parse_expr()
        larik body = parse_blok_brace()
        kembali ["UNTUK", var, iter, body, bl]
    akhir

    jika t == "LEMPAR" maka
        p_makan("LEMPAR")
        kembali ["LEMPAR", parse_expr(), bl]
    akhir

    jika t == "COBA" maka
        p_makan("COBA")
        // bentuk baru: coba { } tangkap { } akhirnya { }  (sama dengan interpreter)
        jika p_tipe() == "LBRACE" maka
            larik cb_body = parse_blok_brace()
            larik tb_body = []
            angka ada_t = 0
            larik ab_body = []
            angka ada_a = 0
            jika p_tipe() == "TANGKAP" maka
                p_makan("TANGKAP")
                tb_body = parse_blok_brace()
                ada_t = 1
            akhir
            jika p_tipe() == "AKHIRNYA" maka
                p_makan("AKHIRNYA")
                ab_body = parse_blok_brace()
                ada_a = 1
            akhir
            jika ada_t == 0 dan ada_a == 0 maka
                p_gagal("'coba' harus diikuti 'tangkap' atau 'akhirnya'", bl)
            akhir
            kembali ["COBA", cb_body, tb_body, bl, ada_t, ab_body, ada_a]
        akhir
        // bentuk lama (tanpa kurung kurawal): coba ... tangkap ... akhir
        larik coba_body = []
        selama p_tipe() != "TANGKAP" dan p_tipe() != "AKHIR" dan p_tipe() != "EOF" {
            coba_body = tambah(coba_body, parse_stmt())
        }
        larik tangkap_body = []
        jika p_tipe() == "TANGKAP" maka
            p_makan("TANGKAP")
            selama p_tipe() != "AKHIR" dan p_tipe() != "EOF" {
                tangkap_body = tambah(tangkap_body, parse_stmt())
            }
        akhir
        p_makan("AKHIR")
        kembali ["COBA", coba_body, tangkap_body, bl, 1, [], 0]
    akhir

    jika t == "PILIH" maka
        p_makan("PILIH")
        larik eksp_pilih = parse_expr()
        p_makan("LBRACE")
        larik daftar_kasus = []
        milik default_stmt = 0
        selama p_tipe() != "RBRACE" dan p_tipe() != "EOF" {
            jika p_tipe() == "KASUS" maka
                p_makan("KASUS")
                larik val = parse_expr()
                p_makan("PANAH")
                larik stmt_kasus = parse_stmt()
                daftar_kasus = tambah(daftar_kasus, [val, stmt_kasus])
            selain
                jika p_tipe() == "SELAIN" maka
                    p_makan("SELAIN")
                    p_makan("PANAH")
                    default_stmt = parse_stmt()
                selain
                    p_maju()
                akhir
            akhir
        }
        p_makan("RBRACE")
        kembali ["PILIH", eksp_pilih, daftar_kasus, default_stmt, bl]
    akhir

    jika t == "JALANKAN" maka
        p_makan("JALANKAN")
        p_makan("LPAREN")
        larik ekspr_jalankan = parse_expr()
        p_makan("RPAREN")
        kembali ["PANGGIL", "eksekusi", [ekspr_jalankan], bl]
    akhir

    jika t == "PAKAI" maka
        p_makan("PAKAI")
        teks modnama = p_val()
        p_makan("IDENTIFIKASI")
        teks alias_modul = ""
        jika p_tipe() == "SEBAGAI" maka
            p_makan("SEBAGAI")
            alias_modul = p_val()
            p_makan("IDENTIFIKASI")
        akhir
        kembali ["PAKAI", modnama, bl, alias_modul]
    akhir

    jika t == "KW_ANGKA" atau t == "KW_TEKS" atau t == "KW_LARIK" atau t == "MILIK" atau t == "KW_BOOL" maka
        p_maju()
        teks nama = p_val()
        p_makan("IDENTIFIKASI")
        p_makan("ASSIGN")
        kembali ["ASSIGN", nama, parse_expr(), bl]
    akhir

    jika t == "IDENTIFIKASI" maka
        teks nxt = p_peek_tipe()
        jika nxt == "ASSIGN" maka
            teks nama = p_val()
            p_makan("IDENTIFIKASI")
            p_makan("ASSIGN")
            kembali ["ASSIGN", nama, parse_expr(), bl]
        akhir
        jika nxt == "PLUSEQ" atau nxt == "MINUSEQ" atau nxt == "STAREQ" atau nxt == "SLASHEQ" maka
            teks nama = p_val()
            p_makan("IDENTIFIKASI")
            teks op = p_val()
            p_maju()
            kembali ["COMPOUND", nama, op, parse_expr(), bl]
        akhir
    akhir

    larik e = parse_expr()
    jika e[0] == "INDEX" dan p_tipe() == "ASSIGN" maka
        p_makan("ASSIGN")
        kembali ["IDXASSIGN", e[1], e[2], parse_expr(), bl]
    akhir
    kembali e
}

fungsi parse_expr()   { kembali parse_logika() }

fungsi parse_logika() {
    larik kiri = parse_perbandingan()
    selama p_tipe() == "DAN" atau p_tipe() == "ATAU" {
        teks op = p_val()
        p_maju()
        kiri = ["BINOP", op, kiri, parse_perbandingan()]
    }
    kembali kiri
}

fungsi parse_perbandingan() {
    larik kiri = parse_tambah_kurang()
    selama p_tipe() == "EQ" atau p_tipe() == "NEQ" atau
           p_tipe() == "LT" atau p_tipe() == "GT"  atau
           p_tipe() == "LEQ" atau p_tipe() == "GEQ" {
        teks op = p_val()
        p_maju()
        kiri = ["BINOP", op, kiri, parse_tambah_kurang()]
    }
    kembali kiri
}

fungsi parse_tambah_kurang() {
    larik kiri = parse_kali_bagi()
    selama p_tipe() == "PLUS" atau p_tipe() == "MINUS" {
        teks op = p_val()
        p_maju()
        kiri = ["BINOP", op, kiri, parse_kali_bagi()]
    }
    kembali kiri
}

fungsi parse_kali_bagi() {
    larik kiri = parse_unary()
    selama p_tipe() == "BINTANG" atau p_tipe() == "SLASH" atau p_tipe() == "PERSEN" {
        teks op = p_val()
        p_maju()
        kiri = ["BINOP", op, kiri, parse_unary()]
    }
    kembali kiri
}

fungsi parse_unary() {
    jika p_tipe() == "TIDAK" maka
        p_maju()
        kembali ["UNIOP", "tidak", parse_unary()]
    akhir
    jika p_tipe() == "MINUS" maka
        p_maju()
        kembali ["UNIOP", "-", parse_unary()]
    akhir
    kembali parse_primer()
}

fungsi parse_arglist() {
    larik args = []
    selama p_tipe() != "RPAREN" dan p_tipe() != "EOF" {
        args = tambah(args, parse_expr())
        jika p_tipe() == "COMMA" maka p_makan("COMMA") akhir
    }
    kembali args
}

fungsi parse_index_chain(basis) {
    selama p_tipe() == "LBRACKET" {
        p_makan("LBRACKET")
        larik idx = parse_expr()
        p_makan("RBRACKET")
        basis = ["INDEX", basis, idx]
    }
    kembali basis
}

fungsi parse_primer() {
    teks t  = p_tipe()
    teks v  = p_val()
    angka bl = p_baris()

    jika t == "NUM"   maka p_maju() kembali ["NUM",  teksAngka(v)] akhir
    jika t == "STR"   maka
        p_maju()
        jika ada_interpolasi(v) maka
            kembali ["STRINTERP", pecah_interpolasi(v)]
        akhir
        kembali ["STR", v]
    akhir
    jika t == "BENAR" maka p_maju() kembali ["BOOL", 1]            akhir
    jika t == "SALAH" maka p_maju() kembali ["BOOL", 0]            akhir
    jika t == "NULL"  maka p_maju() kembali ["BOOL", 0]            akhir   // runtime native tidak punya null terpisah: 0

    jika t == "LBRACKET" maka
        p_makan("LBRACKET")
        larik elems = []
        selama p_tipe() != "RBRACKET" dan p_tipe() != "EOF" {
            elems = tambah(elems, parse_expr())
            jika p_tipe() == "COMMA" maka p_makan("COMMA") akhir
        }
        p_makan("RBRACKET")
        kembali parse_index_chain(["ARRAY", elems])
    akhir

    jika t == "LPAREN" maka
        p_makan("LPAREN")
        larik e = parse_expr()
        p_makan("RPAREN")
        kembali e
    akhir

    jika t == "IDENTIFIKASI" maka
        p_maju()
        jika p_tipe() == "DOT" maka
            p_makan("DOT")
            teks anggota = p_val()
            p_makan("IDENTIFIKASI")
            p_makan("LPAREN")
            larik args_ns = parse_arglist()
            p_makan("RPAREN")
            kembali parse_index_chain(["PANGGIL", "#{v}_#{anggota}", args_ns, bl])
        akhir
        jika p_tipe() == "LPAREN" maka
            p_makan("LPAREN")
            larik args = parse_arglist()
            p_makan("RPAREN")
            kembali parse_index_chain(["PANGGIL", v, args, bl])
        akhir
        kembali parse_index_chain(["VAR", v, bl])
    akhir

    p_gagal("token tak dikenal '#{v}' (#{t})", bl)
    p_maju()
    kembali ["NULL"]
}

fungsi parse_program(tokens) {
    P_tokens = tokens
    P_pos    = 0
    larik body = []
    selama p_tipe() != "EOF" {
        body = tambah(body, parse_stmt())
    }
    kembali ["PROGRAM", body]
}

angka  CG_label    = 0
larik  CG_data     = []
larik  CG_bss      = []
larik  CG_text     = []
angka  CG_stack    = 0
larik  CG_vars     = []
larik  CG_globals  = []
angka  CG_in_fungsi = 0
larik  CG_known_funcs = ["cetak", "cetakAngka", "cetakTeks", "panjang", "tambah",
    "eksekusi", "irisLarik", "gabungTeks", "kodeKarakter", "karakter", "bulatkan",
    "angkaTeks", "teksAngka", "bacaFileTeks", "tulisFileTeks", "cetakTanpaBaris",
    "_sistemKeluar", "tidur", "masukan", "tipe", "acakAngka", "dariKode", "tulisStderr"]

fungsi cg_fungsi_dikenal(nama) {
    angka i = 0
    selama i < panjang(CG_known_funcs) {
        jika CG_known_funcs[i] == nama maka
            kembali 1
        akhir
        i += 1
    }
    kembali 0
}

larik CG_pakai_loaded = []

fungsi cg_sudah_dimuat(nama) {
    angka i = 0
    selama i < panjang(CG_pakai_loaded) {
        jika CG_pakai_loaded[i] == nama maka
            kembali 1
        akhir
        i += 1
    }
    kembali 0
}

// Pecah body program jadi fungsi_nodes vs top_stmts, sambil ngerekursi ke dalam
// modul yang di-'pakai' (load-once, sama kayak semantik loop.c). fn/ts adalah
// akumulator (larik) - dikembalikan sebagai pasangan [fn_baru, ts_baru] karena
// Loop gak punya parameter-by-reference.
fungsi cg_proses_body(body, fn, ts, alias_awalan) {
    angka i = 0
    selama i < panjang(body) {
        larik nd = body[i]
        jika nd[0] == "FUNGSI" maka
            teks nama_final = nd[1]
            jika alias_awalan != "" maka
                nama_final = "#{alias_awalan}_#{nd[1]}"
            akhir
            larik nd_baru = ["FUNGSI", nama_final, nd[2], nd[3], nd[4]]
            fn = tambah(fn, nd_baru)
            CG_known_funcs = tambah(CG_known_funcs, nama_final)
        selain
            jika nd[0] == "PAKAI" maka
                teks modnama = nd[1]
                teks alias_baru = nd[3]
                teks kunci_muat = "#{modnama}::#{alias_baru}"
                jika cg_sudah_dimuat(kunci_muat) == 0 maka
                    CG_pakai_loaded = tambah(CG_pakai_loaded, kunci_muat)
                    teks path = "Library/#{modnama}.lp"
                    teks kode = bacaFileTeks(path)
                    jika kode == "" maka
                        p_gagal("pakai: modul '#{modnama}' tidak ditemukan (dicari di #{path})", nd[2])
                    akhir
                    larik simpan_tok = P_tokens
                    angka simpan_pos = P_pos
                    teks  simpan_src = G_src
                    G_src = kode
                    larik modprog  = parse_program(tokenisasi(kode))
                    larik pasangan = cg_proses_body(modprog[1], fn, ts, alias_baru)
                    fn = pasangan[0]
                    ts = pasangan[1]
                    G_src    = simpan_src
                    P_tokens = simpan_tok
                    P_pos    = simpan_pos
                akhir
            selain
                ts = tambah(ts, nd)
            akhir
        akhir
        i += 1
    }
    kembali [fn, ts]
}

fungsi cg_is_global(nama) {
    angka i = 0
    selama i < panjang(CG_globals) {
        jika CG_globals[i] == nama maka
            kembali 1
        akhir
        i += 1
    }
    kembali 0
}

fungsi cg_global_daftar(nama) {
    jika cg_is_global(nama) maka
        kembali 0
    akhir
    CG_globals = tambah(CG_globals, nama)
    cg_emit_bss("    #{nama}: resq 1")
    kembali 0
}
teks   CG_func_end = ""
angka  CG_str_cnt  = 0
larik  CG_break_stk    = []
larik  CG_cleanup_stk  = []   // wilayah coba aktif: [ada_handler, ada_akhirnya, badan_akhirnya]
larik  CG_loop_base_stk = []  // panjang CG_cleanup_stk saat tiap loop dimulai
larik  CG_continue_stk = []

fungsi cg_label_baru() {
    CG_label += 1
    kembali angkaTeks(CG_label)
}

fungsi cg_emit(baris)    { CG_text = tambah(CG_text, baris) }
fungsi cg_emit_data(row) { CG_data = tambah(CG_data, row)   }
fungsi cg_emit_bss(row)  { CG_bss  = tambah(CG_bss, row)    }

fungsi cg_escape_str(s) {
    teks hasil = ""
    teks buf = ""
    angka pertama = 1
    angka i = 0
    angka slen = panjang(s)
    selama i < slen {
        teks ch = karakter(s, i)
        angka kode = kodeKarakter(ch)
        jika kode == 10 atau kode == 9 atau kode == 13 atau kode == 39 maka
            jika panjang(buf) > 0 maka
                jika pertama == 0 maka hasil = tambah(hasil, ", ") akhir
                hasil = tambah(hasil, "'#{buf}'")
                pertama = 0
                buf = ""
            akhir
            jika pertama == 0 maka hasil = tambah(hasil, ", ") akhir
            jika kode == 10 maka hasil = tambah(hasil, "0xA") akhir
            jika kode == 9 maka hasil = tambah(hasil, "0x9") akhir
            jika kode == 13 maka hasil = tambah(hasil, "0xD") akhir
            jika kode == 39 maka hasil = tambah(hasil, "0x27") akhir
            pertama = 0
        selain
            buf = tambah(buf, ch)
        akhir
        i += 1
    }
    jika panjang(buf) > 0 maka
        jika pertama == 0 maka hasil = tambah(hasil, ", ") akhir
        hasil = tambah(hasil, "'#{buf}'")
        pertama = 0
    akhir
    jika panjang(hasil) == 0 maka
        hasil = "''"
    akhir
    kembali hasil
}

fungsi cg_data_string(s) {
    CG_str_cnt += 1
    teks lbl = "__s#{angkaTeks(CG_str_cnt)}"
    teks esc = cg_escape_str(s)
    angka slen = panjang(s)
    cg_emit_data("    align 2   ; jamin alamat genap - bit0 dipakai penanda tag angka")
    cg_emit_data("    #{lbl}:     db #{esc}, 0")
    cg_emit_data("    #{lbl}_len: equ #{angkaTeks(slen)}")
    kembali lbl
}

fungsi fmt_off(off) {
    jika off >= 0 maka
        kembali "+#{angkaTeks(off)}"
    akhir
    angka positif = 0 - off
    kembali "-#{angkaTeks(positif)}"
}

fungsi cg_alloc(nama) {
    angka i = panjang(CG_vars) - 1
    selama i >= 0 {
        jika CG_vars[i][0] == nama maka
            kembali 0 - CG_vars[i][1]
        akhir
        i -= 1
    }
    CG_stack += 8
    CG_vars = tambah(CG_vars, [nama, CG_stack])
    kembali 0 - CG_stack
}

fungsi cg_cari(nama) {
    angka i = panjang(CG_vars) - 1
    selama i >= 0 {
        jika CG_vars[i][0] == nama maka
            kembali 0 - CG_vars[i][1]
        akhir
        i -= 1
    }
    kembali 99999
}

fungsi cg_var_load(nama, baris) {
    angka off = cg_cari(nama)
    jika off != 99999 maka
        cg_emit("    mov  rax, qword [rbp#{fmt_off(off)}]")
        kembali 0
    akhir
    jika cg_is_global(nama) maka
        cg_emit("    mov  rax, qword [rel #{nama}]")
        kembali 0
    akhir
    jika cg_fungsi_dikenal(nama) == 1 maka
        cg_emit("    lea  rax, [rel #{nama}]   ; nilai fungsi (first-class)")
        kembali 0
    akhir
    p_gagal("variabel '#{nama}' belum pernah di-assign", baris)
}

fungsi cg_var_store(nama) {
    angka off = cg_cari(nama)
    jika off != 99999 maka
        cg_emit("    mov  qword [rbp#{fmt_off(off)}], rax")
        kembali 0
    akhir
    jika cg_is_global(nama) maka
        cg_emit("    mov  qword [rel #{nama}], rax")
        kembali 0
    akhir
    jika CG_in_fungsi == 0 maka
        cg_global_daftar(nama)
        cg_emit("    mov  qword [rel #{nama}], rax")
    selain
        angka newoff = cg_alloc(nama)
        cg_emit("    mov  qword [rbp#{fmt_off(newoff)}], rax")
    akhir
    kembali 0
}

larik ABI_REGS = ["rdi", "rsi", "rdx", "rcx", "r8", "r9"]

fungsi cg_expr(node) {
    teks tip = node[0]

    jika tip == "NUM" maka
        angka ni = bulatkan(node[1])
        cg_emit("    mov  rax, #{angkaTeks(ni * 2 + 1)}   ; angka tag: (n<<1)|1")
        kembali 0
    akhir

    jika tip == "BOOL" maka
        cg_emit("    mov  rax, #{angkaTeks(node[1] * 2 + 1)}   ; bool tag")
        kembali 0
    akhir

    jika tip == "STR" maka
        teks lbl = cg_data_string(node[1])
        cg_emit("    lea  rax, [rel #{lbl}]")
        kembali 0
    akhir

    jika tip == "STRINTERP" maka
        larik parts = node[1]
        angka n = panjang(parts)
        jika n == 0 maka
            teks lbl0 = cg_data_string("")
            cg_emit("    lea  rax, [rel #{lbl0}]")
            kembali 0
        akhir
        jika parts[0][0] == "LIT" maka
            teks lbl1 = cg_data_string(parts[0][1])
            cg_emit("    lea  rax, [rel #{lbl1}]")
        selain
            cg_expr(parts[0][1])
            cg_emit("    mov  rdi, rax")
            cg_emit("    call __ke_teks")
        akhir
        angka idx = 1
        selama idx < n {
            cg_emit("    push rax")
            jika parts[idx][0] == "LIT" maka
                teks lbln = cg_data_string(parts[idx][1])
                cg_emit("    lea  rax, [rel #{lbln}]")
            selain
                cg_expr(parts[idx][1])
                cg_emit("    mov  rdi, rax")
                cg_emit("    call __ke_teks")
            akhir
            cg_emit("    mov  rsi, rax")
            cg_emit("    pop  rdi")
            cg_emit("    call __tambah")
            idx += 1
        }
        kembali 0
    akhir

    jika tip == "NULL" maka
        cg_emit("    xor  rax, rax")
        kembali 0
    akhir

    jika tip == "VAR" maka
        cg_var_load(node[1], node[2])
        kembali 0
    akhir

    jika tip == "BINOP" maka
        cg_binop(node)
        kembali 0
    akhir

    jika tip == "UNIOP" maka
        cg_expr(node[2])
        jika node[1] == "-" maka
            cg_emit("    neg  rax")
            cg_emit("    add  rax, 2   ; re-tag hasil negate")
        akhir
        jika node[1] == "tidak" maka
            cg_emit("    cmp  rax, 1   ; 1 = tagged-false")
            cg_emit("    sete al")
            cg_emit("    movzx rax, al")
            cg_emit("    lea  rax, [rax*2 + 1]   ; re-tag bool")
        akhir
        kembali 0
    akhir

    jika tip == "PANGGIL" maka
        cg_panggil(node)
        kembali 0
    akhir

    jika tip == "ARRAY" maka
        cg_array(node)
        kembali 0
    akhir

    jika tip == "INDEX" maka
        cg_index(node)
        kembali 0
    akhir

    jika tip == "ASSIGN" maka
        cg_expr(node[2])
        angka off = cg_alloc(node[1])
        cg_emit("    mov  qword [rbp#{fmt_off(off)}], rax")
        kembali 0
    akhir

    cg_emit("    ; expr tak dikenal: #{tip}")
    cg_emit("    xor  rax, rax")
}

fungsi cg_binop(node) {
    teks op = node[1]

    jika op == "dan" maka
        teks lid  = cg_label_baru()
        teks lf   = "__df_#{lid}"
        teks le   = "__de_#{lid}"
        cg_expr(node[2])
        cg_emit("    cmp  rax, 1   ; 1 = tagged-false")
        cg_emit("    je   #{lf}")
        cg_expr(node[3])
        cg_emit("    cmp  rax, 1")
        cg_emit("    je   #{lf}")
        cg_emit("    mov  rax, 3   ; tagged true")
        cg_emit("    jmp  #{le}")
        cg_emit("#{lf}:")
        cg_emit("    mov  rax, 1   ; tagged false")
        cg_emit("#{le}:")
        kembali 0
    akhir

    jika op == "atau" maka
        teks lid  = cg_label_baru()
        teks lt2  = "__ot_#{lid}"
        teks le   = "__oe_#{lid}"
        cg_expr(node[2])
        cg_emit("    cmp  rax, 1")
        cg_emit("    jne  #{lt2}")
        cg_expr(node[3])
        cg_emit("    cmp  rax, 1")
        cg_emit("    jne  #{lt2}")
        cg_emit("    mov  rax, 1   ; tagged false")
        cg_emit("    jmp  #{le}")
        cg_emit("#{lt2}:")
        cg_emit("    mov  rax, 3   ; tagged true")
        cg_emit("#{le}:")
        kembali 0
    akhir

    cg_expr(node[2])
    cg_emit("    push rax")
    cg_expr(node[3])
    cg_emit("    mov  rcx, rax")
    cg_emit("    pop  rax")

    jika op == "+" maka
        cg_emit("    add  rax, rcx")
        cg_emit("    dec  rax   ; re-tag jumlah")
        kembali 0
    akhir
    jika op == "-" maka
        cg_emit("    sub  rax, rcx")
        cg_emit("    or   rax, 1   ; re-tag selisih")
        kembali 0
    akhir
    jika op == "*" maka
        cg_emit("    sar  rax, 1   ; lepas tag")
        cg_emit("    sar  rcx, 1")
        cg_emit("    imul rax, rcx")
        cg_emit("    lea  rax, [rax*2 + 1]   ; re-tag hasil kali")
        kembali 0
    akhir
    jika op == "/" maka
        teks ldz1 = "__dz_#{cg_label_baru()}"
        teks lok1 = "__do_#{cg_label_baru()}"
        cg_emit("    sar  rax, 1   ; lepas tag")
        cg_emit("    sar  rcx, 1")
        cg_emit("    test rcx, rcx")
        cg_emit("    jz   #{ldz1}")
        cg_emit("    cqo")
        cg_emit("    idiv rcx")
        cg_emit("    jmp  #{lok1}")
        cg_emit("#{ldz1}:")
        cg_emit("    call __galatNol   ; GalatNilai, tidak kembali")
        cg_emit("    jmp  __lempar")
        cg_emit("    xor  rax, rax")
        cg_emit("#{lok1}:")
        cg_emit("    lea  rax, [rax*2 + 1]   ; re-tag hasil bagi")
        kembali 0
    akhir
    jika op == "%" maka
        teks ldz2 = "__dz_#{cg_label_baru()}"
        teks lok2 = "__do_#{cg_label_baru()}"
        cg_emit("    sar  rax, 1   ; lepas tag")
        cg_emit("    sar  rcx, 1")
        cg_emit("    test rcx, rcx")
        cg_emit("    jz   #{ldz2}")
        cg_emit("    cqo")
        cg_emit("    idiv rcx")
        cg_emit("    mov  rax, rdx")
        cg_emit("    jmp  #{lok2}")
        cg_emit("#{ldz2}:")
        cg_emit("    call __galatNol   ; GalatNilai, tidak kembali")
        cg_emit("    jmp  __lempar")
        cg_emit("    xor  rax, rax")
        cg_emit("#{lok2}:")
        cg_emit("    lea  rax, [rax*2 + 1]   ; re-tag sisa bagi")
        kembali 0
    akhir

    jika op == "==" maka
        cg_emit("    mov  rdi, rax")
        cg_emit("    mov  rsi, rcx")
        cg_emit("    call __setara")
        kembali 0
    akhir
    jika op == "!=" maka
        cg_emit("    mov  rdi, rax")
        cg_emit("    mov  rsi, rcx")
        cg_emit("    call __setara")
        cg_emit("    xor  rax, 2   ; toggle antara tagged-true(3)/tagged-false(1)")
        kembali 0
    akhir

    cg_emit("    cmp  rax, rcx   ; urutan tetap benar walau tertag")
    jika op == "<"  maka cg_emit("    setl  al") akhir
    jika op == ">"  maka cg_emit("    setg  al") akhir
    jika op == "<=" maka cg_emit("    setle al") akhir
    jika op == ">=" maka cg_emit("    setge al") akhir
    cg_emit("    movzx rax, al")
    cg_emit("    lea  rax, [rax*2 + 1]   ; re-tag hasil bool")
}

fungsi cg_panggil(node) {
    teks  nama  = node[1]
    larik args  = node[2]
    angka n_arg = panjang(args)

    angka i = 0
    selama i < n_arg {
        cg_expr(args[i])
        cg_emit("    push rax")
        i += 1
    }
    angka j = n_arg - 1
    selama j >= 0 {
        jika j < 6 maka
            cg_emit("    pop  #{ABI_REGS[j]}")
        selain
            cg_emit("    pop  rax   ; arg >6 - dibuang")
        akhir
        j -= 1
    }

    jika nama == "cetak" maka
        cg_emit("    call __print_auto")
        kembali 0
    akhir
    jika nama == "cetakAngka" maka
        cg_emit("    call __print_int")
        kembali 0
    akhir
    jika nama == "cetakTeks" maka
        cg_emit("    call __print_str_nl")
        kembali 0
    akhir
    jika nama == "panjang" maka
        cg_emit("    call __panjang")
        kembali 0
    akhir
    jika nama == "tambah" maka
        cg_emit("    call __tambah")
        kembali 0
    akhir
    jika nama == "eksekusi" maka
        cg_emit("    call __eksekusi")
        kembali 0
    akhir

    jika nama == "galat" dan n_arg == 2 maka
        cg_emit("    mov  rdx, 1   ; sebab = null")
    akhir
    jika cg_fungsi_dikenal(nama) == 1 maka
        cg_emit("    call #{nama}")
        kembali 0
    akhir

    angka off_cb = cg_cari(nama)
    jika off_cb != 99999 maka
        cg_emit("    mov  rax, qword [rbp#{fmt_off(off_cb)}]   ; nilai fungsi (callback)")
        cg_emit("    call rax")
        kembali 0
    akhir
    jika cg_is_global(nama) maka
        cg_emit("    mov  rax, qword [rel #{nama}]   ; nilai fungsi (callback)")
        cg_emit("    call rax")
        kembali 0
    akhir

    angka baris_panggil = 0
    jika panjang(node) >= 4 maka
        baris_panggil = node[3]
    akhir
    p_gagal("fungsi '#{nama}' tidak dikenal", baris_panggil)
}

fungsi cg_array(node) {
    larik elems = node[1]
    angka n     = panjang(elems)
    angka bytes = 24 + n * 8
    cg_emit("    mov  rdi, #{angkaTeks(bytes)}")
    cg_emit("    call __alloc")
    cg_emit("    push rax")
    cg_emit("    mov  qword [rax], 0x4C4F4F50")
    cg_emit("    mov  qword [rax + 8], #{angkaTeks(n)}")
    cg_emit("    mov  qword [rax + 16], #{angkaTeks(n)}")
    angka i = 0
    selama i < n {
        cg_expr(elems[i])
        cg_emit("    pop  rcx")
        angka offset = 24 + i * 8
        cg_emit("    mov  qword [rcx + #{angkaTeks(offset)}], rax")
        cg_emit("    push rcx")
        i += 1
    }
    cg_emit("    pop  rax")
}

fungsi cg_index(node) {
    cg_expr(node[2])
    cg_emit("    push rax")
    cg_expr(node[1])
    cg_emit("    pop  rcx")
    cg_emit("    sar  rcx, 1   ; lepas tag indeks")
    cg_emit("    call __idx_cek   ; GalatTipe/GalatIndeks bila bukan larik atau di luar batas")
    cg_emit("    imul rcx, 8")
    cg_emit("    add  rax, rcx")
    cg_emit("    add  rax, 24   ; lewati header [MAGIC][PANJANG][KAPASITAS]")
    cg_emit("    mov  rax, qword [rax]")
}

fungsi cg_stmt(node) {
    teks tip = node[0]

    jika tip == "IDXASSIGN" maka
        larik basis = node[1]
        larik idx   = node[2]
        larik val   = node[3]
        cg_expr(val)
        cg_emit("    push rax   ; nilai baru")
        cg_expr(idx)
        cg_emit("    push rax   ; indeks")
        cg_expr(basis)
        cg_emit("    pop  rcx   ; indeks")
        cg_emit("    sar  rcx, 1   ; lepas tag indeks")
        cg_emit("    call __idx_cek   ; GalatTipe/GalatIndeks bila bukan larik atau di luar batas")
        cg_emit("    imul rcx, 8")
        cg_emit("    add  rax, rcx")
        cg_emit("    add  rax, 24   ; lewati header [MAGIC][PANJANG][KAPASITAS]")
        cg_emit("    pop  rdx   ; nilai baru")
        cg_emit("    mov  qword [rax], rdx")
        kembali 0
    akhir

    jika tip == "ASSIGN" maka
        cg_expr(node[2])
        cg_var_store(node[1])
        kembali 0
    akhir

    jika tip == "COMPOUND" maka
        cg_expr(node[3])
        cg_emit("    mov  rcx, rax")
        cg_var_load(node[1], node[4])
        teks op = node[2]
        jika op == "+=" maka
            cg_emit("    add  rax, rcx")
            cg_emit("    dec  rax   ; re-tag")
        akhir
        jika op == "-=" maka
            cg_emit("    sub  rax, rcx")
            cg_emit("    or   rax, 1   ; re-tag")
        akhir
        jika op == "*=" maka
            cg_emit("    sar  rax, 1   ; lepas tag")
            cg_emit("    sar  rcx, 1")
            cg_emit("    imul rax, rcx")
            cg_emit("    lea  rax, [rax*2 + 1]   ; re-tag")
        akhir
        jika op == "/=" maka
            teks ldz3 = "__dz_#{cg_label_baru()}"
            teks lok3 = "__do_#{cg_label_baru()}"
            cg_emit("    sar  rax, 1   ; lepas tag")
            cg_emit("    sar  rcx, 1")
            cg_emit("    test rcx, rcx")
            cg_emit("    jz   #{ldz3}")
            cg_emit("    cqo")
            cg_emit("    idiv rcx")
            cg_emit("    jmp  #{lok3}")
            cg_emit("#{ldz3}:")
            cg_emit("    call __galatNol   ; GalatNilai, tidak kembali")
            cg_emit("    jmp  __lempar")
            cg_emit("    xor  rax, rax")
            cg_emit("#{lok3}:")
            cg_emit("    lea  rax, [rax*2 + 1]   ; re-tag")
        akhir
        cg_var_store(node[1])
        kembali 0
    akhir

    jika tip == "CETAK" maka
        larik e = node[1]
        jika e[0] == "STR" maka
            teks lbl = cg_data_string(e[1])
            angka slen = panjang(e[1])
            cg_emit("    mov  rax, 1")
            cg_emit("    mov  rdi, 1")
            cg_emit("    lea  rsi, [rel #{lbl}]")
            cg_emit("    mov  rdx, #{angkaTeks(slen)}")
            cg_emit("    syscall")
            cg_emit("    mov  rax, 1")
            cg_emit("    mov  rdi, 1")
            cg_emit("    lea  rsi, [rel __nl]")
            cg_emit("    mov  rdx, 1")
            cg_emit("    syscall")
        selain
            jika e[0] == "STRINTERP" maka
                cg_expr(e)
                cg_emit("    mov  rdi, rax")
                cg_emit("    call __print_str_nl")
            selain
                cg_expr(e)
                cg_emit("    mov  rdi, rax")
                cg_emit("    call __ke_teks   ; fix: dulu selalu __print_int, cetak var teks jadi salah")
                cg_emit("    mov  rdi, rax")
                cg_emit("    call __print_str_nl")
            akhir
        akhir
        kembali 0
    akhir

    jika tip == "JIKA" maka
        cg_jika(node)
        kembali 0
    akhir

    jika tip == "PILIH" maka
        cg_pilih(node)
        kembali 0
    akhir

    jika tip == "COBA" maka
        cg_coba(node)
        kembali 0
    akhir

    jika tip == "SELAMA" maka
        cg_selama(node)
        kembali 0
    akhir

    jika tip == "ULANG" maka
        cg_ulang(node)
        kembali 0
    akhir

    jika tip == "UNTUK" maka
        cg_untuk(node)
        kembali 0
    akhir

    jika tip == "LEMPAR" maka
        cg_expr(node[1])
        cg_emit("    mov  rdi, rax")
        cg_emit("    call __lemparCek")
        cg_emit("    jmp  __lempar")
        kembali 0
    akhir

    jika tip == "KEMBALI" maka
        cg_expr(node[1])
        jika CG_func_end != "" maka
            cg_unwind_ke(0)
            cg_emit("    jmp  #{CG_func_end}")
        akhir
        kembali 0
    akhir

    jika tip == "HENTIKAN" maka
        angka bsz = panjang(CG_break_stk)
        jika bsz > 0 maka
            teks blbl = CG_break_stk[bsz - 1]
            cg_unwind_ke(CG_loop_base_stk[bsz - 1])
            cg_emit("    jmp  #{blbl}")
        selain
            cg_emit("    ; hentikan di luar loop - diabaikan")
        akhir
        kembali 0
    akhir

    jika tip == "LEWATI" maka
        angka csz = panjang(CG_continue_stk)
        jika csz > 0 maka
            teks clbl = CG_continue_stk[csz - 1]
            cg_unwind_ke(CG_loop_base_stk[csz - 1])
            cg_emit("    jmp  #{clbl}")
        selain
            cg_emit("    ; lewati di luar loop - diabaikan")
        akhir
        kembali 0
    akhir

    jika tip == "FUNGSI" maka
        cg_fungsi(node)
        kembali 0
    akhir

    cg_expr(node)
}

// Handler galat: catatan 24 byte di stack [prev][rbp tersimpan][alamat catch],
// __handler_top menunjuk ke catatan terbaru. __lempar memulihkan rsp/rbp lalu
// melompat ke alamat catch dengan objek galat di rax.
fungsi cg_handler_push(lcatch) {
    cg_emit("    lea  rax, [rel #{lcatch}]")
    cg_emit("    push rax")
    cg_emit("    push rbp")
    cg_emit("    push qword [rel __handler_top]")
    cg_emit("    mov  qword [rel __handler_top], rsp")
}

fungsi cg_handler_pop() {
    cg_emit("    pop  qword [rel __handler_top]")
    cg_emit("    add  rsp, 16")
}

// Emit kode pelepasan untuk keluar dari wilayah coba (kembali/hentikan/lewati):
// dari wilayah terdalam sampai (tidak termasuk) indeks basis, lepas handler dan
// jalankan badan akhirnya secara inline. Nilai di rax dijaga.
fungsi cg_unwind_ke(basis) {
    larik simpan = CG_cleanup_stk
    angka i = panjang(simpan) - 1
    selama i >= basis {
        larik item = simpan[i]
        jika item[0] == 1 maka
            cg_handler_pop()
        akhir
        jika item[1] == 1 maka
            jika i == 0 maka
                CG_cleanup_stk = []
            selain
                CG_cleanup_stk = irisLarik(simpan, 0, i - 1)
            akhir
            cg_emit("    push rax")
            larik fb = item[2]
            angka fj = 0
            selama fj < panjang(fb) {
                cg_stmt(fb[fj])
                fj += 1
            }
            cg_emit("    pop  rax")
        akhir
        i -= 1
    }
    CG_cleanup_stk = simpan
}

fungsi cg_coba_stmts(daftar) {
    angka k = 0
    selama k < panjang(daftar) {
        cg_stmt(daftar[k])
        k += 1
    }
}

fungsi cg_coba_pop_cleanup() {
    angka sz = panjang(CG_cleanup_stk)
    jika sz <= 1 maka
        CG_cleanup_stk = []
    selain
        CG_cleanup_stk = irisLarik(CG_cleanup_stk, 0, sz - 2)
    akhir
}

fungsi cg_coba(node) {
    larik coba_body    = node[1]
    larik tangkap_body = node[2]
    angka ada_t = 1
    larik fin_body = []
    angka ada_a = 0
    jika panjang(node) >= 7 maka
        ada_t    = node[4]
        fin_body = node[5]
        ada_a    = node[6]
    akhir
    teks lid   = cg_label_baru()
    teks lc    = "__cc_#{lid}"
    teks lc2   = "__c2_#{lid}"
    teks lfin  = "__cf_#{lid}"
    teks lfr   = "__cr_#{lid}"
    teks lend  = "__ce_#{lid}"
    angka tmp  = cg_alloc("__galat_#{lid}")

    // badan coba: handler aktif; akhirnya ikut berjalan bila keluar lewat kembali/hentikan/lewati
    cg_handler_push(lc)
    CG_cleanup_stk = tambah(CG_cleanup_stk, [1, ada_a, fin_body])
    cg_coba_stmts(coba_body)
    cg_coba_pop_cleanup()
    cg_handler_pop()
    cg_emit("    jmp  #{lfin}")

    // galat tertangkap: rsp/rbp sudah dipulihkan, handler sudah dilepas, rax = objek galat
    cg_emit("#{lc}:")
    jika ada_t == 1 maka
        cg_var_store("e")
        jika ada_a == 1 maka
            cg_handler_push(lc2)
            CG_cleanup_stk = tambah(CG_cleanup_stk, [1, 1, fin_body])
        akhir
        cg_coba_stmts(tangkap_body)
        jika ada_a == 1 maka
            cg_coba_pop_cleanup()
            cg_handler_pop()
        akhir
        cg_emit("    jmp  #{lfin}")
        jika ada_a == 1 maka
            cg_emit("#{lc2}:")
            cg_emit("    mov  qword [rbp#{fmt_off(tmp)}], rax")
            cg_emit("    jmp  #{lfr}")
        akhir
    selain
        cg_emit("    mov  qword [rbp#{fmt_off(tmp)}], rax")
        cg_emit("    jmp  #{lfr}")
    akhir

    // jalur normal
    cg_emit("#{lfin}:")
    jika ada_a == 1 maka
        cg_coba_stmts(fin_body)
    akhir
    cg_emit("    jmp  #{lend}")

    // jalur galat: jalankan akhirnya lalu lempar ulang galat yang tertunda
    jika ada_a == 1 maka
        cg_emit("#{lfr}:")
        cg_coba_stmts(fin_body)
        cg_emit("    mov  rax, qword [rbp#{fmt_off(tmp)}]")
        cg_emit("    jmp  __lempar")
    akhir
    cg_emit("#{lend}:")
}

fungsi cg_pilih(node) {
    larik eksp        = node[1]
    larik daftar_ks   = node[2]
    milik default_    = node[3]
    teks  lid  = cg_label_baru()
    teks  lend = "__pe_#{lid}"

    cg_expr(eksp)
    angka tmp_off = cg_alloc("__pilih_#{lid}")
    cg_emit("    mov  qword [rbp#{fmt_off(tmp_off)}], rax")

    angka i = 0
    angka n = panjang(daftar_ks)
    selama i < n {
        larik pasangan  = daftar_ks[i]
        larik val_node  = pasangan[0]
        larik stmt_node = pasangan[1]
        teks  lnext = "__pn_#{lid}_#{angkaTeks(i)}"

        cg_emit("    mov  rax, qword [rbp#{fmt_off(tmp_off)}]")
        cg_emit("    push rax   ; nilai pilih")
        cg_expr(val_node)
        cg_emit("    mov  rsi, rax")
        cg_emit("    pop  rdi   ; nilai pilih")
        cg_emit("    call __setara")
        cg_emit("    cmp  rax, 1   ; 1 = tagged-false")
        cg_emit("    je   #{lnext}")
        cg_stmt(stmt_node)
        cg_emit("    jmp  #{lend}")
        cg_emit("#{lnext}:")
        i += 1
    }

    jika default_ != 0 maka
        cg_stmt(default_)
    akhir
    cg_emit("#{lend}:")
}

fungsi cg_jika(node) {
    larik kond   = node[1]
    larik body_t = node[2]
    larik body_f = node[3]
    teks lid     = cg_label_baru()
    teks lelse   = "__else_#{lid}"
    teks lend    = "__fi_#{lid}"

    cg_expr(kond)
    cg_emit("    cmp  rax, 1   ; 1 = tagged-false")
    jika panjang(body_f) > 0 maka
        cg_emit("    je   #{lelse}")
    selain
        cg_emit("    je   #{lend}")
    akhir

    angka i = 0
    selama i < panjang(body_t) {
        cg_stmt(body_t[i])
        i += 1
    }

    jika panjang(body_f) > 0 maka
        cg_emit("    jmp  #{lend}")
        cg_emit("#{lelse}:")
        angka j = 0
        selama j < panjang(body_f) {
            cg_stmt(body_f[j])
            j += 1
        }
    akhir
    cg_emit("#{lend}:")
}

fungsi cg_selama(node) {
    larik kond = node[1]
    larik body = node[2]
    teks lid   = cg_label_baru()
    teks lcek  = "__wl_#{lid}"
    teks lend  = "__we_#{lid}"

    CG_break_stk    = tambah(CG_break_stk, lend)
    CG_loop_base_stk = tambah(CG_loop_base_stk, panjang(CG_cleanup_stk))
    CG_continue_stk = tambah(CG_continue_stk, lcek)

    cg_emit("#{lcek}:")
    cg_expr(kond)
    cg_emit("    cmp  rax, 1   ; 1 = tagged-false")
    cg_emit("    je   #{lend}")

    angka i = 0
    selama i < panjang(body) {
        cg_stmt(body[i])
        i += 1
    }
    cg_emit("    jmp  #{lcek}")
    cg_emit("#{lend}:")

    angka bsz2 = panjang(CG_break_stk)
    angka csz2 = panjang(CG_continue_stk)
    CG_break_stk    = irisLarik(CG_break_stk, 0, bsz2 - 2)
    CG_loop_base_stk = irisLarik(CG_loop_base_stk, 0, bsz2 - 2)
    CG_continue_stk = irisLarik(CG_continue_stk, 0, csz2 - 2)
}

fungsi cg_ulang(node) {
    larik hitungan = node[1]
    larik body     = node[2]
    teks lid       = cg_label_baru()
    teks lctr_nm   = "__ctr_#{lid}"
    teks lcek      = "__ul_#{lid}"
    teks lend      = "__ue_#{lid}"

    cg_expr(hitungan)
    cg_emit("    sar  rax, 1   ; lepas tag hitungan ulang")
    angka off = cg_alloc(lctr_nm)
    cg_emit("    mov  qword [rbp#{fmt_off(off)}], rax")

    CG_break_stk    = tambah(CG_break_stk, lend)
    CG_loop_base_stk = tambah(CG_loop_base_stk, panjang(CG_cleanup_stk))
    CG_continue_stk = tambah(CG_continue_stk, lcek)

    cg_emit("#{lcek}:")
    cg_emit("    cmp  qword [rbp#{fmt_off(off)}], 0")
    cg_emit("    jle  #{lend}")

    angka i = 0
    selama i < panjang(body) {
        cg_stmt(body[i])
        i += 1
    }
    cg_emit("    dec  qword [rbp#{fmt_off(off)}]")
    cg_emit("    jmp  #{lcek}")
    cg_emit("#{lend}:")

    angka bsz3 = panjang(CG_break_stk)
    angka csz3 = panjang(CG_continue_stk)
    CG_break_stk    = irisLarik(CG_break_stk, 0, bsz3 - 2)
    CG_loop_base_stk = irisLarik(CG_loop_base_stk, 0, bsz3 - 2)
    CG_continue_stk = irisLarik(CG_continue_stk, 0, csz3 - 2)
}

fungsi cg_untuk(node) {
    teks var_nm = node[1]
    larik iter  = node[2]
    larik body  = node[3]
    teks lid    = cg_label_baru()
    teks lptr   = "__u_ptr_#{lid}"
    teks li     = "__u_i_#{lid}"
    teks ln     = "__u_n_#{lid}"
    teks lcek   = "__ul_#{lid}"
    teks lend   = "__ue_#{lid}"

    cg_expr(iter)
    angka ptr_off = cg_alloc(lptr)
    cg_emit("    mov  qword [rbp#{fmt_off(ptr_off)}], rax")

    angka n_iter = 0
    jika iter[0] == "ARRAY" maka
        n_iter = panjang(iter[1])
    akhir

    angka i_off = cg_alloc(li)
    cg_emit("    mov  qword [rbp#{fmt_off(i_off)}], 0")

    CG_break_stk    = tambah(CG_break_stk, lend)
    CG_loop_base_stk = tambah(CG_loop_base_stk, panjang(CG_cleanup_stk))
    CG_continue_stk = tambah(CG_continue_stk, lcek)

    cg_emit("#{lcek}:")
    cg_emit("    cmp  qword [rbp#{fmt_off(i_off)}], #{angkaTeks(n_iter)}")
    cg_emit("    jge  #{lend}")

    cg_emit("    mov  rax, qword [rbp#{fmt_off(ptr_off)}]")
    cg_emit("    mov  rcx, qword [rbp#{fmt_off(i_off)}]")
    cg_emit("    imul rcx, 8")
    cg_emit("    add  rax, rcx")
    cg_emit("    add  rax, 24   ; lewati header [MAGIC][PANJANG][KAPASITAS] - bug lama, belum pernah dilewati")
    cg_emit("    mov  rax, qword [rax]")

    angka var_off = cg_alloc(var_nm)
    cg_emit("    mov  qword [rbp#{fmt_off(var_off)}], rax")

    angka bi = 0
    selama bi < panjang(body) {
        cg_stmt(body[bi])
        bi += 1
    }

    cg_emit("    inc  qword [rbp#{fmt_off(i_off)}]")
    cg_emit("    jmp  #{lcek}")
    cg_emit("#{lend}:")

    angka bsz4 = panjang(CG_break_stk)
    angka csz4 = panjang(CG_continue_stk)
    CG_break_stk    = irisLarik(CG_break_stk, 0, bsz4 - 2)
    CG_loop_base_stk = irisLarik(CG_loop_base_stk, 0, bsz4 - 2)
    CG_continue_stk = irisLarik(CG_continue_stk, 0, csz4 - 2)
}

fungsi cg_fungsi(node) {
    teks nama    = node[1]
    larik params = node[2]
    larik body   = node[3]

    larik vars_lama  = CG_vars
    angka stack_lama = CG_stack
    teks  fend_lama  = CG_func_end
    larik bstk_lama  = CG_break_stk
    larik cstk_lama  = CG_continue_stk
    larik cleanup_lama = CG_cleanup_stk
    larik lbase_lama   = CG_loop_base_stk

    CG_vars         = []
    CG_stack        = 0
    CG_func_end     = "__fe_#{nama}"
    CG_break_stk    = []
    CG_continue_stk = []
    CG_cleanup_stk  = []
    CG_loop_base_stk = []
    CG_in_fungsi    = 1

    cg_emit("")
    cg_emit("; fungsi #{nama}")
    cg_emit("#{nama}:")
    cg_emit("    push rbp")
    cg_emit("    mov  rbp, rsp")
    cg_emit("    sub  rsp, 4096")

    angka i = 0
    selama i < panjang(params) dan i < 6 {
        angka off = cg_alloc(params[i])
        cg_emit("    mov  qword [rbp#{fmt_off(off)}], #{ABI_REGS[i]}")
        i += 1
    }

    angka j = 0
    selama j < panjang(body) {
        cg_stmt(body[j])
        j += 1
    }

    cg_emit("    xor  rax, rax")
    cg_emit("#{CG_func_end}:")
    cg_emit("    leave")
    cg_emit("    ret")

    CG_vars         = vars_lama
    CG_stack        = stack_lama
    CG_func_end     = fend_lama
    CG_break_stk    = bstk_lama
    CG_continue_stk = cstk_lama
    CG_cleanup_stk  = cleanup_lama
    CG_loop_base_stk = lbase_lama
    CG_in_fungsi    = 0
}

fungsi cg_emit_runtime() {
    cg_emit("")
    cg_emit("; ---- runtime ----")
    cg_emit("")
    cg_emit("__print_int:")
    cg_emit("    push rbp")
    cg_emit("    mov  rbp, rsp")
    cg_emit("    push rbx")
    cg_emit("    push r12")
    cg_emit("    push r13")
    cg_emit("    sar  rdi, 1   ; lepas tag angka")
    cg_emit("    mov  rax, rdi")
    cg_emit("    lea  rbx, [rel __intbuf + 31]")
    cg_emit("    mov  byte [rbx], 0x0A")
    cg_emit("    dec  rbx")
    cg_emit("    xor  r12, r12")
    cg_emit("    test rax, rax")
    cg_emit("    jge  .pi_pos")
    cg_emit("    mov  r12, 1")
    cg_emit("    neg  rax")
    cg_emit(".pi_pos:")
    cg_emit("    mov  r13, 10")
    cg_emit(".pi_lp:")
    cg_emit("    xor  rdx, rdx")
    cg_emit("    div  r13")
    cg_emit("    add  dl, 0x30")
    cg_emit("    mov  [rbx], dl")
    cg_emit("    dec  rbx")
    cg_emit("    test rax, rax")
    cg_emit("    jnz  .pi_lp")
    cg_emit("    test r12, r12")
    cg_emit("    jz   .pi_done")
    cg_emit("    mov  byte [rbx], 0x2D")
    cg_emit("    dec  rbx")
    cg_emit(".pi_done:")
    cg_emit("    inc  rbx")
    cg_emit("    mov  rsi, rbx")
    cg_emit("    lea  rdx, [rel __intbuf + 32]")
    cg_emit("    sub  rdx, rbx")
    cg_emit("    mov  rax, 1")
    cg_emit("    mov  rdi, 1")
    cg_emit("    syscall")
    cg_emit("    pop  r13")
    cg_emit("    pop  r12")
    cg_emit("    pop  rbx")
    cg_emit("    leave")
    cg_emit("    ret")
    cg_emit("")
    cg_emit("__strlen:")
    cg_emit("    xor  rax, rax")
    cg_emit(".sl_lp:")
    cg_emit("    cmp  byte [rdi + rax], 0")
    cg_emit("    jz   .sl_done")
    cg_emit("    inc  rax")
    cg_emit("    jmp  .sl_lp")
    cg_emit(".sl_done:")
    cg_emit("    ret")
    cg_emit("")
    cg_emit("__print_str_nl:")
    cg_emit("    push rbp")
    cg_emit("    mov  rbp, rsp")
    cg_emit("    push rbx")
    cg_emit("    mov  rbx, rdi")
    cg_emit("    call __strlen")
    cg_emit("    mov  rdx, rax")
    cg_emit("    mov  rsi, rbx")
    cg_emit("    mov  rax, 1")
    cg_emit("    mov  rdi, 1")
    cg_emit("    syscall")
    cg_emit("    mov  rax, 1")
    cg_emit("    mov  rdi, 1")
    cg_emit("    lea  rsi, [rel __nl]")
    cg_emit("    mov  rdx, 1")
    cg_emit("    syscall")
    cg_emit("    pop  rbx")
    cg_emit("    leave")
    cg_emit("    ret")
    cg_emit("")
    cg_emit("__print_auto:")
    cg_emit("    push rbp")
    cg_emit("    mov  rbp, rsp")
    cg_emit("    call __ke_teks   ; fix: dulu selalu panggil __print_int, cetak string jadi salah")
    cg_emit("    mov  rdi, rax")
    cg_emit("    call __print_str_nl")
    cg_emit("    leave")
    cg_emit("    ret")
    cg_emit("")

    cg_emit("__ke_teks:")
    cg_emit("    test rdi, 1   ; ganjil = angka tertag")
    cg_emit("    jnz  .kt_num")
    cg_emit("    mov  rax, rdi")
    cg_emit("    ret")
    cg_emit(".kt_num:")
    cg_emit("    jmp  angkaTeks")
    cg_emit("")

    cg_emit("__setara:")
    cg_emit("    cmp  rdi, rsi")
    cg_emit("    je   .st_true")
    cg_emit("    test rdi, 1   ; ganjil = angka tertag, tak mungkin sama tapi beda bit")
    cg_emit("    jnz  .st_false")
    cg_emit("    test rsi, 1")
    cg_emit("    jnz  .st_false")
    cg_emit("    push rbx")
    cg_emit("    xor  rcx, rcx")
    cg_emit(".st_lp:")
    cg_emit("    mov  al, byte [rdi + rcx]")
    cg_emit("    mov  bl, byte [rsi + rcx]")
    cg_emit("    cmp  al, bl")
    cg_emit("    jne  .st_diff")
    cg_emit("    cmp  al, 0")
    cg_emit("    je   .st_same")
    cg_emit("    inc  rcx")
    cg_emit("    jmp  .st_lp")
    cg_emit(".st_diff:")
    cg_emit("    pop  rbx")
    cg_emit("    mov  rax, 1   ; tagged false")
    cg_emit("    ret")
    cg_emit(".st_same:")
    cg_emit("    pop  rbx")
    cg_emit("    mov  rax, 3   ; tagged true")
    cg_emit("    ret")
    cg_emit(".st_false:")
    cg_emit("    mov  rax, 1   ; tagged false")
    cg_emit("    ret")
    cg_emit(".st_true:")
    cg_emit("    mov  rax, 3   ; tagged true")
    cg_emit("    ret")
    cg_emit("")

    cg_emit("__alloc:")
    cg_emit("    push rbx")
    cg_emit("    mov  rbx, rdi")
    cg_emit("    add  rbx, 7")
    cg_emit("    and  rbx, -8")
    cg_emit("    mov  rax, qword [rel __heap_ptr]")
    cg_emit("    mov  rcx, rax")
    cg_emit("    add  rcx, rbx")
    cg_emit("    mov  qword [rel __heap_ptr], rcx")
    cg_emit("    pop  rbx")
    cg_emit("    ret")
    cg_emit("")

    cg_emit("__panjang:")
    cg_emit("    mov  rax, qword [rdi]")
    cg_emit("    cmp  rax, 0x4C4F4F50")
    cg_emit("    je   .pj_arr")
    cg_emit("    xor  rax, rax")
    cg_emit(".pj_sl:")
    cg_emit("    cmp  byte [rdi + rax], 0")
    cg_emit("    jz   .pj_done")
    cg_emit("    inc  rax")
    cg_emit("    jmp  .pj_sl")
    cg_emit(".pj_arr:")
    cg_emit("    mov  rax, qword [rdi + 8]")
    cg_emit(".pj_done:")
    cg_emit("    lea  rax, [rax*2 + 1]   ; re-tag hasil panjang()")
    cg_emit("    ret")
    cg_emit("")

    cg_emit("__memcpy:")
    cg_emit("    xor  rcx, rcx")
    cg_emit(".mc_lp:")
    cg_emit("    cmp  rcx, rdx")
    cg_emit("    jge  .mc_done")
    cg_emit("    mov  al, byte [rsi + rcx]")
    cg_emit("    mov  byte [rdi + rcx], al")
    cg_emit("    inc  rcx")
    cg_emit("    jmp  .mc_lp")
    cg_emit(".mc_done:")
    cg_emit("    ret")
    cg_emit("")

    cg_emit("__strcpy_adv:")
    cg_emit("    push rcx")
    cg_emit("    xor  rcx, rcx")
    cg_emit(".sca_lp:")
    cg_emit("    mov  al, byte [rsi + rcx]")
    cg_emit("    cmp  al, 0")
    cg_emit("    je   .sca_done")
    cg_emit("    mov  byte [rdi + rcx], al")
    cg_emit("    inc  rcx")
    cg_emit("    jmp  .sca_lp")
    cg_emit(".sca_done:")
    cg_emit("    lea  rax, [rdi + rcx]")
    cg_emit("    pop  rcx")
    cg_emit("    ret")
    cg_emit("")

    cg_emit("__arr_append:")
    cg_emit("    push rbx")
    cg_emit("    push r12")
    cg_emit("    push r13")
    cg_emit("    push r14")
    cg_emit("    mov  r12, rdi")
    cg_emit("    mov  r13, rsi")
    cg_emit("    mov  r14, qword [r12 + 8]")
    cg_emit("    mov  rax, qword [r12 + 16]")
    cg_emit("    cmp  r14, rax")
    cg_emit("    jl   .aa_inplace")
    cg_emit("    mov  rbx, rax")
    cg_emit("    imul rbx, 2")
    cg_emit("    cmp  rbx, 4")
    cg_emit("    jge  .aa_capok")
    cg_emit("    mov  rbx, 4")
    cg_emit(".aa_capok:")
    cg_emit("    mov  rdi, rbx")
    cg_emit("    imul rdi, 8")
    cg_emit("    add  rdi, 24")
    cg_emit("    call __alloc")
    cg_emit("    mov  qword [rax], 0x4C4F4F50")
    cg_emit("    mov  qword [rax + 8], r14")
    cg_emit("    mov  qword [rax + 16], rbx")
    cg_emit("    xor  rcx, rcx")
    cg_emit(".aa_copy:")
    cg_emit("    cmp  rcx, r14")
    cg_emit("    jge  .aa_copied")
    cg_emit("    mov  rdx, qword [r12 + 24 + rcx*8]")
    cg_emit("    mov  qword [rax + 24 + rcx*8], rdx")
    cg_emit("    inc  rcx")
    cg_emit("    jmp  .aa_copy")
    cg_emit(".aa_copied:")
    cg_emit("    mov  r12, rax")
    cg_emit(".aa_inplace:")
    cg_emit("    mov  qword [r12 + 24 + r14*8], r13")
    cg_emit("    mov  rax, r14")
    cg_emit("    inc  rax")
    cg_emit("    mov  qword [r12 + 8], rax")
    cg_emit("    mov  rax, r12")
    cg_emit("    pop  r14")
    cg_emit("    pop  r13")
    cg_emit("    pop  r12")
    cg_emit("    pop  rbx")
    cg_emit("    ret")
    cg_emit("")

    cg_emit("__str_concat:")
    cg_emit("    push rbx")
    cg_emit("    push r12")
    cg_emit("    push r13")
    cg_emit("    push r14")
    cg_emit("    mov  r12, rdi")
    cg_emit("    mov  r13, rsi")
    cg_emit("    call __strlen")
    cg_emit("    mov  r14, rax")
    cg_emit("    mov  rdi, r13")
    cg_emit("    call __strlen")
    cg_emit("    mov  rbx, rax")
    cg_emit("    mov  rdi, r14")
    cg_emit("    add  rdi, rbx")
    cg_emit("    inc  rdi")
    cg_emit("    call __alloc")
    cg_emit("    push rax")
    cg_emit("    mov  rdi, rax")
    cg_emit("    mov  rsi, r12")
    cg_emit("    mov  rdx, r14")
    cg_emit("    call __memcpy")
    cg_emit("    pop  rax")
    cg_emit("    push rax")
    cg_emit("    mov  rdi, rax")
    cg_emit("    add  rdi, r14")
    cg_emit("    mov  rsi, r13")
    cg_emit("    mov  rdx, rbx")
    cg_emit("    inc  rdx")
    cg_emit("    call __memcpy")
    cg_emit("    pop  rax")
    cg_emit("    pop  r14")
    cg_emit("    pop  r13")
    cg_emit("    pop  r12")
    cg_emit("    pop  rbx")
    cg_emit("    ret")
    cg_emit("")

    cg_emit("__tambah:")
    cg_emit("    test rdi, 1   ; ganjil = angka tertag")
    cg_emit("    jnz  .ta_num")
    cg_emit("    mov  rax, qword [rdi]")
    cg_emit("    cmp  rax, 0x4C4F4F50")
    cg_emit("    je   .ta_arr")
    cg_emit("    jmp  .ta_str")
    cg_emit(".ta_num:")
    cg_emit("    mov  rax, rdi")
    cg_emit("    add  rax, rsi")
    cg_emit("    dec  rax   ; re-tag jumlah")
    cg_emit("    ret")
    cg_emit(".ta_arr:")
    cg_emit("    jmp  __arr_append")
    cg_emit(".ta_str:")
    cg_emit("    jmp  __str_concat")
    cg_emit("")

    cg_emit("irisLarik:")
    cg_emit("    push rbx")
    cg_emit("    push r12")
    cg_emit("    push r13")
    cg_emit("    push r14")
    cg_emit("    mov  r12, rdi")
    cg_emit("    mov  r13, rsi")
    cg_emit("    sar  r13, 1   ; lepas tag mulai")
    cg_emit("    mov  r14, rdx")
    cg_emit("    sar  r14, 1   ; lepas tag akhir")
    cg_emit("    cmp  r13, 0")
    cg_emit("    jge  .il_a_ok")
    cg_emit("    xor  r13, r13")
    cg_emit(".il_a_ok:")
    cg_emit("    mov  rax, qword [r12 + 8]")
    cg_emit("    dec  rax")
    cg_emit("    cmp  r14, rax")
    cg_emit("    jle  .il_b_ok")
    cg_emit("    mov  r14, rax")
    cg_emit(".il_b_ok:")
    cg_emit("    mov  rbx, r14")
    cg_emit("    sub  rbx, r13")
    cg_emit("    inc  rbx")
    cg_emit("    cmp  rbx, 0")
    cg_emit("    jg   .il_sz_ok")
    cg_emit("    xor  rbx, rbx")
    cg_emit(".il_sz_ok:")
    cg_emit("    push rbx")
    cg_emit("    mov  rdi, rbx")
    cg_emit("    imul rdi, 8")
    cg_emit("    add  rdi, 24")
    cg_emit("    call __alloc")
    cg_emit("    pop  rbx")
    cg_emit("    mov  qword [rax], 0x4C4F4F50")
    cg_emit("    mov  qword [rax + 8], rbx")
    cg_emit("    mov  qword [rax + 16], rbx")
    cg_emit("    xor  rcx, rcx")
    cg_emit(".il_copy:")
    cg_emit("    cmp  rcx, rbx")
    cg_emit("    jge  .il_done")
    cg_emit("    mov  rdx, r13")
    cg_emit("    add  rdx, rcx")
    cg_emit("    mov  rsi, qword [r12 + 24 + rdx*8]")
    cg_emit("    mov  qword [rax + 24 + rcx*8], rsi")
    cg_emit("    inc  rcx")
    cg_emit("    jmp  .il_copy")
    cg_emit(".il_done:")
    cg_emit("    pop  r14")
    cg_emit("    pop  r13")
    cg_emit("    pop  r12")
    cg_emit("    pop  rbx")
    cg_emit("    ret")
    cg_emit("")

    cg_emit("gabungTeks:")
    cg_emit("    push rbx")
    cg_emit("    push r12")
    cg_emit("    push r13")
    cg_emit("    push r14")
    cg_emit("    push r15")
    cg_emit("    mov  r12, rdi")
    cg_emit("    mov  r13, rsi")
    cg_emit("    mov  r14, qword [r12 + 8]")
    cg_emit("    mov  rdi, r13")
    cg_emit("    call __strlen")
    cg_emit("    mov  r15, rax")
    cg_emit("    xor  rbx, rbx")
    cg_emit("    xor  rcx, rcx")
    cg_emit(".gt_len_lp:")
    cg_emit("    cmp  rcx, r14")
    cg_emit("    jge  .gt_len_done")
    cg_emit("    mov  rdi, qword [r12 + 24 + rcx*8]")
    cg_emit("    push rcx")
    cg_emit("    call __ke_teks   ; fix: auto-konversi elemen non-teks (angka), dulu crash")
    cg_emit("    mov  rdi, rax")
    cg_emit("    call __strlen")
    cg_emit("    pop  rcx")
    cg_emit("    add  rbx, rax")
    cg_emit("    inc  rcx")
    cg_emit("    cmp  rcx, r14")
    cg_emit("    jl   .gt_len_addsep")
    cg_emit("    jmp  .gt_len_lp")
    cg_emit(".gt_len_addsep:")
    cg_emit("    add  rbx, r15")
    cg_emit("    jmp  .gt_len_lp")
    cg_emit(".gt_len_done:")
    cg_emit("    inc  rbx")
    cg_emit("    mov  rdi, rbx")
    cg_emit("    call __alloc")
    cg_emit("    mov  rbx, rax")
    cg_emit("    push rax")
    cg_emit("    xor  rcx, rcx")
    cg_emit(".gt_copy_lp:")
    cg_emit("    cmp  rcx, r14")
    cg_emit("    jge  .gt_copy_done")
    cg_emit("    mov  rdi, qword [r12 + 24 + rcx*8]")
    cg_emit("    push rcx")
    cg_emit("    call __ke_teks   ; fix: auto-konversi elemen non-teks (angka), dulu crash")
    cg_emit("    mov  rsi, rax")
    cg_emit("    mov  rdi, rbx")
    cg_emit("    call __strcpy_adv")
    cg_emit("    mov  rbx, rax")
    cg_emit("    pop  rcx")
    cg_emit("    inc  rcx")
    cg_emit("    cmp  rcx, r14")
    cg_emit("    jge  .gt_after_sep")
    cg_emit("    mov  rdi, rbx")
    cg_emit("    mov  rsi, r13")
    cg_emit("    push rcx")
    cg_emit("    call __strcpy_adv")
    cg_emit("    mov  rbx, rax")
    cg_emit("    pop  rcx")
    cg_emit(".gt_after_sep:")
    cg_emit("    jmp  .gt_copy_lp")
    cg_emit(".gt_copy_done:")
    cg_emit("    mov  byte [rbx], 0")
    cg_emit("    pop  rax")
    cg_emit("    pop  r15")
    cg_emit("    pop  r14")
    cg_emit("    pop  r13")
    cg_emit("    pop  r12")
    cg_emit("    pop  rbx")
    cg_emit("    ret")
    cg_emit("")

    cg_emit("kodeKarakter:")
    cg_emit("    xor  rax, rax")
    cg_emit("    mov  al, byte [rdi]")
    cg_emit("    lea  rax, [rax*2 + 1]   ; re-tag kode karakter")
    cg_emit("    ret")
    cg_emit("")

    cg_emit("karakter:")
    cg_emit("    push rbx")
    cg_emit("    push r12")
    cg_emit("    mov  r12, rdi")
    cg_emit("    mov  rbx, rsi")
    cg_emit("    sar  rbx, 1   ; lepas tag indeks")
    cg_emit("    call __strlen")
    cg_emit("    cmp  rbx, 0")
    cg_emit("    jl   .kr_empty")
    cg_emit("    cmp  rbx, rax")
    cg_emit("    jge  .kr_empty")
    cg_emit("    mov  rdi, 2")
    cg_emit("    call __alloc")
    cg_emit("    mov  cl, byte [r12 + rbx]")
    cg_emit("    mov  byte [rax], cl")
    cg_emit("    mov  byte [rax + 1], 0")
    cg_emit("    pop  r12")
    cg_emit("    pop  rbx")
    cg_emit("    ret")
    cg_emit(".kr_empty:")
    cg_emit("    mov  rdi, 1")
    cg_emit("    call __alloc")
    cg_emit("    mov  byte [rax], 0")
    cg_emit("    pop  r12")
    cg_emit("    pop  rbx")
    cg_emit("    ret")
    cg_emit("")

    cg_emit("__lempar:")
    cg_emit("    mov  rcx, qword [rel __handler_top]")
    cg_emit("    test rcx, rcx")
    cg_emit("    jz   .lp_tak")
    cg_emit("    mov  rsp, rcx   ; buang semua frame di atas catatan handler")
    cg_emit("    pop  qword [rel __handler_top]")
    cg_emit("    pop  rbp")
    cg_emit("    pop  rdx")
    cg_emit("    jmp  rdx        ; rax = objek galat")
    cg_emit(".lp_tak:")
    cg_emit("    mov  rdi, rax")
    cg_emit("    call __galatTakTertangkap   ; cetak ke stderr dan keluar kode 1")
    cg_emit("    mov  rax, 60")
    cg_emit("    mov  rdi, 1")
    cg_emit("    syscall")
    cg_emit("")

    cg_emit("; rax = larik, rcx = indeks (tanpa tag). Melempar bila bukan larik / di luar batas.")
    cg_emit("__idx_cek:")
    cg_emit("    test rax, rax")
    cg_emit("    jz   .ic_tipe")
    cg_emit("    test al, 1")
    cg_emit("    jnz  .ic_tipe")
    cg_emit("    cmp  qword [rax], 0x4C4F4F50")
    cg_emit("    jne  .ic_tipe")
    cg_emit("    cmp  rcx, qword [rax + 8]")
    cg_emit("    jae  .ic_idx   ; tak bertanda: indeks negatif ikut gagal")
    cg_emit("    ret")
    cg_emit(".ic_tipe:")
    cg_emit("    call __galatIndeksTipe")
    cg_emit("    jmp  __lempar")
    cg_emit(".ic_idx:")
    cg_emit("    call __galatIndeks")
    cg_emit("    jmp  __lempar")
    cg_emit("")

    cg_emit("tulisStderr:")
    cg_emit("    push rbp")
    cg_emit("    mov  rbp, rsp")
    cg_emit("    push rbx")
    cg_emit("    mov  rbx, rdi")
    cg_emit("    call __strlen")
    cg_emit("    mov  rdx, rax")
    cg_emit("    mov  rsi, rbx")
    cg_emit("    mov  rax, 1")
    cg_emit("    mov  rdi, 2")
    cg_emit("    syscall")
    cg_emit("    mov  rax, 1")
    cg_emit("    mov  rdi, 2")
    cg_emit("    lea  rsi, [rel __nl]")
    cg_emit("    mov  rdx, 1")
    cg_emit("    syscall")
    cg_emit("    pop  rbx")
    cg_emit("    leave")
    cg_emit("    ret")
    cg_emit("")

    cg_emit("dariKode:")
    cg_emit("    push rbx")
    cg_emit("    mov  rbx, rdi")
    cg_emit("    sar  rbx, 1   ; lepas tag kode")
    cg_emit("    mov  rdi, 2")
    cg_emit("    call __alloc")
    cg_emit("    mov  byte [rax], bl")
    cg_emit("    mov  byte [rax + 1], 0")
    cg_emit("    pop  rbx")
    cg_emit("    ret")
    cg_emit("")

    cg_emit("bulatkan:")
    cg_emit("    mov  rax, rdi")
    cg_emit("    ret")
    cg_emit("")

    cg_emit("angkaTeks:")
    cg_emit("    push rbx")
    cg_emit("    push r12")
    cg_emit("    push r13")
    cg_emit("    mov  rbx, rdi")
    cg_emit("    sar  rbx, 1   ; lepas tag angka")
    cg_emit("    mov  rdi, 24")
    cg_emit("    call __alloc")
    cg_emit("    mov  r12, rax")
    cg_emit("    lea  r13, [r12 + 23]")
    cg_emit("    mov  byte [r13], 0")
    cg_emit("    dec  r13")
    cg_emit("    mov  rax, rbx")
    cg_emit("    xor  rcx, rcx")
    cg_emit("    test rax, rax")
    cg_emit("    jge  .at_pos")
    cg_emit("    mov  rcx, 1")
    cg_emit("    neg  rax")
    cg_emit(".at_pos:")
    cg_emit("    mov  rbx, 10")
    cg_emit(".at_lp:")
    cg_emit("    xor  rdx, rdx")
    cg_emit("    div  rbx")
    cg_emit("    add  dl, 0x30")
    cg_emit("    mov  [r13], dl")
    cg_emit("    dec  r13")
    cg_emit("    test rax, rax")
    cg_emit("    jnz  .at_lp")
    cg_emit("    test rcx, rcx")
    cg_emit("    jz   .at_done")
    cg_emit("    mov  byte [r13], 0x2D")
    cg_emit("    dec  r13")
    cg_emit(".at_done:")
    cg_emit("    inc  r13")
    cg_emit("    ; geser string ke awal buffer (r12) - r13 bisa di alamat ganjil")
    cg_emit("    ; tergantung jumlah digit, padahal pointer Loop wajib genap")
    cg_emit("    mov  rdx, r12   ; rdx = kursor tujuan, r12 tetap alamat basis")
    cg_emit(".at_shift:")
    cg_emit("    mov  al, byte [r13]")
    cg_emit("    mov  byte [rdx], al")
    cg_emit("    inc  r13")
    cg_emit("    inc  rdx")
    cg_emit("    test al, al")
    cg_emit("    jnz  .at_shift")
    cg_emit("    mov  rax, r12   ; kembalikan alamat basis (selalu genap)")
    cg_emit("    pop  r13")
    cg_emit("    pop  r12")
    cg_emit("    pop  rbx")
    cg_emit("    ret")
    cg_emit("")

    cg_emit("teksAngka:")
    cg_emit("    push rbx")
    cg_emit("    xor  rax, rax")
    cg_emit("    xor  rcx, rcx")
    cg_emit("    mov  bl, byte [rdi]")
    cg_emit("    cmp  bl, 0x2D")
    cg_emit("    jne  .ta2_lp")
    cg_emit("    mov  rcx, 1")
    cg_emit("    inc  rdi")
    cg_emit(".ta2_lp:")
    cg_emit("    mov  bl, byte [rdi]")
    cg_emit("    cmp  bl, 0")
    cg_emit("    je   .ta2_done")
    cg_emit("    cmp  bl, 0x30")
    cg_emit("    jl   .ta2_done")
    cg_emit("    cmp  bl, 0x39")
    cg_emit("    jg   .ta2_done")
    cg_emit("    imul rax, rax, 10")
    cg_emit("    movzx rbx, bl")
    cg_emit("    sub  rbx, 0x30")
    cg_emit("    add  rax, rbx")
    cg_emit("    inc  rdi")
    cg_emit("    jmp  .ta2_lp")
    cg_emit(".ta2_done:")
    cg_emit("    test rcx, rcx")
    cg_emit("    jz   .ta2_ret")
    cg_emit("    neg  rax")
    cg_emit(".ta2_ret:")
    cg_emit("    lea  rax, [rax*2 + 1]   ; re-tag hasil parse")
    cg_emit("    pop  rbx")
    cg_emit("    ret")
    cg_emit("")

    cg_emit("bacaFileTeks:")
    cg_emit("    push rbx")
    cg_emit("    push r12")
    cg_emit("    push r13")
    cg_emit("    mov  r12, rdi")
    cg_emit("    mov  rax, 2")
    cg_emit("    mov  rdi, r12")
    cg_emit("    xor  rsi, rsi")
    cg_emit("    xor  rdx, rdx")
    cg_emit("    syscall")
    cg_emit("    cmp  rax, 0")
    cg_emit("    jl   .bf_fail")
    cg_emit("    mov  r13, rax")
    cg_emit("    mov  rax, 8")
    cg_emit("    mov  rdi, r13")
    cg_emit("    xor  rsi, rsi")
    cg_emit("    mov  rdx, 2")
    cg_emit("    syscall")
    cg_emit("    mov  rbx, rax")
    cg_emit("    mov  rax, 8")
    cg_emit("    mov  rdi, r13")
    cg_emit("    xor  rsi, rsi")
    cg_emit("    xor  rdx, rdx")
    cg_emit("    syscall")
    cg_emit("    mov  rdi, rbx")
    cg_emit("    inc  rdi")
    cg_emit("    call __alloc")
    cg_emit("    mov  r12, rax")
    cg_emit("    mov  rax, 0")
    cg_emit("    mov  rdi, r13")
    cg_emit("    mov  rsi, r12")
    cg_emit("    mov  rdx, rbx")
    cg_emit("    syscall")
    cg_emit("    mov  byte [r12 + rbx], 0")
    cg_emit("    mov  rax, 3")
    cg_emit("    mov  rdi, r13")
    cg_emit("    syscall")
    cg_emit("    mov  rax, r12")
    cg_emit("    pop  r13")
    cg_emit("    pop  r12")
    cg_emit("    pop  rbx")
    cg_emit("    ret")
    cg_emit(".bf_fail:")
    cg_emit("    call __galatBerkas   ; GalatBerkas, tidak kembali")
    cg_emit("    jmp  __lempar")
    cg_emit("    mov  rdi, 1")
    cg_emit("    call __alloc")
    cg_emit("    mov  byte [rax], 0")
    cg_emit("    pop  r13")
    cg_emit("    pop  r12")
    cg_emit("    pop  rbx")
    cg_emit("    ret")
    cg_emit("")

    cg_emit("tulisFileTeks:")
    cg_emit("    push rbx")
    cg_emit("    push r12")
    cg_emit("    push r13")
    cg_emit("    mov  r12, rdi")
    cg_emit("    mov  r13, rsi")
    cg_emit("    mov  rax, 2")
    cg_emit("    mov  rdi, r12")
    cg_emit("    mov  rsi, 577")
    cg_emit("    mov  rdx, 420")
    cg_emit("    syscall")
    cg_emit("    cmp  rax, 0")
    cg_emit("    jl   .tf_fail")
    cg_emit("    mov  rbx, rax")
    cg_emit("    mov  rdi, r13")
    cg_emit("    call __strlen")
    cg_emit("    mov  rdx, rax")
    cg_emit("    mov  rax, 1")
    cg_emit("    mov  rdi, rbx")
    cg_emit("    mov  rsi, r13")
    cg_emit("    syscall")
    cg_emit("    mov  rax, 3")
    cg_emit("    mov  rdi, rbx")
    cg_emit("    syscall")
    cg_emit("    mov  rax, 3   ; tagged true (sukses)")
    cg_emit("    pop  r13")
    cg_emit("    pop  r12")
    cg_emit("    pop  rbx")
    cg_emit("    ret")
    cg_emit(".tf_fail:")
    cg_emit("    call __galatBerkas   ; GalatBerkas, tidak kembali")
    cg_emit("    jmp  __lempar")
    cg_emit("    mov  rax, 1   ; tagged false (gagal)")
    cg_emit("    pop  r13")
    cg_emit("    pop  r12")
    cg_emit("    pop  rbx")
    cg_emit("    ret")
    cg_emit("")

    cg_emit("_sistemKeluar:")
    cg_emit("    sar  rdi, 1   ; lepas tag kode keluar")
    cg_emit("    mov  rax, 60  ; sys_exit")
    cg_emit("    syscall")
    cg_emit("")

    cg_emit("tidur:")
    cg_emit("    push rbx")
    cg_emit("    sar  rdi, 1        ; lepas tag: total milidetik")
    cg_emit("    mov  rax, rdi")
    cg_emit("    xor  rdx, rdx")
    cg_emit("    mov  rbx, 1000")
    cg_emit("    div  rbx           ; rax = detik, rdx = sisa milidetik")
    cg_emit("    imul rdx, 1000000  ; rdx = nanodetik")
    cg_emit("    sub  rsp, 16       ; struct timespec { long tv_sec; long tv_nsec; }")
    cg_emit("    mov  qword [rsp], rax")
    cg_emit("    mov  qword [rsp + 8], rdx")
    cg_emit("    mov  rdi, rsp")
    cg_emit("    xor  rsi, rsi")
    cg_emit("    mov  rax, 35       ; sys_nanosleep")
    cg_emit("    syscall")
    cg_emit("    add  rsp, 16")
    cg_emit("    pop  rbx")
    cg_emit("    mov  rax, 1        ; tagged 0 (nilai kembali tak terpakai)")
    cg_emit("    ret")
    cg_emit("")

    cg_emit("masukan:")
    cg_emit("    push rbx")
    cg_emit("    push r12")
    cg_emit("    push r13")
    cg_emit("    mov  rdi, 256")
    cg_emit("    call __alloc       ; buffer baris, maks 255 karakter")
    cg_emit("    mov  r12, rax")
    cg_emit("    xor  r13, r13      ; panjang terisi")
    cg_emit("    sub  rsp, 8        ; slot 1 byte buat baca per-karakter")
    cg_emit(".ms_lp:")
    cg_emit("    cmp  r13, 255")
    cg_emit("    jge  .ms_done")
    cg_emit("    xor  rax, rax      ; sys_read")
    cg_emit("    xor  rdi, rdi      ; fd 0 (stdin)")
    cg_emit("    mov  rsi, rsp")
    cg_emit("    mov  rdx, 1")
    cg_emit("    syscall")
    cg_emit("    cmp  rax, 1")
    cg_emit("    jne  .ms_done      ; EOF atau error")
    cg_emit("    mov  bl, byte [rsp]")
    cg_emit("    cmp  bl, 10        ; '\\n'")
    cg_emit("    je   .ms_done")
    cg_emit("    mov  byte [r12 + r13], bl")
    cg_emit("    inc  r13")
    cg_emit("    jmp  .ms_lp")
    cg_emit(".ms_done:")
    cg_emit("    add  rsp, 8")
    cg_emit("    mov  byte [r12 + r13], 0")
    cg_emit("    mov  rax, r12")
    cg_emit("    pop  r13")
    cg_emit("    pop  r12")
    cg_emit("    pop  rbx")
    cg_emit("    ret")
    cg_emit("")

    cg_emit("tipe:")
    cg_emit("    test rdi, 1        ; ganjil = angka tertag")
    cg_emit("    jnz  .tp_angka")
    cg_emit("    mov  rax, qword [rdi]")
    cg_emit("    cmp  rax, 0x4C4F4F50")
    cg_emit("    je   .tp_larik")
    cg_emit("    lea  rax, [rel __tp_teks]")
    cg_emit("    ret")
    cg_emit(".tp_angka:")
    cg_emit("    lea  rax, [rel __tp_angka]")
    cg_emit("    ret")
    cg_emit(".tp_larik:")
    cg_emit("    lea  rax, [rel __tp_larik]")
    cg_emit("    ret")
    cg_emit("")

    cg_emit("acakAngka:")
    cg_emit("    push rbx")
    cg_emit("    sar  rdi, 1        ; lepas tag: batas atas (eksklusif)")
    cg_emit("    mov  rbx, rdi")
    cg_emit("    sub  rsp, 8")
    cg_emit("    mov  rdi, rsp")
    cg_emit("    mov  rsi, 8")
    cg_emit("    xor  rdx, rdx")
    cg_emit("    mov  rax, 318      ; sys_getrandom")
    cg_emit("    syscall")
    cg_emit("    mov  rax, qword [rsp]")
    cg_emit("    add  rsp, 8")
    cg_emit("    and  rax, 0x7FFFFFFF   ; buang bit tanda & batasi ke 31-bit, cukup buat modulo")
    cg_emit("    test rbx, rbx")
    cg_emit("    jz   .ak_nol")
    cg_emit("    xor  rdx, rdx")
    cg_emit("    div  rbx           ; rdx = sisa bagi = angka acak [0, batas)")
    cg_emit("    mov  rax, rdx")
    cg_emit("    jmp  .ak_done")
    cg_emit(".ak_nol:")
    cg_emit("    xor  rax, rax")
    cg_emit(".ak_done:")
    cg_emit("    lea  rax, [rax*2 + 1]   ; re-tag")
    cg_emit("    pop  rbx")
    cg_emit("    ret")
    cg_emit("")

    cg_emit_data("    align 2")
    cg_emit_data("    __tp_angka: db 'angka', 0")
    cg_emit_data("    align 2")
    cg_emit_data("    __tp_teks:  db 'teks', 0")
    cg_emit_data("    align 2")
    cg_emit_data("    __tp_larik: db 'larik', 0")

    cg_emit("cetakTanpaBaris:")
    cg_emit("    push rbx")
    cg_emit("    call __ke_teks   ; fix: dulu asumsi selalu teks, angka bikin crash")
    cg_emit("    mov  rbx, rax")
    cg_emit("    mov  rdi, rax")
    cg_emit("    call __strlen")
    cg_emit("    mov  rdx, rax")
    cg_emit("    mov  rsi, rbx")
    cg_emit("    mov  rdi, 1")
    cg_emit("    mov  rax, 1")
    cg_emit("    syscall")
    cg_emit("    pop  rbx")
    cg_emit("    mov  rax, 1   ; tagged 0 (nilai kembali tak terpakai)")
    cg_emit("    ret")
    cg_emit("")

    // __eksekusi: rdi=ptr teks perintah shell -> rax=kode keluar
    // Jalanin lewat /bin/sh -c "<perintah>" (fork + execve + wait4),
    // stdin/stdout/stderr ikut proses anak apa adanya (warisan fork).
    cg_emit("__eksekusi:")
    cg_emit("    push rbx")
    cg_emit("    push r12")
    cg_emit("    push r13")
    cg_emit("    mov  rbx, rdi              ; ptr teks perintah")
    cg_emit("    sub  rsp, 64")
    cg_emit("    lea  rax, [rel __sh_path]")
    cg_emit("    mov  qword [rsp], rax")
    cg_emit("    lea  rax, [rel __sh_dashc]")
    cg_emit("    mov  qword [rsp + 8], rax")
    cg_emit("    mov  qword [rsp + 16], rbx")
    cg_emit("    mov  qword [rsp + 24], 0")
    cg_emit("    lea  r13, [rsp]            ; argv utk execve")
    cg_emit("    mov  rax, 57                 ; sys_fork")
    cg_emit("    syscall")
    cg_emit("    cmp  rax, 0")
    cg_emit("    jne  .ek_parent")
    cg_emit("    ; proses anak")
    cg_emit("    lea  rdi, [rel __sh_path]")
    cg_emit("    mov  rsi, r13")
    cg_emit("    mov  rdx, qword [rel __envp]")
    cg_emit("    mov  rax, 59                 ; sys_execve")
    cg_emit("    syscall")
    cg_emit("    mov  rdi, 127                ; execve gagal")
    cg_emit("    mov  rax, 60")
    cg_emit("    syscall")
    cg_emit(".ek_parent:")
    cg_emit("    mov  r12, rax               ; pid anak")
    cg_emit("    lea  rsi, [rsp + 32]          ; slot status")
    cg_emit("    mov  qword [rsi], 0")
    cg_emit("    mov  rdi, r12")
    cg_emit("    xor  rdx, rdx")
    cg_emit("    xor  r10, r10")
    cg_emit("    mov  rax, 61                 ; sys_wait4")
    cg_emit("    syscall")
    cg_emit("    mov  rax, qword [rsp + 32]")
    cg_emit("    add  rsp, 64")
    cg_emit("    and  rax, 0xFF00")
    cg_emit("    shr  rax, 8                  ; WEXITSTATUS")
    cg_emit("    lea  rax, [rax*2 + 1]        ; re-tag kode keluar")
    cg_emit("    pop  r13")
    cg_emit("    pop  r12")
    cg_emit("    pop  rbx")
    cg_emit("    ret")
    cg_emit("")

    cg_emit_bss("    __intbuf: resb 32")
    cg_emit_bss("    __heap_ptr: resq 1")
    cg_emit_bss("    align 8   ; jamin pointer heap selalu genap (skema tag)")
    cg_emit_bss("    __heap: resb 200000000")
    cg_emit_bss("    __envp: resq 1")
    cg_emit_bss("    __handler_top: resq 1   ; puncak tumpukan handler coba/tangkap")
    cg_emit_data("    __nl: db 0xA")
    cg_emit_data("    __sh_path: db '/bin/sh', 0")
    cg_emit_data("    __sh_dashc: db '-c', 0")
}

fungsi generate_nasm(prog) {
    // Reset state
    CG_label        = 0
    CG_data         = []
    CG_bss          = []
    CG_text         = []
    CG_stack        = 0
    CG_vars         = []
    CG_str_cnt      = 0
    CG_func_end     = ""
    CG_break_stk    = []
    CG_continue_stk = []

    larik body         = prog[1]
    larik fungsi_nodes = []
    larik top_stmts    = []
    CG_pakai_loaded    = []

    larik pasangan_awal = cg_proses_body(body, fungsi_nodes, top_stmts, "")
    fungsi_nodes = pasangan_awal[0]
    top_stmts    = pasangan_awal[1]

    larik simpan_tokens_pre = P_tokens
    angka simpan_pos_pre    = P_pos
    teks  simpan_src_pre    = G_src
    teks  kode_bawaan = bacaFileTeks("Library/_bawaan.lp")
    jika kode_bawaan == "" maka
        p_gagal("Library/_bawaan.lp tidak ditemukan - wajib ada di samping compiler.lp", 0)
    akhir
    G_src = kode_bawaan
    larik bawaan_prog = parse_program(tokenisasi(kode_bawaan))
    larik bawaan_body = bawaan_prog[1]
    angka pi = 0
    selama pi < panjang(bawaan_body) {
        jika bawaan_body[pi][0] == "FUNGSI" dan cg_fungsi_dikenal(bawaan_body[pi][1]) == 0 maka
            fungsi_nodes = tambah(fungsi_nodes, bawaan_body[pi])
            CG_known_funcs = tambah(CG_known_funcs, bawaan_body[pi][1])
        akhir
        pi += 1
    }
    G_src    = simpan_src_pre
    P_tokens = simpan_tokens_pre
    P_pos    = simpan_pos_pre

    cg_emit("global _start")
    cg_emit("")
    cg_emit("_start:")
    cg_emit("    mov  r15, qword [rsp]      ; argc mentah dari OS")
    cg_emit("    lea  r14, [rsp + 8]        ; alamat argv[0] mentah dari OS")
    cg_emit("    push rbp")
    cg_emit("    mov  rbp, rsp")
    cg_emit("    sub  rsp, 4096   ; ruang variabel global")
    cg_emit("    lea  rax, [rel __heap]")
    cg_emit("    mov  qword [rel __heap_ptr], rax")

    angka arg_off = cg_alloc("argumen")
    cg_emit("    ; bangun 'argumen' = larik argv, format [MAGIC][PANJANG][KAPASITAS][elemen..]")
    cg_emit("    mov  rdi, r15")
    cg_emit("    imul rdi, 8")
    cg_emit("    add  rdi, 24")
    cg_emit("    call __alloc")
    cg_emit("    mov  qword [rax], 0x4C4F4F50")
    cg_emit("    mov  qword [rax + 8], r15")
    cg_emit("    mov  qword [rax + 16], r15")
    cg_emit("    mov  qword [rbp#{fmt_off(arg_off)}], rax")
    cg_emit("    xor  rcx, rcx")
    cg_emit(".av_lp:")
    cg_emit("    cmp  rcx, r15")
    cg_emit("    jge  .av_done")
    cg_emit("    mov  rdi, qword [r14 + rcx*8]   ; pointer argv OS mentah (alamat tak terjamin genap)")
    cg_emit("    push rcx")
    cg_emit("    call __strlen")
    cg_emit("    mov  r13, rax")
    cg_emit("    mov  rdi, qword [r14 + rcx*8]")
    cg_emit("    push rdi")
    cg_emit("    mov  rdi, r13")
    cg_emit("    inc  rdi")
    cg_emit("    call __alloc   ; salin ke heap - jamin alamat genap (skema tag)")
    cg_emit("    pop  rsi")
    cg_emit("    mov  rdi, rax")
    cg_emit("    push rax")
    cg_emit("    xor  rdx, rdx")
    cg_emit(".av_cpy:")
    cg_emit("    cmp  rdx, r13")
    cg_emit("    jge  .av_cpy_done")
    cg_emit("    mov  al, byte [rsi + rdx]")
    cg_emit("    mov  byte [rdi + rdx], al")
    cg_emit("    inc  rdx")
    cg_emit("    jmp  .av_cpy")
    cg_emit(".av_cpy_done:")
    cg_emit("    mov  byte [rdi + r13], 0")
    cg_emit("    pop  rdx   ; alamat salinan (genap)")
    cg_emit("    pop  rcx")
    cg_emit("    mov  rax, qword [rbp#{fmt_off(arg_off)}]")
    cg_emit("    mov  qword [rax + 24 + rcx*8], rdx")
    cg_emit("    inc  rcx")
    cg_emit("    jmp  .av_lp")
    cg_emit(".av_done:")
    cg_emit("    lea  rax, [r14 + r15*8 + 8]  ; envp: tepat setelah argv[] + NULL")
    cg_emit("    mov  qword [rel __envp], rax")

    angka j = 0
    selama j < panjang(top_stmts) {
        cg_stmt(top_stmts[j])
        j += 1
    }

    cg_emit("")
    cg_emit("    xor  rdi, rdi")
    cg_emit("    mov  rax, 60")
    cg_emit("    syscall")

    angka k = 0
    selama k < panjang(fungsi_nodes) {
        cg_fungsi(fungsi_nodes[k])
        k += 1
    }

    cg_emit_runtime()

    larik out = []
    out = tambah(out, "; Loop Compiler - NASM x86-64 Linux")
    out = tambah(out, "; Dihasilkan oleh compiler.lp")
    out = tambah(out, "")
    out = tambah(out, "section .data")
    angka di = 0
    selama di < panjang(CG_data) {
        out = tambah(out, CG_data[di])
        di += 1
    }
    out = tambah(out, "")
    out = tambah(out, "section .bss")
    angka bi = 0
    selama bi < panjang(CG_bss) {
        out = tambah(out, CG_bss[bi])
        bi += 1
    }
    out = tambah(out, "")
    out = tambah(out, "section .text")
    angka ti = 0
    selama ti < panjang(CG_text) {
        out = tambah(out, CG_text[ti])
        ti += 1
    }

    kembali gabungTeks(out, "\n")
}

larik argv = argumen
jika panjang(argv) < 2 maka
    cetak "Penggunaan: compiler.lp <program.lp> [-o output.nasm]"
selain
    teks target = argv[1]
    teks src = bacaFileTeks(target)
    G_src = src
    jika src == "" maka
        cetak "Error: file kosong atau tidak ditemukan: #{target}"
    selain
        larik tokens = tokenisasi(src)
        larik ast    = parse_program(tokens)
        teks  nasm   = generate_nasm(ast)
        teks outfile = ""
        jika panjang(argv) >= 4 maka
            jika argv[2] == "-o" maka
                outfile = argv[3]
            akhir
        akhir

        jika outfile != "" maka
            tulisFileTeks(outfile, nasm)
            cetak "[compiler.lp] NASM → #{outfile}"
            cetak "  nasm -f elf64 #{outfile} -o out.o && ld out.o -o out && ./out"
        selain
            teks tnasm = "/tmp/.loop_run.nasm"
            teks tobj  = "/tmp/.loop_run.o"
            teks tbin  = "/tmp/.loop_run.bin"
            tulisFileTeks(tnasm, nasm)
            angka rc1 = eksekusi("nasm -f elf64 #{tnasm} -o #{tobj}")
            jika rc1 != 0 maka
                cetak "Error: nasm gagal (kode #{rc1})"
            selain
                angka rc2 = eksekusi("ld #{tobj} -o #{tbin}")
                jika rc2 != 0 maka
                    cetak "Error: ld gagal (kode #{rc2})"
                selain
                    eksekusi(tbin)
                akhir
            akhir
        akhir
    akhir
akhir

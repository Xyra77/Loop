// spesifikasi/level-rendah.lp
// DESAIN — JANGAN DIJALANKAN!
// Tujuan: Loop tetap mudah dibaca kayak Python/Lua secara default,
// tapi punya jalur eksplisit turun ke level C/C++ kalau dibutuhkan.
//
// Prinsip: AMAN itu default. RENDAH itu pilihan sadar, ditandai jelas
// di kode, bukan nyelip diam-diam.

// ─────────────────────────────────────────────
// 1. TIPE BERUKURAN TETAP
// ─────────────────────────────────────────────
// "angka" biasa tetap fleksibel (kayak sekarang) buat kode harian.
// Untuk kerja low-level, tipe eksak dibutuhkan supaya codegen tau
// persis berapa byte yang dipesan/register mana yang dipakai.

angka8  kecil   = 250      // 1 byte,  0..255
angka16 sedang  = 60000    // 2 byte
angka32 normal  = 4000000  // 4 byte
angka64 besar   = 9000000000  // 8 byte
mengambang32 f32 = 3.14
mengambang64 f64 = 3.14159265358979

// ─────────────────────────────────────────────
// 2. PENUNJUK (POINTER)
// ─────────────────────────────────────────────
// Tipe "penunjuk<T>" cuma boleh dideklarasikan di dalam blok
// `mentah { }` atau fungsi yang ditandai `tak_aman`.
// Di luar situ, compiler nolak — jadi programmer nggak bisa
// "kebablasan" pegang alamat memori tanpa sadar.

mentah {
    penunjuk<angka64> buf = alokasi(64)   // minta 64 byte, dapet alamatnya
    tulisMemori(buf, 0, 72)               // tulis byte manual di offset 0
    angka64 nilai = bacaMemori(buf, 0)    // baca balik
    bebas(buf)                            // WAJIB — nggak ada GC di sini
}

// ─────────────────────────────────────────────
// 3. FUNGSI TAK_AMAN
// ─────────────────────────────────────────────
// Alternatif blok mentah: seluruh fungsi ditandai sekali,
// isinya boleh pakai penunjuk/memori manual di mana aja.

tak_aman fungsi salinCepat(penunjuk<angka8> tujuan, penunjuk<angka8> asal, angka64 n) {
    angka64 i = 0
    selama i < n {
        tulisMemori(tujuan, i, bacaMemori(asal, i))
        i += 1
    }
}

// ─────────────────────────────────────────────
// 4. SYSCALL LANGSUNG
// ─────────────────────────────────────────────
// syscall(nomor, arg1, arg2, arg3, arg4, arg5) → emit langsung
// mov rax,nomor / mov rdi,arg1 / ... / syscall di NASM.
// Cuma valid di dalam mentah{} / fungsi tak_aman.

mentah {
    // write(1, buf, 64) — syscall write x86-64 Linux nomor 1
    syscall(1, 1, buf, 64, 0, 0)
    // exit(0) — syscall exit nomor 60
    syscall(60, 0, 0, 0, 0, 0)
}

// ─────────────────────────────────────────────
// 5. ASSEMBLY INLINE
// ─────────────────────────────────────────────
// Buat kasus ekstrem yang butuh instruksi CPU spesifik
// (SIMD, atomic, dll) yang belum ada builtin-nya.

mentah {
    asm {
        "mov rax, 42"
        "inc rax"
    }
}

// ─────────────────────────────────────────────
// 6. EKSTERNAL — SAMBUNG KE loop_runtime (C)
// ─────────────────────────────────────────────
// Daripada nulis ulang crypto/networking di NASM mentah,
// deklarasikan fungsi C yang udah ada di loop_runtime,
// lalu linker gabungin .o hasil compile Loop dengan loop_runtime.a

eksternal fungsi loop_tcp_connect(host: teks, port: angka32) -> angka32 dari "loop_runtime"
eksternal fungsi loop_sha256(data: penunjuk<angka8>, panjang: angka64, keluar: penunjuk<angka8>) dari "loop_runtime"

// ─────────────────────────────────────────────
// 7. BUNGKUS BALIK KE DUNIA AMAN
// ─────────────────────────────────────────────
// Aturan wajib: data hasil olahan mentah{} yang mau dipakai
// balik sebagai objek Loop biasa (teks/larik yang dikelola GC)
// HARUS lewat fungsi bungkus resmi — nggak boleh assign langsung.
// Ini yang jaga batas antara memori manual dan memori ber-GC.

fungsi bacaBerkasCepat(path) {
    mentah {
        penunjuk<angka8> buf = alokasi(4096)
        angka64 n = syscall(0, 3, buf, 4096, 0, 0)   // read()
        teks hasil = bungkusTeks(buf, n)             // resmi: keluar dari mentah
        bebas(buf)
        kembali hasil
    }
}

// ─────────────────────────────────────────────
// CATATAN IMPLEMENTASI
// ─────────────────────────────────────────────
// - Semua fitur di file ini CUMA valid saat compile --nasm.
//   Interpreter (mode default `loop file.lp`) wajib TOLAK node
//   mentah/penunjuk/asm dengan pesan jelas: "fitur ini butuh --nasm,
//   nggak bisa diinterpret".
// - Type-checker: di luar mentah{} tetap longgar (kayak sekarang).
//   Di dalam mentah{}: wajib tipe eksak, nggak ada auto-convert.
// - CATATAN PENTING (per Agustus 2026): self-hosting compiler.lp
//   udah tercapai duluan sebelum fitur di file ini diimplementasi.
//   Jadi begitu tier low-level ini beneran dibangun, compiler.lp
//   bisa langsung PAKAI fitur ini buat ngatur memorinya sendiri —
//   makin lepas dari ketergantungan ke desain runtime interpreter C.

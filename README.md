<div align="center">
  <img src="https://github.com/Xyra77/Loop/blob/main/src/Loop.png" alt="Loop Language" width="180"/>

# Loop Language
Modern programming language with Indonesian-based syntax.
Designed for scripting and automation, with an interpreter written in C.

[![Version](https://img.shields.io/badge/version-0.6-blueviolet)](#)
[![VS Code](https://img.shields.io/badge/VS%20Code-Extension-blue?logo=visualstudiocode)](#)
[![License](https://img.shields.io/badge/license-MIT-green)](LICENSE.txt)

</div>

---

# About Loop

Loop is a modern programming language with Indonesian-based syntax. It provides:

- High performance
- Developer productivity
- Natural and readable code
- Lightweight runtime
- Complete standard library

Designed for:
- System automation
- Artificial Intelligence
- Machine Learning
- Networking & cryptography
- Backend services
- CLI tooling

Source files use the `.lp` extension.

---

# Recent Updates (unreleased)

Error model, binary data and cryptography were redesigned. Details, breaking
changes, and verification notes are in [CATATAN-PERUBAHAN.md](CATATAN-PERUBAHAN.md).

- Typed errors that can always be caught: `lempar galat("GalatNilai", "pesan")`,
  `coba { } tangkap { } akhirnya { }`, `jenisGalat(e)`, `pesanGalat(e)`.
  Uncaught errors print a call trace and exit with code 1.
- New `bita` type for binary data; strings no longer truncate at 4096 characters.
- Native cryptography via OpenSSL (AES-256-GCM, PBKDF2-HMAC-SHA256, HMAC-SHA256,
  secure RNG); `Library/crypto.lp` is now a thin wrapper.
- File builtins that report failure (`bacaBita`, `tulisAtomik`, `adaBerkas`, ...).
- Test suite with standard vectors, run in CI under ASan/UBSan.

# Earlier Updates (v0.6)

## Bug Fixes

### 1. jalankan keyword
Fixed to properly execute shell commands instead of just printing.

Before: `jalankan("ls -la")` would print the string (was aliased to cetak)
After: `jalankan("ls -la")` executes the command and returns exit code

Implementation: Added N_JALANKAN node type to properly handle shell execution.

### 2. bacaFile() documentation
Clarified that bacaFile() executes files as code (import-like behavior), not read as text.

Use bacaFileTeks() to read file contents as plain text string.
Added explicit comments in source code for clarity.

### 3. telusuri() depth parameter
Fixed to accept optional depth parameter as documented.

Before: telusuri(url) — only accepted 1 argument
After: telusuri(url, depth) — accepts 1 or 2 arguments, depth limited to 0-5

For detailed technical breakdown, see CATATAN-PROGRESS.md

---

# Philosophy

Loop is built to be:

- Small: a single-file C interpreter
- Clean: Readable syntax
- Expressive: Powerful language features
- Easy to learn: Natural Indonesian-based keywords

with failures that are visible: an operation that fails raises a typed error
instead of returning a placeholder value.

---

# Example Program

```loop
angka umur = 17
teks nama = "Budi"

jika umur >= 17 maka
    cetak "Halo, #{nama}!"
selain
    cetak "Belum cukup umur"
akhir

ulang 3 {
    cetak "Loop!"
}

fungsi sapa(nama) {
    kembali "Halo, #{nama}!"
}
```

---

# Language Features

- Modern data types (angka, teks, larik, boolean)
- Modular functions
- Pipe operator (|>)
- Array utilities (urut, balik, saring, peta, kurangi)
- String manipulation (potong, ganti, pisah, gabung)
- Error handling (coba, tangkap, akhirnya, lempar) with typed errors
- Binary data (`bita`), UTF-8 text without silent length limits
- Module system (`pakai`, `bacaFile`; `bacaFileTeks` reads text)
- Mathematical operations (trig, log, sqrt, bitwise)
- Networking (HTTP, TCP/TLS sockets, timeouts, certificate verification)
- Cryptography (AES-256-GCM, PBKDF2, HMAC) through OpenSSL
- System integration (jalankan shell commands, env vars)

---

# Building from Source

Requirements:
- GCC or compatible C compiler
- OpenSSL libraries (libssl-dev on Linux)
- POSIX-compliant system

Build:
```bash
cd Loop/seed
gcc -O2 -o loop loop.c -lm -lssl -lcrypto -lpthread
```

`Library/` is found next to the binary (`<dir>/Library`) or one level up
(`<dir>/../Library`, the repository layout). Set `LOOP_STDLIB=/path/to/Library`
to point elsewhere, which is what you want after copying the binary to
`/usr/local/bin`.

Environment variables:

| Variable | Meaning |
|---|---|
| `LOOP_STDLIB` | directory of the standard library |
| `LOOP_MAKS_UKURAN` | max size in bytes of a text/bita/file (default 64 MiB); exceeding it raises `GalatBatas` |
| `LOOP_TIMEOUT_JARINGAN` | default network timeout in seconds (default 10) |
| `LOOP_TLS_TIDAK_AMAN=1` | disable TLS certificate verification (testing only) |

---

# Self-hosting

`seed/loop.c` is the parent. It runs `seed/compiler.lp`, which compiles Loop to
NASM; the compiler can compile itself:

```bash
seed/bootstrap.sh   # needs gcc, nasm, ld
```

The script builds the generation-1 native compiler with the interpreter, uses it to
compile `compiler.lp` again, and checks that both NASM outputs are byte-identical
(fixpoint). It also runs `tes/selfhost/*.lp` natively and compares with the
interpreter. The native backend supports a subset of the language (see
CATATAN-PERUBAHAN.md), including typed errors: `coba { } tangkap { } akhirnya { }`,
`lempar`, `galat()`, `jenisGalat`/`pesanGalat`/`sebabGalat`, division by zero and
out-of-range indexing raise catchable errors, and an uncaught error prints
`[GALAT] Jenis: pesan` to stderr and exits with code 1 (no call trace yet).

---

# Testing

From the repository root:
```bash
gcc -O2 -o seed/loop seed/loop.c -lm -lssl -lcrypto -lpthread
tes/jalankan_semua.sh
```

See [tes/README.md](tes/README.md). CI (`.github/workflows/ci.yml`) runs the same
suite on a release build and on an ASan/UBSan build.

---

# VS Code Extension

## Features

- Syntax highlighting
- Snippets
- Auto indent
- Bracket pairing
- Run files from editor
- Syntax validation

## Installation

Via Marketplace:
1. Open VS Code
2. Press Ctrl+Shift+X
3. Search "Loop Language"
4. Click Install

Via manual installation:
```bash
code --install-extension loop-lang-1_4_0.vsix
```

---

# Development Structure

```
Loop/
├── Library/            # Thin standard library written in Loop
│   ├── dasar.lp        # helpers that are not builtins
│   ├── bridge.lp       # wrappers over internal builtins
│   ├── io.lp
│   ├── net.lp
│   ├── _bawaan.lp      # builtin runtime read by seed/compiler.lp (not loaded via pakai)
│   ├── crypto.lp       # wrapper over native AES-GCM/PBKDF2/HMAC
│   └── pastikan.lp     # tiny test helper
├── seed/
│   ├── loop.c          # Interpreter & runtime
│   ├── compiler.lp     # Self-hosted compiler (Loop -> NASM x86-64)
│   └── bootstrap.sh    # loop.c -> compiler.lp -> native compiler, fixpoint check
├── contoh/             # Learning material (pure-Loop AES), not a library
├── tes/                # Test suite (see tes/README.md)
├── spesifikasi/        # Language specification
├── .github/workflows/  # CI
├── README.md
├── CATATAN-PERUBAHAN.md
└── LICENSE.txt
```

---

# Roadmap

Current development:
- Port of the interpreter to C++ (exceptions + RAII, no leaks on the error path)
- Decide how the native compiler reports errors (`seed/compiler.lp`)
- Property access for errors (`e.jenis`) next to `jenisGalat(e)`

Known limitations are listed in CATATAN-PERUBAHAN.md.

Future:
- Concurrency system
- AI/ML utilities
- Enhanced networking
- Package manager
- Native compilation

---

# Status

Loop is under active development (v0.6).

Current focus: Language stability and correctness.
All core features tested and working.

---

# License

Apache-2.0 License — Copyright (c) 2026 Xyra77. See [LICENSE.txt](LICENSE.txt).

---

# Support

Issues and contributions: https://github.com/Xyra77/Loop

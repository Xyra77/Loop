<div align="center">
  <img src="https://b.top4top.io/p_3799m5jcu1.png" alt="Loop Language" width="180"/>

# Loop Language
Modern programming language with Indonesian-based syntax.
Designed for automation, artificial intelligence, machine learning, modern systems, and high-performance computing.

[![Version](https://img.shields.io/badge/version-0.6-blueviolet)](#)
[![VS Code](https://img.shields.io/badge/VS%20Code-Extension-blue?logo=visualstudiocode)](#)
[![License](https://img.shields.io/badge/license-Proprietary-red)](LICENSE)

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

# Recent Updates (v0.6)

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

For detailed technical breakdown, see FIXES_SUMMARY.md

---

# Philosophy

Loop is built to be:

- Fast: Optimized C runtime
- Clean: Readable syntax
- Expressive: Powerful language features
- Efficient: Minimal overhead
- Easy to learn: Natural Indonesian-based keywords

without sacrificing low-level capabilities.

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
- Error handling (coba, tangkap)
- Module system (bacaFile, bacaFileTeks)
- Mathematical operations (trig, log, sqrt, bitwise)
- Networking (HTTP, socket operations)
- System integration (jalankan shell commands, env vars)
- Built-in standard library
- Lightweight runtime

---

# Building from Source

Requirements:
- GCC or compatible C compiler
- OpenSSL libraries (libssl-dev on Linux)
- POSIX-compliant system

Build:
```bash
cd Loop/seed
gcc -O2 -o loop loop.c -lm -lssl -lcrypto
sudo cp loop /usr/local/bin/
```

---

# Testing

Run test suite:
```bash
cd tes/
loop io.lp           # I/O operations
loop string.lp       # String functions
loop larik.lp        # Array operations
loop matematika.lp   # Math & bitwise
loop sistem.lp       # System functions
loop jaringan.lp     # Network functions
```

All tests pass on v0.6.

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
├── seed/
│   ├── loop.c          # Interpreter & runtime
│   ├── compiler.lp     # Self-hosted compiler
├── tes/                # Test suite
│   ├── io.lp
│   ├── string.lp
│   ├── larik.lp
│   ├── matematika.lp
│   ├── sistem.lp
│   ├── jaringan.lp
├── spesifikasi/        # Language specification
├── README.md
├── FIXES_SUMMARY.md
└── LICENSE.txt
```

---

# Roadmap

Current development:
- Core language stability (v0.6)
- Keyword behavior correctness
- Performance optimization
- Extended standard library
- Production tooling

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

Copyright 2026 Xyra77 — All Rights Reserved.

---

# Support

Issues and contributions: https://github.com/Xyra77/Loop

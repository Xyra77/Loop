#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <openssl/x509_vfy.h>
#include <openssl/crypto.h>
#include <pthread.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <regex.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

extern char **environ;

#define LOOP_VERSI "0.6"
#define MAX_TOKEN 65536
#define MAX_IDENT 256
#define MAX_STRING 4096
#define MAX_PARAMS 32
#define MAX_ARGS 32
#define MAX_ARRAY 4096
#define MAX_ENV_VARS 1024
#define MAX_CALL_STACK 4000
#define MAX_GABUNG_ELEMEN 64
#define MAX_INTERP_PARTS 128

static jmp_buf g_repl_jmp;
static int g_in_repl = 0;

typedef struct Val Val;
typedef struct Node Node;

/* ===== Galat berjenis =====
   Satu jalur untuk semua kegagalan: objek galat (jenis, pesan, lokasi, sebab)
   dilempar lewat loop_lempar(). Bila ada blok `coba` yang menunggu, kendali
   melompat ke sana (setjmp/longjmp); bila tidak, jejak dicetak ke stderr dan
   proses keluar dengan kode 1 (atau kembali ke prompt bila di REPL).
   Catatan: longjmp tidak membebaskan Env/Val milik frame yang dilewati, jadi
   jalur galat bisa membocorkan memori. Dapat diterima untuk skrip pendek;
   untuk proses panjang tunggu port C++ (RAII). */
typedef struct Handler {
  jmp_buf jb;
  struct Handler *prev;
} Handler;
static Handler *g_handler = NULL;
static Val *g_galat_aktif = NULL;

static void loop_lempar_jenis(const char *jenis, const char *fmt, ...)
    __attribute__((noreturn, format(printf, 2, 3)));
static void loop_galat(const char *fmt, ...)
    __attribute__((noreturn, format(printf, 1, 2)));

/* Titik lama (LOOP_ASSERT, loop_galat) menjadi GalatInternal. */
static void loop_galat(const char *fmt, ...) {
  char buf[2048];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  loop_lempar_jenis("GalatInternal", "%s", buf);
}

#define LOOP_ASSERT(cond, ...)                                                 \
  do {                                                                         \
    if (!(cond))                                                               \
      loop_galat(__VA_ARGS__);                                                 \
  } while (0)

/* Validasi argumen builtin: jumlah/tipe salah -> GalatTipe (TypeError). */
#define ARG_ASSERT(cond, ...)                                                  \
  do {                                                                         \
    if (!(cond))                                                               \
      loop_lempar_jenis("GalatTipe", __VA_ARGS__);                             \
  } while (0)

/* Nama berkas sumber yang sedang di-parse/dijalankan (untuk lokasi galat). */
static const char *g_berkas_parse = "<stdin>";
static const char *g_berkas_aktif = "<stdin>";
static int g_baris_kini = 0;
typedef enum {
  TK_EOF = 0,
  TK_NUMBER,
  TK_STRING,
  TK_BOOL_BENAR,
  TK_BOOL_SALAH,
  TK_NULL,
  TK_IDENT,
  TK_FUNGSI,
  TK_KEMBALI,
  TK_JIKA,
  TK_MAKA,
  TK_SELAIN,
  TK_AKHIR,
  TK_SELAMA,
  TK_ULANG,
  TK_HENTIKAN,
  TK_LEWATI,
  TK_PILIH,
  TK_KASUS,
  TK_COBA,
  TK_TANGKAP,
  TK_ANGKA,
  TK_TEKS,
  TK_BOOL_T,
  TK_LARIK,
  TK_MILIK,
  TK_CETAK,
  TK_JALANKAN,
  TK_BACAFILE,
  TK_TULISFILE,
  TK_PAKAI,
  TK_DAN,
  TK_ATAU,
  TK_TIDAK,
  TK_PLUS,
  TK_MINUS,
  TK_BINTANG,
  TK_SLASH,
  TK_PERSEN,
  TK_PLUSEQ,
  TK_MINUSEQ,
  TK_STAREQ,
  TK_SLASHEQ,
  TK_PRSNEQ,
  TK_EQ,
  TK_NEQ,
  TK_LT,
  TK_GT,
  TK_LEQ,
  TK_GEQ,
  TK_ASSIGN,
  TK_PIPA,
  TK_PANAH,
  TK_LPAREN,
  TK_RPAREN,
  TK_LBRACE,
  TK_RBRACE,
  TK_LBRACKET,
  TK_RBRACKET,
  TK_COMMA,
  TK_LEMPAR,
  TK_AKHIRNYA,
  TK_BITA_T,
} TokenType;

typedef struct {
  TokenType type;
  char val[MAX_STRING];
  int line;
  double num;
} Token;

typedef struct {
  const char *src;
  int pos;
  int line;
  Token tokens[MAX_TOKEN];
  int n_tokens;
} Lexer;

static int adalah_alfa(char c) { return isalpha((unsigned char)c) || c == '_'; }
static int adalah_alfanumerik(char c) {
  return isalnum((unsigned char)c) || c == '_';
}

static void lex_maju(Lexer *L) {
  if (L->src[L->pos])
    L->pos++;
}

static void lewati_spasi_komentar(Lexer *L) {
  for (;;) {
    while (L->src[L->pos] && strchr(" \t\r", L->src[L->pos])) {
      lex_maju(L);
    }
    if (L->src[L->pos] == '\n') {
      L->line++;
      lex_maju(L);
      continue;
    }
    if (L->src[L->pos] == '/' && L->src[L->pos + 1] == '/') {
      while (L->src[L->pos] && L->src[L->pos] != '\n')
        lex_maju(L);
      continue;
    }
    if (L->src[L->pos] == '/' && L->src[L->pos + 1] == '*') {
      lex_maju(L);
      lex_maju(L);
      while (L->src[L->pos] &&
             !(L->src[L->pos] == '*' && L->src[L->pos + 1] == '/')) {
        if (L->src[L->pos] == '\n')
          L->line++;
        lex_maju(L);
      }
      if (L->src[L->pos]) {
        lex_maju(L);
        lex_maju(L);
      }
      continue;
    }
    break;
  }
}

typedef struct {
  const char *kw;
  TokenType type;
} KwMap;
static const KwMap KEYWORDS[] = {{"fungsi", TK_FUNGSI},
                                 {"kembali", TK_KEMBALI},
                                 {"jika", TK_JIKA},
                                 {"maka", TK_MAKA},
                                 {"selain", TK_SELAIN},
                                 {"akhir", TK_AKHIR},
                                 {"selama", TK_SELAMA},
                                 {"ulang", TK_ULANG},
                                 {"hentikan", TK_HENTIKAN},
                                 {"lewati", TK_LEWATI},
                                 {"pilih", TK_PILIH},
                                 {"kasus", TK_KASUS},
                                 {"coba", TK_COBA},
                                 {"tangkap", TK_TANGKAP},
                                 {"akhirnya", TK_AKHIRNYA},
                                 {"lempar", TK_LEMPAR},
                                 {"angka", TK_ANGKA},
                                 {"teks", TK_TEKS},
                                 {"bool", TK_BOOL_T},
                                 {"larik", TK_LARIK},
                                 {"bita", TK_BITA_T},
                                 {"milik", TK_MILIK},
                                 {"cetak", TK_CETAK},
                                 {"jalankan", TK_JALANKAN},
                                 {"bacaFile", TK_BACAFILE},
                                 {"tulisFile", TK_TULISFILE},
                                 {"pakai", TK_PAKAI},
                                 {"dan", TK_DAN},
                                 {"atau", TK_ATAU},
                                 {"tidak", TK_TIDAK},
                                 {"benar", TK_BOOL_BENAR},
                                 {"salah", TK_BOOL_SALAH},
                                 {"null", TK_NULL},
                                 {NULL, 0}};

static TokenType cari_kataKunci(const char *s) {
  for (int i = 0; KEYWORDS[i].kw; i++)
    if (strcmp(s, KEYWORDS[i].kw) == 0)
      return KEYWORDS[i].type;
  return TK_IDENT;
}

static void tokenisasi(Lexer *L) {
  L->pos = 0;
  L->line = 1;
  L->n_tokens = 0;
  for (;;) {
    lewati_spasi_komentar(L);
    char c = L->src[L->pos];
    if (!c) {
      L->tokens[L->n_tokens++] = (Token){TK_EOF, "", L->line, 0};
      break;
    }

    Token t;
    t.line = L->line;

    if (isdigit((unsigned char)c) ||
        (c == '0' &&
         (L->src[L->pos + 1] == 'x' || L->src[L->pos + 1] == 'X'))) {
#define NUM_PUT(ch)                                                            \
  do {                                                                         \
    if (bi >= 62)                                                              \
      loop_lempar_jenis("GalatBatas", "Baris %d: literal angka terlalu panjang",\
                        L->line);                                              \
    buf[bi++] = (ch);                                                          \
  } while (0)
      char buf[64];
      int bi = 0;
      if (c == '0' &&
          (L->src[L->pos + 1] == 'x' || L->src[L->pos + 1] == 'X')) {
        NUM_PUT(L->src[L->pos++]);
        NUM_PUT(L->src[L->pos++]);
        while (isxdigit((unsigned char)L->src[L->pos]))
          NUM_PUT(L->src[L->pos++]);
      } else {
        while (isdigit((unsigned char)L->src[L->pos]))
          NUM_PUT(L->src[L->pos++]);
        if (L->src[L->pos] == '.' &&
            isdigit((unsigned char)L->src[L->pos + 1])) {
          NUM_PUT(L->src[L->pos++]);
          while (isdigit((unsigned char)L->src[L->pos]))
            NUM_PUT(L->src[L->pos++]);
        }
        if (L->src[L->pos] == 'e' || L->src[L->pos] == 'E') {
          NUM_PUT(L->src[L->pos++]);
          if (L->src[L->pos] == '+' || L->src[L->pos] == '-')
            NUM_PUT(L->src[L->pos++]);
          while (isdigit((unsigned char)L->src[L->pos]))
            NUM_PUT(L->src[L->pos++]);
        }
      }
      buf[bi] = 0;
#undef NUM_PUT
      t.type = TK_NUMBER;
      t.num = strtod(buf, NULL);
      snprintf(t.val, MAX_STRING, "%s", buf);
      L->tokens[L->n_tokens++] = t;
      continue;
    }

    if (c == '"') {
      lex_maju(L);
      int bi = 0;
      t.type = TK_STRING;
      int baris_mulai = L->line;
      while (L->src[L->pos] && L->src[L->pos] != '"') {
        if (bi >= MAX_STRING - 3)
          loop_lempar_jenis("GalatBatas",
                            "Baris %d: literal teks melebihi %d karakter; "
                            "sambung beberapa literal dengan +",
                            baris_mulai, MAX_STRING - 3);
        if (L->src[L->pos] == '\\') {
          lex_maju(L);
          char esc = L->src[L->pos];
          switch (esc) {
          case 'n':
            t.val[bi++] = '\n';
            break;
          case 't':
            t.val[bi++] = '\t';
            break;
          case 'r':
            t.val[bi++] = '\r';
            break;
          case '"':
            t.val[bi++] = '"';
            break;
          case '\\':
            t.val[bi++] = '\\';
            break;
          case '0':
            loop_lempar_jenis("GalatNilai",
                              "Baris %d: teks tidak boleh berisi byte 0x00 "
                              "(gunakan bita)",
                              L->line);
          default:
            t.val[bi++] = '\\';
            t.val[bi++] = esc;
            break;
          }
        } else {
          t.val[bi++] = L->src[L->pos];
        }
        lex_maju(L);
      }
      if (L->src[L->pos] != '"')
        loop_galat("Baris %d: literal teks tidak ditutup", baris_mulai);
      lex_maju(L);
      t.val[bi] = 0;
      t.num = 0;
      L->tokens[L->n_tokens++] = t;
      continue;
    }

    if (adalah_alfa(c)) {
      int bi = 0;
      char buf[MAX_IDENT];
      while (adalah_alfanumerik(L->src[L->pos])) {
        if (bi >= MAX_IDENT - 1)
          loop_lempar_jenis("GalatBatas", "Baris %d: nama terlalu panjang (maks %d)",
                            L->line, MAX_IDENT - 1);
        buf[bi++] = L->src[L->pos++];
      }
      buf[bi] = 0;
      t.type = cari_kataKunci(buf);
      snprintf(t.val, MAX_STRING, "%s", buf);
      t.num = 0;
      L->tokens[L->n_tokens++] = t;
      continue;
    }

    lex_maju(L);
    char c2 = L->src[L->pos];
    t.val[0] = c;
    t.val[1] = 0;
    t.num = 0;
#define TWOC(a, b, tp)                                                         \
  if (c == (a) && c2 == (b)) {                                                 \
    t.type = (tp);                                                             \
    snprintf(t.val, 4, "%c%c", (a), (b));                                      \
    lex_maju(L);                                                               \
    L->tokens[L->n_tokens++] = t;                                              \
    continue;                                                                  \
  }
    TWOC('|', '>', TK_PIPA)
    TWOC('-', '>', TK_PANAH)
    TWOC('=', '=', TK_EQ)
    TWOC('!', '=', TK_NEQ)
    TWOC('<', '=', TK_LEQ)
    TWOC('>', '=', TK_GEQ)
    TWOC('+', '=', TK_PLUSEQ)
    TWOC('-', '=', TK_MINUSEQ)
    TWOC('*', '=', TK_STAREQ)
    TWOC('/', '=', TK_SLASHEQ)
    TWOC('%', '=', TK_PRSNEQ)
#undef TWOC
    switch (c) {
    case '+':
      t.type = TK_PLUS;
      break;
    case '-':
      t.type = TK_MINUS;
      break;
    case '*':
      t.type = TK_BINTANG;
      break;
    case '/':
      t.type = TK_SLASH;
      break;
    case '%':
      t.type = TK_PERSEN;
      break;
    case '<':
      t.type = TK_LT;
      break;
    case '>':
      t.type = TK_GT;
      break;
    case '=':
      t.type = TK_ASSIGN;
      break;
    case '(':
      t.type = TK_LPAREN;
      break;
    case ')':
      t.type = TK_RPAREN;
      break;
    case '{':
      t.type = TK_LBRACE;
      break;
    case '}':
      t.type = TK_RBRACE;
      break;
    case '[':
      t.type = TK_LBRACKET;
      break;
    case ']':
      t.type = TK_RBRACKET;
      break;
    case ',':
      t.type = TK_COMMA;
      break;
    default:
      loop_galat("Baris %d: Karakter tak dikenal: '%c'", L->line, c);
    }
    L->tokens[L->n_tokens++] = t;
  }
}

typedef enum {
  N_LIT_NUM,
  N_LIT_STR,
  N_LIT_BOOL,
  N_LIT_NULL,
  N_VAR,
  N_ARRAY,
  N_STRING_INTERP,
  N_BINOP,
  N_UNIOP,
  N_INDEX,
  N_ASSIGN,
  N_ASSIGN_COMPOUND,
  N_CETAK,
  N_JALANKAN,
  N_BACA_FILE,
  N_TULIS_FILE,
  N_PAKAI,
  N_JIKA,
  N_SELAMA,
  N_ULANG,
  N_PILIH,
  N_COBA,
  N_LEMPAR,
  N_FUNGSI,
  N_KEMBALI,
  N_HENTIKAN,
  N_LEWATI,
  N_PANGGIL,
  N_PIPA,
  N_PROGRAM,
} NodeType;

struct Node;
typedef struct Node Node;

typedef struct {
  int is_expr;
  char *text;
  Node *expr;
} InterpPart;

struct Node {
  NodeType type;
  int line;
  double num;
  char *str;
  int bval;
  char op[8];
  Node *left, *right, *cond;
  Node **body;
  int n_body;
  Node **selain;
  int n_selain;
  char *fname;
  char **params;
  int n_params;
  Node **args;
  int n_args;
  int is_async;
  int is_variadic;
  char *ret_type;
  char *varname;
  char *var_tipe;
  int is_const;
  Node **elems;
  int n_elems;
  Node *obj;
  Node *idx;
  Node **kasus_val;
  Node **kasus_body;
  int n_kasus;
  Node *lainnya;
  Node **tangkap;
  int n_tangkap;
  char *tangkap_var;
  Node **akhirnya;
  int n_akhirnya;
  int has_tangkap;
  int has_akhirnya;
  const char *berkas; /* berkas sumber node ini (untuk jejak galat) */
  InterpPart *interp;
  int n_interp;
  Node *pipa_kiri;
  char *pipa_fn;
  Node **pipa_extra;
  int n_pipa_extra;
  char *untuk_var;
  Node *untuk_iter;
  char *impor_path;
  char *impor_alias;
};

static Node *node_baru(NodeType t, int line) {
  Node *n = calloc(1, sizeof(Node));
  if (!n)
    loop_galat("Out of memory");
  n->type = t;
  n->line = line;
  n->berkas = g_berkas_parse;
  return n;
}

typedef struct {
  Token *tokens;
  int pos;
  int n;
} Parser;

static Token *pcur(Parser *P) { return &P->tokens[P->pos]; }
static Token *intip_token(Parser *P) {
  int nx = P->pos + 1;
  return nx < P->n ? &P->tokens[nx] : &P->tokens[P->n - 1];
}

static Token *makan_token(Parser *P, TokenType t) {
  Token *cur = pcur(P);
  if (t && cur->type != t)
    loop_galat("Baris %d: Diharapkan token %d, dapat %d ('%s')", cur->line, t,
               cur->type, cur->val);
  P->pos++;
  return cur;
}

static Node **urai_blokKurung(Parser *P, int *out_n);
static Node *urai_pernyataan(Parser *P);
static Node *parse_expr(Parser *P);
static Node *urai_logika(Parser *P);
static Node *urai_pipa(Parser *P);
static Node *urai_perbandingan(Parser *P);
static Node *urai_tambahKurang(Parser *P);
static Node *urai_kaliBagi(Parser *P);
static Node *urai_uner(Parser *P);
static Node *urai_primer(Parser *P);

static void urai_interpolasiString(Node *n, const char *s) {
  int cap = 8;
  n->interp = malloc(sizeof(InterpPart) * cap);
  n->n_interp = 0;
  if (!n->interp)
    loop_galat("Out of memory (interp)");
  int i = 0, len = (int)strlen(s);
  char buf[MAX_STRING];
  int bi = 0;
  while (i < len) {
    if (s[i] == '#' && i + 1 < len && s[i + 1] == '{') {
      if (bi > 0) {
        buf[bi] = 0;
        if (n->n_interp >= cap) {
          cap *= 2;
          n->interp = realloc(n->interp, sizeof(InterpPart) * cap);
        }
        InterpPart *p = &n->interp[n->n_interp++];
        p->is_expr = 0;
        p->text = strdup(buf);
        p->expr = NULL;
        bi = 0;
      }
      i += 2;
      int depth = 1;
      int ei = 0;
      char ebuf[MAX_STRING];
      while (i < len && depth > 0) {
        if (s[i] == '{')
          depth++;
        else if (s[i] == '}') {
          depth--;
          if (depth == 0)
            break;
        }
        if (ei >= MAX_STRING - 1)
          loop_lempar_jenis("GalatBatas",
                            "ekspresi dalam #{...} melebihi %d karakter",
                            MAX_STRING - 1);
        ebuf[ei++] = s[i];
        i++;
      }
      ebuf[ei] = 0;
      if (i < len && s[i] == '}')
        i++;
      if (n->n_interp >= cap) {
        cap *= 2;
        n->interp = realloc(n->interp, sizeof(InterpPart) * cap);
      }
      InterpPart *p = &n->interp[n->n_interp++];
      p->is_expr = 1;
      p->text = strdup(ebuf);
      p->expr = NULL;
    } else {
      if (bi >= MAX_STRING - 1)
        loop_lempar_jenis("GalatBatas", "teks interpolasi melebihi %d karakter",
                          MAX_STRING - 1);
      buf[bi++] = s[i];
      i++;
    }
  }
  if (bi > 0) {
    buf[bi] = 0;
    if (n->n_interp >= cap) {
      cap *= 2;
      n->interp = realloc(n->interp, sizeof(InterpPart) * cap);
    }
    InterpPart *p = &n->interp[n->n_interp++];
    p->is_expr = 0;
    p->text = strdup(buf);
    p->expr = NULL;
  }
}

static int ada_interpolasi(const char *s) { return strstr(s, "#{") != NULL; }

static Node **urai_blokKurung(Parser *P, int *out_n) {
  makan_token(P, TK_LBRACE);
  Node **stmts = NULL;
  int n = 0, cap = 0;
  while (pcur(P)->type != TK_RBRACE && pcur(P)->type != TK_EOF) {
    if (n >= cap) {
      cap = cap ? cap * 2 : 8;
      stmts = realloc(stmts, sizeof(Node *) * cap);
    }
    stmts[n++] = urai_pernyataan(P);
  }
  makan_token(P, TK_RBRACE);
  *out_n = n;
  return stmts;
}

static Node **urai_blokJika(Parser *P, int *out_n, int *hit_selain) {
  Node **stmts = NULL;
  int n = 0, cap = 0;
  while (pcur(P)->type != TK_EOF) {
    TokenType t = pcur(P)->type;
    if (t == TK_AKHIR || t == TK_SELAIN)
      break;
    if (n >= cap) {
      cap = cap ? cap * 2 : 8;
      stmts = realloc(stmts, sizeof(Node *) * cap);
    }
    stmts[n++] = urai_pernyataan(P);
  }
  *hit_selain = (pcur(P)->type == TK_SELAIN);
  *out_n = n;
  return stmts;
}

static Node *urai_pernyataan(Parser *P) {
  Token *cur = pcur(P);
  int line = cur->line;
  if (cur->type == TK_ANGKA || cur->type == TK_TEKS || cur->type == TK_BOOL_T ||
      cur->type == TK_LARIK || cur->type == TK_BITA_T) {
    char tipe[MAX_IDENT];
    snprintf(tipe, MAX_IDENT, "%s", cur->val);
    makan_token(P, cur->type);
    Token *id = makan_token(P, TK_IDENT);
    makan_token(P, TK_ASSIGN);
    Node *val = parse_expr(P);
    Node *n = node_baru(N_ASSIGN, line);
    n->varname = strdup(id->val);
    n->var_tipe = strdup(tipe);
    n->right = val;
    return n;
  }
  if (cur->type == TK_MILIK) {
    makan_token(P, TK_MILIK);
    Token *id = makan_token(P, TK_IDENT);
    makan_token(P, TK_ASSIGN);
    Node *val = parse_expr(P);
    Node *n = node_baru(N_ASSIGN, line);
    n->varname = strdup(id->val);
    /* tandai sebagai deklarasi (beda dari 'x = ..'); validasi_tipe mengabaikan
     * "milik" */
    n->var_tipe = strdup("milik");
    n->right = val;
    return n;
  }
  switch (cur->type) {
  case TK_CETAK: {
    makan_token(P, TK_CETAK);
    Node *n = node_baru(N_CETAK, line);
    n->left = parse_expr(P);
    return n;
  }
  case TK_JALANKAN: {
    makan_token(P, TK_JALANKAN);
    Node *n = node_baru(N_JALANKAN, line);
    n->left = parse_expr(P);
    return n;
  }
  case TK_BACAFILE: {
    makan_token(P, TK_BACAFILE);
    makan_token(P, TK_LPAREN);
    Token *path = makan_token(P, TK_STRING);
    makan_token(P, TK_RPAREN);
    Node *n = node_baru(N_BACA_FILE, line);
    n->str = strdup(path->val);
    return n;
  }
  case TK_PAKAI: {
    makan_token(P, TK_PAKAI);
    Token *nm = makan_token(P, TK_IDENT);
    Node *n = node_baru(N_PAKAI, line);
    n->str = strdup(nm->val);
    return n;
  }
  case TK_TULISFILE: {
    makan_token(P, TK_TULISFILE);
    makan_token(P, TK_LPAREN);
    Token *path = makan_token(P, TK_STRING);
    makan_token(P, TK_COMMA);
    Node *isi = parse_expr(P);
    makan_token(P, TK_RPAREN);
    Node *n = node_baru(N_TULIS_FILE, line);
    n->str = strdup(path->val);
    n->left = isi;
    return n;
  }
  case TK_FUNGSI: {
    makan_token(P, TK_FUNGSI);
    Token *nm = makan_token(P, TK_IDENT);
    makan_token(P, TK_LPAREN);
    Node *n = node_baru(N_FUNGSI, line);
    n->fname = strdup(nm->val);
    n->n_params = 0;
    int params_cap = 0;
    n->params = NULL;
    while (pcur(P)->type != TK_RPAREN && pcur(P)->type != TK_EOF) {
      if (n->n_params >= params_cap) {
        params_cap = params_cap ? params_cap * 2 : 4;
        n->params = realloc(n->params, sizeof(char *) * params_cap);
      }
      n->params[n->n_params++] = strdup(makan_token(P, TK_IDENT)->val);
      if (pcur(P)->type == TK_COMMA)
        makan_token(P, TK_COMMA);
    }
    makan_token(P, TK_RPAREN);
    n->body = urai_blokKurung(P, &n->n_body);
    return n;
  }
  case TK_KEMBALI: {
    makan_token(P, TK_KEMBALI);
    Node *n = node_baru(N_KEMBALI, line);
    n->left = parse_expr(P);
    return n;
  }
  case TK_HENTIKAN:
    makan_token(P, TK_HENTIKAN);
    return node_baru(N_HENTIKAN, line);
  case TK_LEWATI:
    makan_token(P, TK_LEWATI);
    return node_baru(N_LEWATI, line);
  case TK_JIKA: {
    makan_token(P, TK_JIKA);
    Node *cond = parse_expr(P);
    makan_token(P, TK_MAKA);
    Node *n = node_baru(N_JIKA, line);
    n->cond = cond;
    int hit_selain = 0;
    n->body = urai_blokJika(P, &n->n_body, &hit_selain);
    if (hit_selain) {
      makan_token(P, TK_SELAIN);
      int dummy = 0;
      n->selain = urai_blokJika(P, &n->n_selain, &dummy);
    }
    makan_token(P, TK_AKHIR);
    return n;
  }
  case TK_SELAMA: {
    makan_token(P, TK_SELAMA);
    Node *cond = parse_expr(P);
    Node *n = node_baru(N_SELAMA, line);
    n->cond = cond;
    n->body = urai_blokKurung(P, &n->n_body);
    return n;
  }
  case TK_ULANG: {
    makan_token(P, TK_ULANG);
    Node *hitungan = parse_expr(P);
    Node *n = node_baru(N_ULANG, line);
    n->left = hitungan;
    n->body = urai_blokKurung(P, &n->n_body);
    return n;
  }
  case TK_PILIH: {
    makan_token(P, TK_PILIH);
    Node *eksp = parse_expr(P);
    Node *n = node_baru(N_PILIH, line);
    n->left = eksp;
    n->n_kasus = 0;
    n->lainnya = NULL;
    n->kasus_val = calloc(MAX_ARGS, sizeof(Node *));
    n->kasus_body = calloc(MAX_ARGS, sizeof(Node *));
    makan_token(P, TK_LBRACE);
    while (pcur(P)->type != TK_RBRACE && pcur(P)->type != TK_EOF) {
      if (pcur(P)->type == TK_KASUS) {
        makan_token(P, TK_KASUS);
        n->kasus_val[n->n_kasus] = parse_expr(P);
        makan_token(P, TK_PANAH);
        n->kasus_body[n->n_kasus] = urai_pernyataan(P);
        n->n_kasus++;
      } else if (pcur(P)->type == TK_SELAIN) {
        makan_token(P, TK_SELAIN);
        makan_token(P, TK_PANAH);
        n->lainnya = urai_pernyataan(P);
      } else {
        makan_token(P, 0);
      }
    }
    makan_token(P, TK_RBRACE);
    return n;
  }
  case TK_COBA: {
    makan_token(P, TK_COBA);
    Node *n = node_baru(N_COBA, line);
    n->body = urai_blokKurung(P, &n->n_body);
    if (pcur(P)->type == TK_TANGKAP) {
      makan_token(P, TK_TANGKAP);
      n->tangkap = urai_blokKurung(P, &n->n_tangkap);
      n->has_tangkap = 1;
    }
    if (pcur(P)->type == TK_AKHIRNYA) {
      makan_token(P, TK_AKHIRNYA);
      n->akhirnya = urai_blokKurung(P, &n->n_akhirnya);
      n->has_akhirnya = 1;
    }
    if (!n->has_tangkap && !n->has_akhirnya)
      loop_galat("Baris %d: 'coba' harus diikuti 'tangkap' atau 'akhirnya'",
                 line);
    return n;
  }
  case TK_LEMPAR: {
    makan_token(P, TK_LEMPAR);
    Node *n = node_baru(N_LEMPAR, line);
    n->left = parse_expr(P);
    return n;
  }
  default:
    break;
  }
  if (cur->type == TK_IDENT) {
    TokenType nxt = intip_token(P)->type;
    if (nxt == TK_PLUSEQ || nxt == TK_MINUSEQ || nxt == TK_STAREQ ||
        nxt == TK_SLASHEQ || nxt == TK_PRSNEQ) {
      Token *id = makan_token(P, TK_IDENT);
      Token *op = makan_token(P, nxt);
      Node *val = parse_expr(P);
      Node *n = node_baru(N_ASSIGN_COMPOUND, line);
      n->varname = strdup(id->val);
      strncpy(n->op, op->val, 8);
      n->right = val;
      return n;
    }
    if (nxt == TK_ASSIGN) {
      Token *id = makan_token(P, TK_IDENT);
      makan_token(P, TK_ASSIGN);
      Node *val = parse_expr(P);
      Node *n = node_baru(N_ASSIGN, line);
      n->varname = strdup(id->val);
      n->right = val;
      return n;
    }
  }
  return parse_expr(P);
}

static Node *parse_expr(Parser *P) { return urai_pipa(P); }

static Node *urai_pipa(Parser *P) {
  Node *kiri = urai_logika(P);
  while (pcur(P)->type == TK_PIPA) {
    int line = pcur(P)->line;
    makan_token(P, TK_PIPA);
    Token *fn = makan_token(P, TK_IDENT);
    Node *n = node_baru(N_PIPA, line);
    n->pipa_kiri = kiri;
    n->pipa_fn = strdup(fn->val);
    n->pipa_extra = NULL;
    n->n_pipa_extra = 0;
    if (pcur(P)->type == TK_LPAREN) {
      makan_token(P, TK_LPAREN);
      int cap = 0;
      while (pcur(P)->type != TK_RPAREN && pcur(P)->type != TK_EOF) {
        if (n->n_pipa_extra >= cap) {
          cap = cap ? cap * 2 : 4;
          n->pipa_extra = realloc(n->pipa_extra, sizeof(Node *) * cap);
        }
        n->pipa_extra[n->n_pipa_extra++] = parse_expr(P);
        if (pcur(P)->type == TK_COMMA)
          makan_token(P, TK_COMMA);
      }
      makan_token(P, TK_RPAREN);
    }
    kiri = n;
  }
  return kiri;
}

static Node *urai_logika(Parser *P) {
  Node *kiri = urai_perbandingan(P);
  while (pcur(P)->type == TK_DAN || pcur(P)->type == TK_ATAU) {
    Token *op = makan_token(P, pcur(P)->type);
    Node *n = node_baru(N_BINOP, op->line);
    strncpy(n->op, op->val, 8);
    n->left = kiri;
    n->right = urai_perbandingan(P);
    kiri = n;
  }
  return kiri;
}

static Node *urai_perbandingan(Parser *P) {
  Node *kiri = urai_tambahKurang(P);
  while (pcur(P)->type == TK_EQ || pcur(P)->type == TK_NEQ ||
         pcur(P)->type == TK_LT || pcur(P)->type == TK_GT ||
         pcur(P)->type == TK_LEQ || pcur(P)->type == TK_GEQ) {
    Token *op = makan_token(P, pcur(P)->type);
    Node *n = node_baru(N_BINOP, op->line);
    strncpy(n->op, op->val, 8);
    n->left = kiri;
    n->right = urai_tambahKurang(P);
    kiri = n;
  }
  return kiri;
}

static Node *urai_tambahKurang(Parser *P) {
  Node *kiri = urai_kaliBagi(P);
  while (pcur(P)->type == TK_PLUS || pcur(P)->type == TK_MINUS) {
    Token *op = makan_token(P, pcur(P)->type);
    Node *n = node_baru(N_BINOP, op->line);
    strncpy(n->op, op->val, 8);
    n->left = kiri;
    n->right = urai_kaliBagi(P);
    kiri = n;
  }
  return kiri;
}

static Node *urai_kaliBagi(Parser *P) {
  Node *kiri = urai_uner(P);
  while (pcur(P)->type == TK_BINTANG || pcur(P)->type == TK_SLASH ||
         pcur(P)->type == TK_PERSEN) {
    Token *op = makan_token(P, pcur(P)->type);
    Node *n = node_baru(N_BINOP, op->line);
    strncpy(n->op, op->val, 8);
    n->left = kiri;
    n->right = urai_uner(P);
    kiri = n;
  }
  return kiri;
}

static Node *urai_uner(Parser *P) {
  if (pcur(P)->type == TK_TIDAK) {
    Token *op = makan_token(P, TK_TIDAK);
    Node *n = node_baru(N_UNIOP, op->line);
    strncpy(n->op, "tidak", 8);
    n->left = urai_uner(P);
    return n;
  }
  if (pcur(P)->type == TK_MINUS) {
    Token *op = makan_token(P, TK_MINUS);
    Node *n = node_baru(N_UNIOP, op->line);
    strncpy(n->op, "-", 8);
    n->left = urai_uner(P);
    return n;
  }
  return urai_primer(P);
}

static Node *urai_primer(Parser *P) {
  Token *cur = pcur(P);
  int line = cur->line;
  if (cur->type == TK_NUMBER) {
    makan_token(P, TK_NUMBER);
    Node *n = node_baru(N_LIT_NUM, line);
    n->num = cur->num;
    return n;
  }
  if (cur->type == TK_STRING) {
    makan_token(P, TK_STRING);
    if (ada_interpolasi(cur->val)) {
      Node *n = node_baru(N_STRING_INTERP, line);
      urai_interpolasiString(n, cur->val);
      return n;
    }
    Node *n = node_baru(N_LIT_STR, line);
    n->str = strdup(cur->val);
    return n;
  }
  if (cur->type == TK_BOOL_BENAR) {
    makan_token(P, TK_BOOL_BENAR);
    Node *n = node_baru(N_LIT_BOOL, line);
    n->bval = 1;
    return n;
  }
  if (cur->type == TK_BOOL_SALAH) {
    makan_token(P, TK_BOOL_SALAH);
    Node *n = node_baru(N_LIT_BOOL, line);
    n->bval = 0;
    return n;
  }
  /* N_LIT_NULL sudah ditangani evaluasi, tapi dulu tidak ada kata kunci yang
   menghasilkannya, jadi literal null tidak bisa ditulis. */
  if (cur->type == TK_NULL) {
    makan_token(P, TK_NULL);
    return node_baru(N_LIT_NULL, line);
  }
  if (cur->type == TK_LBRACKET) {
    makan_token(P, TK_LBRACKET);
    Node *n = node_baru(N_ARRAY, line);
    n->elems = NULL;
    n->n_elems = 0;
    int cap = 0;
    while (pcur(P)->type != TK_RBRACKET && pcur(P)->type != TK_EOF) {
      if (n->n_elems >= cap) {
        cap = cap ? cap * 2 : 8;
        n->elems = realloc(n->elems, sizeof(Node *) * cap);
      }
      n->elems[n->n_elems++] = parse_expr(P);
      if (pcur(P)->type == TK_COMMA)
        makan_token(P, TK_COMMA);
    }
    makan_token(P, TK_RBRACKET);
    Node *base = n;
    while (pcur(P)->type == TK_LBRACKET) {
      makan_token(P, TK_LBRACKET);
      Node *idx = parse_expr(P);
      makan_token(P, TK_RBRACKET);
      Node *in = node_baru(N_INDEX, line);
      in->obj = base;
      in->idx = idx;
      base = in;
    }
    return base;
  }
  if (cur->type == TK_LPAREN) {
    makan_token(P, TK_LPAREN);
    Node *n = parse_expr(P);
    makan_token(P, TK_RPAREN);
    return n;
  }
  if (cur->type == TK_IDENT) {
    Token *id = makan_token(P, TK_IDENT);
    if (pcur(P)->type == TK_LPAREN) {
      makan_token(P, TK_LPAREN);
      Node *n = node_baru(N_PANGGIL, line);
      n->fname = strdup(id->val);
      n->args = NULL;
      n->n_args = 0;
      int cap = 0;
      while (pcur(P)->type != TK_RPAREN && pcur(P)->type != TK_EOF) {
        if (n->n_args >= cap) {
          cap = cap ? cap * 2 : 4;
          n->args = realloc(n->args, sizeof(Node *) * cap);
        }
        n->args[n->n_args++] = parse_expr(P);
        if (pcur(P)->type == TK_COMMA)
          makan_token(P, TK_COMMA);
      }
      makan_token(P, TK_RPAREN);
      Node *base = n;
      while (pcur(P)->type == TK_LBRACKET) {
        makan_token(P, TK_LBRACKET);
        Node *idx = parse_expr(P);
        makan_token(P, TK_RBRACKET);
        Node *in = node_baru(N_INDEX, line);
        in->obj = base;
        in->idx = idx;
        base = in;
      }
      return base;
    }
    Node *n = node_baru(N_VAR, line);
    n->str = strdup(id->val);
    Node *base = n;
    while (pcur(P)->type == TK_LBRACKET) {
      makan_token(P, TK_LBRACKET);
      Node *idx = parse_expr(P);
      makan_token(P, TK_RBRACKET);
      Node *in = node_baru(N_INDEX, line);
      in->obj = base;
      in->idx = idx;
      base = in;
    }
    return base;
  }
  loop_galat("Baris %d: Token tak terduga: '%s'", line, cur->val);
  return NULL;
}

static Node *urai_program(Token *tokens, int n_tokens) {
  Parser P;
  P.tokens = tokens;
  P.pos = 0;
  P.n = n_tokens;
  Node *prog = node_baru(N_PROGRAM, 1);
  prog->body = NULL;
  prog->n_body = 0;
  int cap = 0;
  while (pcur(&P)->type != TK_EOF) {
    if (prog->n_body >= cap) {
      cap = cap ? cap * 2 : 16;
      prog->body = realloc(prog->body, sizeof(Node *) * cap);
    }
    prog->body[prog->n_body++] = urai_pernyataan(&P);
  }
  return prog;
}

typedef enum {
  VAL_NULL = 0,
  VAL_NUM,
  VAL_STR,
  VAL_BOOL,
  VAL_ARRAY,
  VAL_FUNC,
  VAL_PRIMA,
  VAL_ENKRIPSI,
  VAL_GANDA,
  VAL_KONEKSI,
  VAL_BITA,
  VAL_GALAT,
} ValType;

typedef struct Val Val;
struct Val {
  ValType type;
  int refs;
  double num;
  char *str;
  int bval;
  Val **arr;
  int arr_n;
  Node *func_node;
  uint64_t prima_kunci;
  unsigned char *enc_data;
  int enc_len;
  char *enc_hex;
  uint64_t enc_seed;
  double ganda_asli;
  char *ganda_enc;
  int kon_fd;
  int kon_tls;
  void *kon_ssl;
  void *kon_ctx;
  char *kon_host;
  int kon_port;
  unsigned char *bita; /* VAL_BITA: isi biner, tanpa terminator */
  size_t bita_n;
  char *galat_jenis; /* VAL_GALAT: pesan ada di str */
  char *galat_lokasi;
  char *galat_jejak;
  Val *galat_sebab;
};

static Val *val_kosong(void) {
  Val *v = calloc(1, sizeof(Val));
  v->type = VAL_NULL;
  v->refs = 1;
  return v;
}
static Val *val_angka(double n) {
  Val *v = calloc(1, sizeof(Val));
  v->type = VAL_NUM;
  v->num = n;
  v->refs = 1;
  return v;
}
static Val *val_boolean(int b) {
  Val *v = calloc(1, sizeof(Val));
  v->type = VAL_BOOL;
  v->bval = b;
  v->refs = 1;
  return v;
}
static Val *val_teks(const char *s) {
  Val *v = calloc(1, sizeof(Val));
  v->type = VAL_STR;
  v->str = strdup(s);
  v->refs = 1;
  return v;
}
static Val *val_larik(Val **arr, int n) {
  Val *v = calloc(1, sizeof(Val));
  v->type = VAL_ARRAY;
  v->arr = arr;
  v->arr_n = n;
  v->refs = 1;
  return v;
}
static Val *val_fungsi(Node *fn) {
  Val *v = calloc(1, sizeof(Val));
  v->type = VAL_FUNC;
  v->func_node = fn;
  v->refs = 1;
  return v;
}

static void val_hapus(Val *v) {
  if (!v)
    return;
  if (--v->refs > 0)
    return;
  if (v->str)
    free(v->str);
  if (v->arr) {
    for (int i = 0; i < v->arr_n; i++)
      val_hapus(v->arr[i]);
    free(v->arr);
  }
  if (v->bita)
    free(v->bita);
  if (v->galat_jenis)
    free(v->galat_jenis);
  if (v->galat_lokasi)
    free(v->galat_lokasi);
  if (v->galat_jejak)
    free(v->galat_jejak);
  if (v->galat_sebab)
    val_hapus(v->galat_sebab);
  if (v->enc_data)
    free(v->enc_data);
  if (v->enc_hex)
    free(v->enc_hex);
  if (v->ganda_enc)
    free(v->ganda_enc);
  if (v->type == VAL_KONEKSI) {
    if (v->kon_ssl) {
      SSL_shutdown((SSL *)v->kon_ssl);
      SSL_free((SSL *)v->kon_ssl);
    }
    if (v->kon_ctx)
      SSL_CTX_free((SSL_CTX *)v->kon_ctx);
    if (v->kon_fd >= 0)
      close(v->kon_fd);
    if (v->kon_host)
      free(v->kon_host);
  }
  free(v);
}

static Val *val_referensi(Val *v) {
  if (v)
    v->refs++;
  return v;
}

static char *val_ke_teks(Val *v);
/* Versi berbatas untuk pemanggil lama; data jangan lewat sini (dipotong). */
static void val_format(Val *v, char *buf, int bufsz) {
  char *t = val_ke_teks(v);
  snprintf(buf, (size_t)bufsz, "%s", t);
  free(t);
}

/* Kesamaan nilai untuk ==, != dan 'pilih/kasus'.
   Sebelumnya == hanya membandingkan teks-vs-teks lewat strcmp, selain itu
 L->num == R->num. Boolean hanya mengisi bval (num selalu 0), dan null/larik
 juga num == 0, sehingga: 'benar == salah' -> benar, 'null == 0' -> benar,
'"abc" == 0' -> benar, dan dua larik apa pun dianggap sama.
Aturan baru, KHUSUS di antara tipe dasar (null/angka/teks/bool/larik): tipe
beda -> tidak sama; tipe sama -> bandingkan isinya (larik rekursif). Tipe
khusus (fungsi, kunci_prima, terenkripsi, ganda, koneksi) memakai perilaku
lama supaya kode yang bergantung padanya tidak berubah. */
static int val_tipe_dasar(const Val *v) {
  return v->type == VAL_NULL || v->type == VAL_NUM || v->type == VAL_STR ||
         v->type == VAL_BOOL || v->type == VAL_ARRAY || v->type == VAL_BITA;
}

static int val_sama(const Val *a, const Val *b) {
  if (!val_tipe_dasar(a) || !val_tipe_dasar(b)) {
    if (a->type == VAL_STR && b->type == VAL_STR)
      return strcmp(a->str, b->str) == 0;
    return a->num == b->num; /* perilaku lama untuk tipe khusus */
  }
  if (a->type != b->type)
    return 0;
  switch (a->type) {
  case VAL_NULL:
    return 1;
  case VAL_NUM:
    return a->num == b->num;
  case VAL_STR:
    return strcmp(a->str, b->str) == 0;
  case VAL_BOOL:
    return (a->bval != 0) == (b->bval != 0);
  case VAL_BITA:
    return a->bita_n == b->bita_n &&
           (a->bita_n == 0 || memcmp(a->bita, b->bita, a->bita_n) == 0);
  case VAL_ARRAY:
    if (a->arr_n != b->arr_n)
      return 0;
    for (int i = 0; i < a->arr_n; i++) {
      const Val *x = a->arr[i], *y = b->arr[i];
      if (!x || !y) {
        if (x != y)
          return 0;
        continue;
      }
      if (!val_sama(x, y))
        return 0;
    }
    return 1;
  default:
    return 0;
  }
}

/* ===== String builder dinamis: tidak ada batas diam-diam ===== */
typedef struct {
  char *p;
  size_t n, cap;
} Sb;

static size_t g_maks_ukuran = (size_t)64 * 1024 * 1024; /* LOOP_MAKS_UKURAN */

static void sb_awal(Sb *s) {
  s->cap = 64;
  s->n = 0;
  s->p = malloc(s->cap);
  if (!s->p) {
    fprintf(stderr, "[LOOP] Kehabisan memori\n");
    exit(1);
  }
  s->p[0] = 0;
}

static void sb_tambah(Sb *s, const char *d, size_t len) {
  if (s->n + len > g_maks_ukuran) {
    free(s->p);
    s->p = NULL;
    loop_lempar_jenis("GalatBatas",
                      "ukuran teks melewati batas %zu byte (atur LOOP_MAKS_UKURAN)",
                      g_maks_ukuran);
  }
  if (s->n + len + 1 > s->cap) {
    while (s->n + len + 1 > s->cap)
      s->cap *= 2;
    char *np = realloc(s->p, s->cap);
    if (!np) {
      fprintf(stderr, "[LOOP] Kehabisan memori\n");
      exit(1);
    }
    s->p = np;
  }
  memcpy(s->p + s->n, d, len);
  s->n += len;
  s->p[s->n] = 0;
}

static void sb_str(Sb *s, const char *z) { sb_tambah(s, z, strlen(z)); }

static void sb_fmt(Sb *s, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));
static void sb_fmt(Sb *s, const char *fmt, ...) {
  char tmp[512];
  va_list ap;
  va_start(ap, fmt);
  int k = vsnprintf(tmp, sizeof(tmp), fmt, ap);
  va_end(ap);
  if (k < 0)
    return;
  if ((size_t)k < sizeof(tmp)) {
    sb_tambah(s, tmp, (size_t)k);
    return;
  }
  char *big = malloc((size_t)k + 1);
  va_start(ap, fmt);
  vsnprintf(big, (size_t)k + 1, fmt, ap);
  va_end(ap);
  sb_tambah(s, big, (size_t)k);
  free(big);
}

/* Periksa ukuran terhadap batas konfigurasi; melewati batas = GalatBatas. */
static void periksa_batas(size_t n, const char *apa) {
  if (n > g_maks_ukuran)
    loop_lempar_jenis("GalatBatas",
                      "%s melewati batas %zu byte (atur LOOP_MAKS_UKURAN)", apa,
                      g_maks_ukuran);
}

/* Teks lengkap sebuah nilai; hasil malloc, tidak pernah dipotong. */
static char *val_ke_teks(Val *v) {
  Sb s;
  sb_awal(&s);
  if (!v) {
    sb_str(&s, "kosong");
    return s.p;
  }
  switch (v->type) {
  case VAL_NULL:
    sb_str(&s, "kosong");
    break;
  case VAL_BOOL:
    sb_str(&s, v->bval ? "benar" : "salah");
    break;
  case VAL_NUM: {
    double n = v->num;
    if (n == (int64_t)n)
      sb_fmt(&s, "%lld", (long long)(int64_t)n);
    else
      sb_fmt(&s, "%g", n);
    break;
  }
  case VAL_STR:
    sb_str(&s, v->str ? v->str : "");
    break;
  case VAL_FUNC:
    sb_fmt(&s, "<fungsi %s>", v->func_node ? v->func_node->fname : "?");
    break;
  case VAL_ARRAY:
    sb_str(&s, "[");
    for (int i = 0; i < v->arr_n; i++) {
      if (i)
        sb_str(&s, ", ");
      char *t = val_ke_teks(v->arr[i]);
      sb_str(&s, t);
      free(t);
    }
    sb_str(&s, "]");
    break;
  case VAL_PRIMA:
    sb_fmt(&s, "[KunciPrima | kunci=%llu]", (unsigned long long)v->prima_kunci);
    break;
  case VAL_ENKRIPSI:
    sb_fmt(&s, "[Terenkripsi | hex=%s | seed=%llu]",
           v->enc_hex ? v->enc_hex : "?", (unsigned long long)v->enc_seed);
    break;
  case VAL_GANDA:
    sb_fmt(&s, "[Ganda | asli=%g | enc=%s]", v->ganda_asli,
           v->ganda_enc ? v->ganda_enc : "?");
    break;
  case VAL_KONEKSI:
    sb_fmt(&s, "[Koneksi | %s:%d | %s | %s]", v->kon_host ? v->kon_host : "?",
           v->kon_port, v->kon_tls ? "tls" : "tcp",
           v->kon_fd >= 0 ? "terbuka" : "tertutup");
    break;
  case VAL_BITA:
    sb_fmt(&s, "<bita %zu byte>", v->bita_n);
    break;
  case VAL_GALAT:
    /* Jembatan kode lama: "" + e menghasilkan teks "Jenis: pesan". */
    sb_fmt(&s, "%s: %s", v->galat_jenis ? v->galat_jenis : "Galat",
           v->str ? v->str : "");
    break;
  default:
    sb_str(&s, "?");
    break;
  }
  return s.p;
}

/* ===== Objek galat, jejak pemanggilan, dan pelemparan ===== */
/* Ambil alih buffer malloc (tanpa salin). */
static Val *val_teks_ambil(char *s) {
  Val *v = calloc(1, sizeof(Val));
  v->type = VAL_STR;
  v->str = s;
  v->refs = 1;
  return v;
}

/* Penyambung teks untuk + dan +=: tidak memotong; melewati batas = GalatBatas;
   bita tidak boleh digabung diam-diam dengan teks. */
static Val *gabung_dua(Val *a, Val *b) {
  if (a->type == VAL_BITA || b->type == VAL_BITA)
    loop_lempar_jenis("GalatTipe",
                      "tidak bisa menggabung bita dengan + (gunakan sambungBita() atau keHex())");
  char *x = val_ke_teks(a), *y = val_ke_teks(b);
  size_t lx = strlen(x), ly = strlen(y);
  if (lx + ly > g_maks_ukuran) {
    free(x);
    free(y);
    loop_lempar_jenis("GalatBatas",
                      "hasil penggabungan teks melewati batas %zu byte (atur LOOP_MAKS_UKURAN)",
                      g_maks_ukuran);
  }
  char *s = malloc(lx + ly + 1);
  memcpy(s, x, lx);
  memcpy(s + lx, y, ly + 1);
  free(x);
  free(y);
  return val_teks_ambil(s);
}

static Val *val_teks_n(const char *s, size_t n) {
  Val *v = calloc(1, sizeof(Val));
  v->type = VAL_STR;
  v->str = malloc(n + 1);
  memcpy(v->str, s, n);
  v->str[n] = 0;
  v->refs = 1;
  return v;
}

static Val *val_bita(const unsigned char *d, size_t n) {
  Val *v = calloc(1, sizeof(Val));
  v->type = VAL_BITA;
  v->refs = 1;
  v->bita = malloc(n ? n : 1);
  if (n)
    memcpy(v->bita, d, n);
  v->bita_n = n;
  return v;
}

static Val *val_galat(const char *jenis, const char *pesan, Val *sebab) {
  Val *v = calloc(1, sizeof(Val));
  v->type = VAL_GALAT;
  v->refs = 1;
  v->str = strdup(pesan);
  v->galat_jenis = strdup(jenis);
  v->galat_sebab = sebab ? val_referensi(sebab) : NULL;
  return v;
}

typedef struct {
  const char *fn;
  const char *berkas_lama; /* berkas pemanggil saat fungsi dipanggil */
  int baris_panggil;       /* baris pemanggilan di berkas pemanggil */
} Frame;
static Frame g_frames[MAX_CALL_STACK + 1];
static int g_frames_n = 0;

static void frame_masuk(const char *fn, Node *fnode, int line) {
  if (g_frames_n >= MAX_CALL_STACK)
    loop_lempar_jenis("GalatBatas",
                      "rekursi terlalu dalam (batas %d pemanggilan fungsi)",
                      MAX_CALL_STACK);
  Frame *f = &g_frames[g_frames_n++];
  f->fn = fn ? fn : "<fungsi>";
  f->baris_panggil = line;
  f->berkas_lama = g_berkas_aktif;
  if (fnode && fnode->berkas)
    g_berkas_aktif = fnode->berkas;
}

static void frame_keluar(void) {
  if (g_frames_n <= 0)
    return;
  Frame *f = &g_frames[--g_frames_n];
  g_berkas_aktif = f->berkas_lama;
}

static char *bangun_jejak(void) {
  Sb s;
  sb_awal(&s);
  const char *berkas = g_berkas_aktif;
  int baris = g_baris_kini;
  for (int i = g_frames_n - 1; i >= 0; i--) {
    sb_fmt(&s, "  di %s (%s:%d)\n", g_frames[i].fn, berkas, baris);
    berkas = g_frames[i].berkas_lama;
    baris = g_frames[i].baris_panggil;
  }
  sb_fmt(&s, "  di <utama> (%s:%d)\n", berkas, baris);
  return s.p;
}

static void cetak_galat_takTertangkap(Val *g) {
  fflush(stdout);
  fprintf(stderr, "[GALAT] %s: %s\n", g->galat_jenis, g->str);
  if (g->galat_jejak)
    fputs(g->galat_jejak, stderr);
  for (Val *c = g->galat_sebab; c; c = c->galat_sebab) {
    fprintf(stderr, "Disebabkan oleh: %s: %s\n", c->galat_jenis, c->str);
    if (c->galat_jejak)
      fputs(c->galat_jejak, stderr);
  }
}

/* Lempar objek galat. Mengambil alih satu referensi milik pemanggil. */
static void loop_lempar(Val *g) __attribute__((noreturn));
static void loop_lempar(Val *g) {
  if (!g->galat_jejak) { /* pertama kali dilempar: catat lokasi dan jejak */
    char lok[1024];
    snprintf(lok, sizeof(lok), "%s:%d", g_berkas_aktif, g_baris_kini);
    g->galat_lokasi = strdup(lok);
    g->galat_jejak = bangun_jejak();
  }
  if (g_handler) {
    g_galat_aktif = g;
    longjmp(g_handler->jb, 1);
  }
  cetak_galat_takTertangkap(g);
  if (g_in_repl) {
    g_handler = NULL;
    g_frames_n = 0;
    longjmp(g_repl_jmp, 1);
  }
  exit(1);
}

static void loop_lempar_jenis(const char *jenis, const char *fmt, ...) {
  char stack_buf[1024];
  va_list ap;
  va_start(ap, fmt);
  int k = vsnprintf(stack_buf, sizeof(stack_buf), fmt, ap);
  va_end(ap);
  char *msg = stack_buf;
  char *heap = NULL;
  if (k >= (int)sizeof(stack_buf)) {
    heap = malloc((size_t)k + 1);
    va_start(ap, fmt);
    vsnprintf(heap, (size_t)k + 1, fmt, ap);
    va_end(ap);
    msg = heap;
  }
  Val *g = val_galat(jenis, msg, NULL);
  free(heap);
  loop_lempar(g);
}

typedef struct Env Env;
struct Env {
  char names[MAX_ENV_VARS][MAX_IDENT];
  Val *vals[MAX_ENV_VARS];
  int n;
  Env *parent;
  /* 1 untuk frame pemanggilan fungsi; dipakai env_deklarasi(). */
  int is_frame;
  /* Daftar Env yang masih hidup (terbaru di g_env_live). Dipakai jalur galat:
     longjmp melompati env_hapus milik frame yang dilewati, jadi saat galat
     tertangkap semua Env yang dibuat sejak blok `coba` dimulai dibebaskan
     (lihat evaluasi_blok_terlindung). */
  Env *live_prev, *live_next;
};

static Env *g_env_live = NULL;

static Env *env_baru(Env *parent) {
  Env *e = calloc(1, sizeof(Env));
  e->parent = parent;
  e->n = 0;
  e->live_prev = g_env_live;
  if (g_env_live)
    g_env_live->live_next = e;
  g_env_live = e;
  return e;
}

static Val *env_ambil(Env *e, const char *name) {
  for (; e; e = e->parent)
    for (int i = e->n - 1; i >= 0; i--)
      if (strcmp(e->names[i], name) == 0)
        return e->vals[i];
  loop_galat("Variabel tidak ditemukan: '%s'", name);
  return NULL;
}

static void env_atur(Env *e, const char *name, Val *v) {
  for (Env *cur = e; cur; cur = cur->parent)
    for (int i = cur->n - 1; i >= 0; i--)
      if (strcmp(cur->names[i], name) == 0) {
        val_hapus(cur->vals[i]);
        cur->vals[i] = val_referensi(v);
        return;
      }
  if (e->n >= MAX_ENV_VARS)
    loop_lempar_jenis("GalatBatas", "Terlalu banyak variabel (batas %d)", MAX_ENV_VARS);
  strncpy(e->names[e->n], name, MAX_IDENT);
  e->vals[e->n] = val_referensi(v);
  e->n++;
}

static void env_aturLokal(Env *e, const char *name, Val *v) {
  for (int i = e->n - 1; i >= 0; i--)
    if (strcmp(e->names[i], name) == 0) {
      val_hapus(e->vals[i]);
      e->vals[i] = val_referensi(v);
      return;
    }
  if (e->n >= MAX_ENV_VARS)
    loop_lempar_jenis("GalatBatas", "Terlalu banyak variabel lokal (batas %d)", MAX_ENV_VARS);
  strncpy(e->names[e->n], name, MAX_IDENT);
  e->vals[e->n] = val_referensi(v);
  e->n++;
}

/* Untuk DEKLARASI ('angka x = ..', 'teks/bool/larik x = ..', 'milik x = ..').
   Berbeda dari env_atur (assignment biasa 'x = ..'), pencarian ke atas berhenti
   di frame fungsi: deklarasi di dalam fungsi TIDAK menimpa binding global.
 Tanpa ini 'teks isi = ..' di dalam sebuah fungsi menghapus fungsi global
 isi() (fungsi disimpan sebagai variabel di env global). Deklarasi di blok
 bersarang dalam fungsi tetap menemukan variabel level-fungsi, dan di
 top-level perilakunya sama persis dengan env_atur. */
static void env_deklarasi(Env *e, const char *name, Val *v) {
  for (Env *cur = e; cur; cur = cur->parent) {
    for (int i = cur->n - 1; i >= 0; i--)
      if (strcmp(cur->names[i], name) == 0) {
        val_hapus(cur->vals[i]);
        cur->vals[i] = val_referensi(v);
        return;
      }
    if (cur->is_frame)
      break;
  }
  if (e->n >= MAX_ENV_VARS)
    loop_lempar_jenis("GalatBatas", "Terlalu banyak variabel (batas %d)", MAX_ENV_VARS);
  strncpy(e->names[e->n], name, MAX_IDENT);
  e->vals[e->n] = val_referensi(v);
  e->n++;
}

static void env_hapus(Env *e) {
  if (e->live_next)
    e->live_next->live_prev = e->live_prev;
  else
    g_env_live = e->live_prev;
  if (e->live_prev)
    e->live_prev->live_next = e->live_next;
  for (int i = 0; i < e->n; i++)
    val_hapus(e->vals[i]);
  free(e);
}

static int adalah_prima(uint64_t n) {
  if (n < 2)
    return 0;
  if (n == 2)
    return 1;
  if (n % 2 == 0)
    return 0;
  for (uint64_t i = 3; i * i <= n; i += 2)
    if (n % i == 0)
      return 0;
  return 1;
}

static uint64_t prima_berikutnya(uint64_t n) {
  n++;
  while (!adalah_prima(n))
    n++;
  return n;
}

static int jumlah_faktor_prima(uint64_t n) {
  int total = 0;
  if (n < 2)
    return 0;
  for (uint64_t p = 2; p * p <= n; p++) {
    while (n % p == 0) {
      total++;
      n /= p;
    }
  }
  if (n > 1)
    total++;
  return total;
}

static uint64_t hitung_lapisan(uint64_t n, uint64_t sebelumnya, int is_first) {
  if (n < 2)
    n = 2;
  int jf = jumlah_faktor_prima(n);
  uint64_t raw = (uint64_t)jf * n;
  uint64_t mod;
  if (is_first) {
    mod = prima_berikutnya(n);
    uint64_t hasil = raw % mod;
    if (hasil == 0)
      hasil = (uint64_t)jf * mod;
    return hasil;
  } else {
    mod = sebelumnya > 1 ? sebelumnya : 2;
    uint64_t hasil = raw % mod;
    if (hasil == 0)
      hasil = (uint64_t)jf * prima_berikutnya(mod);
    return hasil;
  }
}

static void xor_bytes(unsigned char *dst, const unsigned char *src, int src_len,
                      const unsigned char *key, int key_len) {
  for (int i = 0; i < src_len; i++)
    dst[i] = src[i] ^ key[i % key_len];
}

static void acak_bytes(unsigned char *data, int len, uint64_t seed) {
  for (int i = len - 1; i > 0; i--) {
    seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
    int j = (int)(seed >> 33) % (i + 1);
    unsigned char tmp = data[i];
    data[i] = data[j];
    data[j] = tmp;
  }
}

static char *bytes_keHex(const unsigned char *data, int len) {
  char *hex = malloc(len * 2 + 1);
  for (int i = 0; i < len; i++)
    snprintf(hex + i * 2, 3, "%02x", data[i]);
  hex[len * 2] = 0;
  return hex;
}

static int g_kembali = 0;
static Val *g_kembali_v = NULL;
static int g_hentikan = 0;
static int g_lewati = 0;

/* Baca seluruh isi FILE* sampai EOF. Aman untuk input yang tidak bisa di-seek
 (pipe, FIFO, /dev/stdin): ftell() di sana mengembalikan -1, dan kode lama
 (malloc(sz + 1); fread(.., sz, ..); kode[sz] = 0) menjadi heap overflow. */
static char *baca_file_penuh(FILE *f) {
  size_t cap = 4096, len = 0, got;
  char *buf = malloc(cap);
  if (!buf) {
    fprintf(stderr, "[LOOP] Kehabisan memori\n");
    exit(1);
  }
  while ((got = fread(buf + len, 1, cap - len - 1, f)) > 0) {
    len += got;
    if (len + 1 >= cap) {
      char *nb = realloc(buf, cap * 2);
      if (!nb) {
        free(buf);
        fprintf(stderr, "[LOOP] Kehabisan memori\n");
        exit(1);
      }
      buf = nb;
      cap *= 2;
    }
  }
  buf[len] = 0;
  return buf;
}

static char g_stdlib_dir[PATH_MAX] = {0};
static char *g_pakai_loaded[64];
static int g_pakai_n_loaded = 0;

static void inisialisasi_dirStdlib(void) {
  const char *override = getenv("LOOP_STDLIB");
  if (override && override[0]) {
    strncpy(g_stdlib_dir, override, PATH_MAX - 1);
    return;
  }
  char exe[PATH_MAX] = {0};
  ssize_t len = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
  if (len <= 0) {
    strncpy(g_stdlib_dir, "Library", PATH_MAX - 1);
    return;
  }
  exe[len] = 0;
  char *dir = dirname(exe);
  snprintf(g_stdlib_dir, PATH_MAX, "%s/Library", dir);
  /* Tata letak repositori: biner di seed/, Library/ di tingkat atasnya. */
  if (access(g_stdlib_dir, R_OK) != 0) {
    char alt[PATH_MAX];
    snprintf(alt, PATH_MAX, "%s/../Library", dir);
    if (access(alt, R_OK) == 0)
      snprintf(g_stdlib_dir, PATH_MAX, "%s", alt);
  }
}

static Val *evaluasi(Node *n, Env *env);

/* Jalankan blok di bawah handler. Mengembalikan galat yang tertangkap (satu
   referensi milik pemanggil) atau NULL bila blok selesai tanpa galat. */
static Val *evaluasi_blok_terlindung(Node **stmts, int n, Env *env) {
  Handler h;
  h.prev = g_handler;
  int frames_simpan = g_frames_n;
  const char *berkas_simpan = g_berkas_aktif;
  Env *env_penanda = g_env_live;
  g_handler = &h;
  if (setjmp(h.jb) == 0) {
    for (int i = 0; i < n && !g_kembali && !g_hentikan && !g_lewati; i++)
      val_hapus(evaluasi(stmts[i], env));
    g_handler = h.prev;
    return NULL;
  }
  g_handler = h.prev;
  g_frames_n = frames_simpan;
  g_berkas_aktif = berkas_simpan;
  /* bebaskan Env milik frame/blok yang dilewati longjmp */
  while (g_env_live && g_env_live != env_penanda)
    env_hapus(g_env_live);
  g_kembali = g_hentikan = g_lewati = 0;
  g_kembali_v = NULL;
  Val *g = g_galat_aktif;
  g_galat_aktif = NULL;
  return g;
}
static Val *jalankan_kode(const char *kode, Env *env, const char *berkas);
static Env *g_global_env = NULL;
static Val *panggil_nilai_fungsi(Val *fv, Val **args, int n_args, int line);

typedef Val *(*BuiltinFn)(Val **args, int n_args, int line);

static int utf8_valid(const unsigned char *s, size_t n);
static unsigned char *baca_biner(const char *path, size_t *out_n);
static void tulis_biner(const char *path, const unsigned char *d, size_t n,
                        const char *mode);

static Val *bawaan_panjang(Val **args, int n, int line) {
  ARG_ASSERT(n == 1, "panjang() butuh 1 argumen");
  if (args[0]->type == VAL_STR)
    return val_angka((double)strlen(args[0]->str));
  if (args[0]->type == VAL_ARRAY)
    return val_angka((double)args[0]->arr_n);
  if (args[0]->type == VAL_BITA)
    return val_angka((double)args[0]->bita_n);
  loop_lempar_jenis("GalatTipe", "Baris %d: panjang() tidak bisa untuk tipe %s",
                    line, args[0]->type == VAL_GALAT ? "galat" : "ini");
  return NULL;
}

static Val *bawaan_jumlah(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_ARRAY, "jumlah() butuh 1 larik");
  double s = 0;
  for (int i = 0; i < args[0]->arr_n; i++)
    s += args[0]->arr[i]->num;
  return val_angka(s);
}

static Val *bawaan_maks(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_ARRAY && args[0]->arr_n > 0,
              "maks() butuh larik isi");
  double m = args[0]->arr[0]->num;
  for (int i = 1; i < args[0]->arr_n; i++)
    if (args[0]->arr[i]->num > m)
      m = args[0]->arr[i]->num;
  return val_angka(m);
}

static Val *bawaan_min(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_ARRAY && args[0]->arr_n > 0,
              "min() butuh larik isi");
  double m = args[0]->arr[0]->num;
  for (int i = 1; i < args[0]->arr_n; i++)
    if (args[0]->arr[i]->num < m)
      m = args[0]->arr[i]->num;
  return val_angka(m);
}

static Val *bawaan_rerata(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_ARRAY && args[0]->arr_n > 0,
              "rerata() butuh larik isi");
  double s = 0;
  for (int i = 0; i < args[0]->arr_n; i++)
    s += args[0]->arr[i]->num;
  return val_angka(s / args[0]->arr_n);
}

static int banding_val(const void *a, const void *b) {
  Val *va = *(Val **)a, *vb = *(Val **)b;
  if (va->num < vb->num)
    return -1;
  if (va->num > vb->num)
    return 1;
  return 0;
}

static Val *bawaan_urut(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_ARRAY, "urut() butuh larik");
  Val **copy = malloc(sizeof(Val *) * args[0]->arr_n);
  for (int i = 0; i < args[0]->arr_n; i++) {
    copy[i] = args[0]->arr[i];
    copy[i]->refs++;
  }
  qsort(copy, args[0]->arr_n, sizeof(Val *), banding_val);
  return val_larik(copy, args[0]->arr_n);
}

static Val *bawaan_balik(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_ARRAY, "balik() butuh larik");
  int sz = args[0]->arr_n;
  Val **copy = malloc(sizeof(Val *) * sz);
  for (int i = 0; i < sz; i++) {
    copy[i] = args[0]->arr[sz - 1 - i];
    copy[i]->refs++;
  }
  return val_larik(copy, sz);
}

static Val *bawaan_angkaTeks(Val **args, int n, int line) {
  ARG_ASSERT(n == 1, "angkaTeks() butuh 1 argumen");
  char buf[64];
  double v = args[0]->num;
  if (v == (int64_t)v)
    snprintf(buf, 64, "%lld", (long long)(int64_t)v);
  else
    snprintf(buf, 64, "%g", v);
  return val_teks(buf);
}

static Val *bawaan_teksAngka(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_STR, "teksAngka() butuh teks");
  return val_angka(atof(args[0]->str));
}

static Val *bawaan_masukan(Val **args, int n, int line) {
  if (n > 0 && args[0]->type == VAL_STR)
    printf("%s", args[0]->str);
  char buf[4096];
  if (!fgets(buf, 4096, stdin))
    buf[0] = 0;
  int len = strlen(buf);
  if (len > 0 && buf[len - 1] == '\n')
    buf[len - 1] = 0;
  return val_teks(buf);
}

static const char *nama_tipe_awal(const Val *v) {
  switch (v->type) {
  case VAL_NUM: return "angka";
  case VAL_STR: return "teks";
  case VAL_BOOL: return "bool";
  case VAL_ARRAY: return "larik";
  case VAL_FUNC: return "fungsi";
  case VAL_BITA: return "bita";
  case VAL_GALAT: return "galat";
  default: return "kosong";
  }
}

static Val *bawaan_tipe(Val **args, int n, int line) {
  ARG_ASSERT(n == 1, "tipe() butuh 1 argumen");
  switch (args[0]->type) {
  case VAL_NUM:
    return val_teks("angka");
  case VAL_STR:
    return val_teks("teks");
  case VAL_BOOL:
    return val_teks("bool");
  case VAL_ARRAY:
    return val_teks("larik");
  case VAL_FUNC:
    return val_teks("fungsi");
  case VAL_PRIMA:
    return val_teks("kunci_prima");
  case VAL_ENKRIPSI:
    return val_teks("terenkripsi");
  case VAL_GANDA:
    return val_teks("ganda");
  case VAL_KONEKSI:
    return val_teks("koneksi");
  case VAL_BITA:
    return val_teks("bita");
  case VAL_GALAT:
    return val_teks("galat");
  default:
    return val_teks("kosong");
  }
}

static Val *bawaan_tambah(Val **args, int n, int line) {
  ARG_ASSERT(n == 2, "tambah() butuh 2 argumen");
  Val *a = args[0], *b = args[1];
  if (a->type == VAL_STR && b->type == VAL_STR)
    return gabung_dua(a, b);
  if (a->type == VAL_NUM && b->type == VAL_NUM)
    return val_angka(a->num + b->num);
  if (a->type == VAL_ARRAY) {
    int sz = a->arr_n + 1;
    Val **arr = malloc(sizeof(Val *) * sz);
    for (int i = 0; i < a->arr_n; i++) {
      arr[i] = a->arr[i];
      arr[i]->refs++;
    }
    arr[a->arr_n] = val_referensi(b);
    return val_larik(arr, sz);
  }
  if (a->type == VAL_PRIMA && b->type == VAL_PRIMA) {
    Val *r = calloc(1, sizeof(Val));
    r->type = VAL_PRIMA;
    r->refs = 1;
    r->prima_kunci = a->prima_kunci ^ b->prima_kunci;
    return r;
  }
  return gabung_dua(a, b);
}

static Val *bawaan_kodeKarakter(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_STR, "kodeKarakter() butuh teks");
  unsigned char c =
      (args[0]->str && args[0]->str[0]) ? (unsigned char)args[0]->str[0] : 0;
  return val_angka((double)c);
}

static Val *bawaan_karakter(Val **args, int n, int line) {
  ARG_ASSERT(n == 2 && args[0]->type == VAL_STR && args[1]->type == VAL_NUM,
              "karakter() butuh (teks, indeks)");
  const char *s = args[0]->str ? args[0]->str : "";
  int idx = (int)args[1]->num;
  int len = (int)strlen(s);
  if (idx < 0 || idx >= len)
    return val_teks("");
  char buf[2] = {s[idx], 0};
  return val_teks(buf);
}

static Val *bawaan_bulatkan(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_NUM, "bulatkan() butuh angka");
  double v = args[0]->num;
  return val_angka(v >= 0 ? floor(v + 0.5) : ceil(v - 0.5));
}

static Val *bawaan_irisLarik(Val **args, int n, int line) {
  ARG_ASSERT(n == 3 && args[0]->type == VAL_ARRAY &&
                  args[1]->type == VAL_NUM && args[2]->type == VAL_NUM,
              "irisLarik() butuh (larik, awal, akhir)");
  Val *arr = args[0];
  int awal = (int)args[1]->num, akhir = (int)args[2]->num;
  if (awal < 0)
    awal = 0;
  if (akhir >= arr->arr_n)
    akhir = arr->arr_n - 1;
  if (akhir < awal - 1) {
    return val_larik(NULL, 0);
  }
  int sz = akhir - awal + 1;
  if (sz <= 0)
    return val_larik(NULL, 0);
  Val **out = malloc(sizeof(Val *) * (size_t)sz);
  for (int i = 0; i < sz; i++)
    out[i] = val_referensi(arr->arr[awal + i]);
  return val_larik(out, sz);
}

static Val *bawaan_bacaFileTeks(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_STR,
             "bacaFileTeks() butuh path teks");
  size_t len;
  unsigned char *buf = baca_biner(args[0]->str, &len);
  if (memchr(buf, 0, len) != NULL) {
    free(buf);
    loop_lempar_jenis("GalatNilai",
                      "bacaFileTeks(): '%s' berisi byte 0x00, bukan teks; gunakan bacaBita()",
                      args[0]->str);
  }
  Val *r = val_teks_n((const char *)buf, len);
  free(buf);
  return r;
}

static Val *bawaan_tulisFileTeks(Val **args, int n, int line) {
  ARG_ASSERT(n == 2 && args[0]->type == VAL_STR && args[1]->type == VAL_STR,
             "tulisFileTeks() butuh (path, isi)");
  tulis_biner(args[0]->str, (const unsigned char *)args[1]->str,
              strlen(args[1]->str), "wb");
  return val_boolean(1);
}

static Val *bawaan_eksekusi(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_STR,
              "jalankan() butuh teks perintah");
  int rc = system(args[0]->str);
  if (rc == -1)
    return val_angka(-1);
  if (WIFEXITED(rc))
    return val_angka(WEXITSTATUS(rc));
  return val_angka(-1);
}

static Val *bawaan_syscall(Val **args, int n, int line) {
  // syscall(nomor, a1, a2, a3, a4, a5, a6) -- panggilan kernel MENTAH.
  // Nomor & argumen angka dipakai langsung (sesuai ABI syscall x86-64 Linux).
  // Argumen teks dikirim sebagai alamat buffer mentahnya (char*).
  // AMAN dipakai untuk syscall yang cuma MEMBACA buffer teks (mis. write()).
  // TIDAK AMAN untuk syscall yang MENULIS ke buffer teks (mis. read()) --
  // panjang & pencatatan internal string Loop bisa jadi rusak/tidak sinkron.
  ARG_ASSERT(n >= 1 && n <= 7 && args[0]->type == VAL_NUM,
              "Baris %d: syscall() butuh (nomor, a1..a6) -- nomor wajib "
              "angka",
              line);
  long a[6] = {0, 0, 0, 0, 0, 0};
  for (int i = 1; i < n; i++) {
    if (args[i]->type == VAL_STR) {
      a[i - 1] = (long)(intptr_t)args[i]->str;
    } else if (args[i]->type == VAL_NUM) {
      a[i - 1] = (long)args[i]->num;
    } else {
      ARG_ASSERT(0, "Baris %d: syscall() argumen ke-%d harus angka atau teks",
                  line, i + 1);
    }
  }
  long nomor = (long)args[0]->num;
  long hasil = syscall(nomor, a[0], a[1], a[2], a[3], a[4], a[5]);
  return val_angka((double)hasil);
}

static Val *bawaan_gabungTeks(Val **args, int n, int line) {
  ARG_ASSERT(n == 2 && args[0]->type == VAL_ARRAY && args[1]->type == VAL_STR,
              "gabungTeks() butuh (larik, pemisah)");
  Val *arr = args[0];
  const char *sep = args[1]->str;
  size_t seplen = strlen(sep);
  size_t cap = 4096, len = 0;
  char *out = malloc(cap);
  out[0] = 0;
  for (int i = 0; i < arr->arr_n; i++) {
    char *tmp = NULL;
    const char *piece;
    if (arr->arr[i]->type == VAL_STR) {
      piece = arr->arr[i]->str ? arr->arr[i]->str : "";
    } else {
      tmp = val_ke_teks(arr->arr[i]);
      piece = tmp;
    }
    size_t plen = strlen(piece);
    size_t need = len + plen + (i < arr->arr_n - 1 ? seplen : 0) + 1;
    if (need > g_maks_ukuran + 1) {
      free(tmp);
      free(out);
      loop_lempar_jenis("GalatBatas", "gabungTeks(): hasil melewati batas %zu byte", g_maks_ukuran);
    }
    if (need > cap) {
      while (cap < need)
        cap *= 2;
      out = realloc(out, cap);
    }
    memcpy(out + len, piece, plen);
    free(tmp);
    len += plen;
    if (i < arr->arr_n - 1) {
      memcpy(out + len, sep, seplen);
      len += seplen;
    }
    out[len] = 0;
  }
  Val *r = val_teks(out);
  free(out);
  return r;
}

static Val *bawaan_cetakTanpaBaris(Val **args, int n, int line) {
  ARG_ASSERT(n == 1, "cetakTanpaBaris() butuh 1 argumen");
  if (args[0]->type == VAL_STR) {
    printf("%s", args[0]->str ? args[0]->str : "");
  } else {
    char *buf = val_ke_teks(args[0]);
    fputs(buf, stdout);
    free(buf);
  }
  return val_kosong();
}

static Val *bawaan_gabung(Val **args, int n, int line) {
  ARG_ASSERT(n >= 1, "gabung() butuh minimal 1 argumen");
  Val **arr = malloc(sizeof(Val *) * n);
  for (int i = 0; i < n; i++)
    arr[i] = val_referensi(args[i]);
  Val *v = val_larik(arr, n);
  v->type = VAL_ARRAY;
  return v;
}

static Val *bawaan_prima(Val **args, int n, int line) {
  ARG_ASSERT(n >= 1, "prima() butuh minimal 1 argumen");
  uint64_t sebelumnya = 0;
  uint64_t prod = 1;
  uint64_t mod_prima = 1;
  for (int i = 0; i < n; i++) {
    uint64_t ni = (uint64_t)fabs(args[i]->num);
    if (ni < 2)
      ni = 2;
    uint64_t L = hitung_lapisan(ni, sebelumnya, i == 0);
    sebelumnya = L;
    prod *= L;
    mod_prima *= prima_berikutnya(ni);
  }
  uint64_t kunci = mod_prima > 0 ? prod % mod_prima : prod;
  if (kunci == 0)
    kunci = prod + n;
  Val *v = calloc(1, sizeof(Val));
  v->type = VAL_PRIMA;
  v->refs = 1;
  v->prima_kunci = kunci;
  return v;
}

static Val *bawaan_acak(Val **args, int n, int line) {
  ARG_ASSERT(n >= 1, "acak() butuh minimal 1 argumen");
  char *tmp = val_ke_teks(args[0]);
  unsigned char *data = (unsigned char *)tmp;
  int dlen = (int)strlen(tmp);
  uint64_t seed;
  unsigned char kbuf[8];
  int klen;
  if (n >= 2 && args[1]->type == VAL_PRIMA) {
    seed = args[1]->prima_kunci;
    for (int i = 0; i < 8; i++)
      kbuf[7 - i] = (seed >> (i * 8)) & 0xFF;
    klen = 8;
  } else if (n >= 2) {
    char kb[256];
    val_format(args[1], kb, 256);
    klen = strlen(kb);
    if (klen > 8)
      klen = 8;
    memcpy(kbuf, kb, klen);
    seed = 0;
    for (int i = 0; i < klen; i++)
      seed = seed * 256 + kbuf[i];
  } else {
    seed = (uint64_t)time(NULL);
    for (int i = 0; i < 8; i++)
      kbuf[7 - i] = (seed >> (i * 8)) & 0xFF;
    klen = 8;
  }
  unsigned char *out = malloc(dlen > 0 ? dlen : 1);
  memcpy(out, data, dlen);
  free(tmp);
  acak_bytes(out, dlen, seed);
  xor_bytes(out, out, dlen, kbuf, klen);
  if (n >= 3 && args[2]->type == VAL_NUM) {
    uint64_t pv = (uint64_t)args[2]->num;
    LOOP_ASSERT(adalah_prima(pv),
                "Baris %d: acak() arg ke-3 harus prima, dapat %llu", line,
                (unsigned long long)pv);
    unsigned char pb[8];
    int plen = 0;
    uint64_t tmp2 = pv;
    while (tmp2 > 0) {
      pb[plen++] = tmp2 & 0xFF;
      tmp2 >>= 8;
    }
    xor_bytes(out, out, dlen, pb, plen);
  }
  char *hex = bytes_keHex(out, dlen);
  Val *v = calloc(1, sizeof(Val));
  v->type = VAL_ENKRIPSI;
  v->refs = 1;
  v->enc_data = out;
  v->enc_len = dlen;
  v->enc_hex = hex;
  v->enc_seed = seed;
  return v;
}

static Val *bawaan_ganda(Val **args, int n, int line) {
  ARG_ASSERT(n == 1, "ganda() butuh 1 argumen");
  double asli = args[0]->num;
  Val *karg[1] = {val_angka(fabs(asli) < 2 ? 2 : fabs(asli))};
  Val *kunci = bawaan_prima(karg, 1, line);
  val_hapus(karg[0]);
  char src[64];
  snprintf(src, 64, "%g", asli);
  unsigned char kbuf[8];
  uint64_t k = kunci->prima_kunci;
  for (int i = 0; i < 8; i++)
    kbuf[7 - i] = (k >> (i * 8)) & 0xFF;
  unsigned char out[64];
  int slen = strlen(src);
  xor_bytes(out, (unsigned char *)src, slen, kbuf, 8);
  val_hapus(kunci);
  char *hex = bytes_keHex(out, slen);
  Val *v = calloc(1, sizeof(Val));
  v->type = VAL_GANDA;
  v->refs = 1;
  v->ganda_asli = asli;
  v->ganda_enc = hex;
  return v;
}

/* ===================== Jaringan: galat berjenis + timeout ===================== */
static int g_timeout_jaringan = 10; /* detik; env LOOP_TIMEOUT_JARINGAN */

static int net_timeout_arg(Val **args, int n, int idx, const char *fn) {
  if (n <= idx)
    return g_timeout_jaringan;
  if (args[idx]->type != VAL_NUM)
    loop_lempar_jenis("GalatTipe", "%s: timeout harus angka (detik)", fn);
  double t = args[idx]->num;
  if (!(t > 0) || t > 86400)
    loop_lempar_jenis("GalatNilai", "%s: timeout harus antara 0 dan 86400 detik", fn);
  return (int)(t + 0.999);
}

static void net_periksa_port(int port_val, double raw, const char *fn) {
  (void)port_val;
  if (raw != floor(raw) || raw < 1 || raw > 65535)
    loop_lempar_jenis("GalatNilai", "%s: port harus bilangan bulat 1 sampai 65535", fn);
}

/* Kembalikan fd, atau -1 dengan alasan di `alasan`. */
static int net_tcpSambung(const char *host, int port, int timeout_detik,
                          char *alasan, size_t alasan_n) {
  char portstr[16];
  snprintf(portstr, sizeof(portstr), "%d", port);
  struct addrinfo hints, *res, *rp;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  int gai = getaddrinfo(host, portstr, &hints, &res);
  if (gai != 0) {
    snprintf(alasan, alasan_n, "host tidak ditemukan: %s", gai_strerror(gai));
    return -1;
  }
  int fd = -1;
  snprintf(alasan, alasan_n, "tidak ada alamat yang bisa dihubungi");
  for (rp = res; rp != NULL; rp = rp->ai_next) {
    fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (fd < 0) {
      snprintf(alasan, alasan_n, "socket: %s", strerror(errno));
      continue;
    }
    int fl = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, fl | O_NONBLOCK);
    int rc = connect(fd, rp->ai_addr, rp->ai_addrlen);
    if (rc != 0 && errno == EINPROGRESS) {
      struct pollfd p = {fd, POLLOUT, 0};
      int pr;
      do {
        pr = poll(&p, 1, timeout_detik * 1000);
      } while (pr < 0 && errno == EINTR);
      if (pr == 0) {
        snprintf(alasan, alasan_n, "timeout setelah %d detik", timeout_detik);
        close(fd);
        fd = -1;
        continue;
      }
      int soerr = 0;
      socklen_t sl = sizeof(soerr);
      getsockopt(fd, SOL_SOCKET, SO_ERROR, &soerr, &sl);
      if (pr < 0 || soerr != 0) {
        snprintf(alasan, alasan_n, "%s", strerror(pr < 0 ? errno : soerr));
        close(fd);
        fd = -1;
        continue;
      }
      rc = 0;
    }
    if (rc != 0) {
      snprintf(alasan, alasan_n, "%s", strerror(errno));
      close(fd);
      fd = -1;
      continue;
    }
    fcntl(fd, F_SETFL, fl);
    struct timeval tv = {timeout_detik, 0};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    break;
  }
  freeaddrinfo(res);
  return fd;
}

/* TLS dengan verifikasi sertifikat dan nama host (kecuali
   LOOP_TLS_TIDAK_AMAN=1). Mengembalikan SSL* atau NULL + alasan. */
static SSL *net_tls_mulai(int fd, const char *host, SSL_CTX **ctx_out,
                          char *alasan, size_t alasan_n) {
  SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
  if (!ctx) {
    snprintf(alasan, alasan_n, "gagal membuat konteks TLS");
    return NULL;
  }
  const char *tak_aman = getenv("LOOP_TLS_TIDAK_AMAN");
  int verifikasi = !(tak_aman && strcmp(tak_aman, "1") == 0);
  if (verifikasi) {
    SSL_CTX_set_default_verify_paths(ctx);
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
  } else {
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
  }
  SSL *ssl = SSL_new(ctx);
  if (!ssl) {
    SSL_CTX_free(ctx);
    snprintf(alasan, alasan_n, "gagal membuat sesi TLS");
    return NULL;
  }
  SSL_set_fd(ssl, fd);
  unsigned char ipbuf[16];
  if (inet_pton(AF_INET, host, ipbuf) == 1 || inet_pton(AF_INET6, host, ipbuf) == 1) {
    if (verifikasi)
      X509_VERIFY_PARAM_set1_ip_asc(SSL_get0_param(ssl), host);
  } else {
    SSL_set_tlsext_host_name(ssl, host);
    if (verifikasi)
      SSL_set1_host(ssl, host);
  }
  if (SSL_connect(ssl) != 1) {
    long vr = SSL_get_verify_result(ssl);
    if (vr != X509_V_OK)
      snprintf(alasan, alasan_n, "handshake TLS gagal, sertifikat tidak valid: %s",
               X509_verify_cert_error_string(vr));
    else
      snprintf(alasan, alasan_n, "handshake TLS gagal");
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    return NULL;
  }
  *ctx_out = ctx;
  return ssl;
}

static void net_parse_url(const char *url, char *skema, char *host, int *port,
                          char **path_out) {
  skema[0] = 0;
  host[0] = 0;
  *port = 0;
  const char *p = strstr(url, "://");
  const char *rest;
  if (p) {
    size_t slen = (size_t)(p - url);
    if (slen >= 16)
      loop_lempar_jenis("GalatNilai", "URL: skema terlalu panjang");
    memcpy(skema, url, slen);
    skema[slen] = 0;
    rest = p + 3;
  } else {
    strcpy(skema, "http");
    rest = url;
  }
  if (strcmp(skema, "http") != 0 && strcmp(skema, "https") != 0)
    loop_lempar_jenis("GalatNilai", "URL: skema '%s' tidak didukung (hanya http/https)", skema);
  const char *slash = strchr(rest, '/');
  const char *hostend = slash ? slash : rest + strlen(rest);
  size_t hlen = (size_t)(hostend - rest);
  char hostport[512];
  if (hlen == 0)
    loop_lempar_jenis("GalatNilai", "URL: host kosong");
  if (hlen >= sizeof(hostport) - 1)
    loop_lempar_jenis("GalatNilai", "URL: host terlalu panjang");
  memcpy(hostport, rest, hlen);
  hostport[hlen] = 0;
  char *colon = strchr(hostport, ':');
  if (colon) {
    *colon = 0;
    char *endp = NULL;
    long pv = strtol(colon + 1, &endp, 10);
    if (!*(colon + 1) || *endp || pv < 1 || pv > 65535)
      loop_lempar_jenis("GalatNilai", "URL: port tidak valid");
    *port = (int)pv;
  }
  if (strlen(hostport) > 255)
    loop_lempar_jenis("GalatNilai", "URL: host terlalu panjang");
  strcpy(host, hostport);
  if (*port == 0)
    *port = (strcmp(skema, "https") == 0) ? 443 : 80;
  *path_out = strdup(slash ? slash : "/");
}

static Val *net_make_koneksi(int fd, int tls, void *ssl, void *ctx,
                             const char *host, int port) {
  Val *v = calloc(1, sizeof(Val));
  v->type = VAL_KONEKSI;
  v->refs = 1;
  v->kon_fd = fd;
  v->kon_tls = tls;
  v->kon_ssl = ssl;
  v->kon_ctx = ctx;
  v->kon_host = strdup(host ? host : "");
  v->kon_port = port;
  return v;
}

static Val *net_sambung_umum(Val **args, int n, int tls, const char *fn) {
  ARG_ASSERT((n == 2 || n == 3) && args[0]->type == VAL_STR && args[1]->type == VAL_NUM,
             "%s() butuh (host, port, [timeoutDetik])", fn);
  net_periksa_port(0, args[1]->num, fn);
  int port = (int)args[1]->num;
  int to = net_timeout_arg(args, n, 2, fn);
  char alasan[256];
  int fd = net_tcpSambung(args[0]->str, port, to, alasan, sizeof(alasan));
  if (fd < 0)
    loop_lempar_jenis("GalatJaringan", "gagal konek ke %s:%d (%s)", args[0]->str, port, alasan);
  if (!tls)
    return net_make_koneksi(fd, 0, NULL, NULL, args[0]->str, port);
  SSL_CTX *ctx = NULL;
  SSL *ssl = net_tls_mulai(fd, args[0]->str, &ctx, alasan, sizeof(alasan));
  if (!ssl) {
    close(fd);
    loop_lempar_jenis("GalatJaringan", "TLS ke %s:%d gagal (%s)", args[0]->str, port, alasan);
  }
  return net_make_koneksi(fd, 1, ssl, ctx, args[0]->str, port);
}

static Val *bawaan_sambung(Val **args, int n, int line) {
  return net_sambung_umum(args, n, 0, "sambung");
}
static Val *bawaan_tlsSambung(Val **args, int n, int line) {
  return net_sambung_umum(args, n, 1, "tlsSambung");
}

static void net_kirim_semua(Val *kon, const unsigned char *d, size_t len) {
  size_t off = 0;
  while (off < len) {
    ssize_t w;
    if (kon->kon_tls) {
      int k = SSL_write((SSL *)kon->kon_ssl, d + off, (int)(len - off > 1 << 20 ? 1 << 20 : len - off));
      w = k > 0 ? k : -1;
    } else {
      w = send(kon->kon_fd, d + off, len - off, MSG_NOSIGNAL);
    }
    if (w < 0) {
      if (!kon->kon_tls && errno == EINTR)
        continue;
      if (!kon->kon_tls && (errno == EAGAIN || errno == EWOULDBLOCK))
        loop_lempar_jenis("GalatJaringan", "timeout saat mengirim ke %s:%d", kon->kon_host, kon->kon_port);
      loop_lempar_jenis("GalatJaringan", "gagal mengirim ke %s:%d (%s)", kon->kon_host, kon->kon_port,
                        kon->kon_tls ? "kesalahan TLS" : strerror(errno));
    }
    off += (size_t)w;
  }
}

static Val *bawaan_kirim(Val **args, int n, int line) {
  ARG_ASSERT(n == 2 && args[0]->type == VAL_KONEKSI, "kirim() butuh (koneksi, data)");
  Val *kon = args[0];
  if (kon->kon_fd < 0)
    loop_lempar_jenis("GalatJaringan", "kirim(): koneksi sudah tertutup");
  const unsigned char *data;
  size_t len;
  char *tmp = NULL;
  if (args[1]->type == VAL_BITA) {
    data = args[1]->bita;
    len = args[1]->bita_n;
  } else if (args[1]->type == VAL_STR) {
    data = (const unsigned char *)args[1]->str;
    len = strlen(args[1]->str);
  } else {
    tmp = val_ke_teks(args[1]);
    data = (const unsigned char *)tmp;
    len = strlen(tmp);
  }
  net_kirim_semua(kon, data, len);
  free(tmp);
  return val_angka((double)len);
}

/* Baca sekali dari koneksi. EOF -> *got = 0; galat/timeout -> GalatJaringan. */
static int net_baca(Val *kon, unsigned char *buf, int maks) {
  if (kon->kon_tls) {
    int k = SSL_read((SSL *)kon->kon_ssl, buf, maks);
    if (k > 0)
      return k;
    int e = SSL_get_error((SSL *)kon->kon_ssl, k);
    if (e == SSL_ERROR_ZERO_RETURN)
      return 0;
    if (e == SSL_ERROR_SYSCALL && k == 0)
      return 0; /* peer menutup tanpa close_notify */
    if (e == SSL_ERROR_WANT_READ || (e == SSL_ERROR_SYSCALL && (errno == EAGAIN || errno == EWOULDBLOCK)))
      loop_lempar_jenis("GalatJaringan", "timeout menunggu data dari %s:%d", kon->kon_host, kon->kon_port);
    loop_lempar_jenis("GalatJaringan", "gagal menerima dari %s:%d (kesalahan TLS)", kon->kon_host, kon->kon_port);
  }
  for (;;) {
    ssize_t k = recv(kon->kon_fd, buf, (size_t)maks, 0);
    if (k >= 0)
      return (int)k;
    if (errno == EINTR)
      continue;
    if (errno == EAGAIN || errno == EWOULDBLOCK)
      loop_lempar_jenis("GalatJaringan", "timeout menunggu data dari %s:%d", kon->kon_host, kon->kon_port);
    loop_lempar_jenis("GalatJaringan", "gagal menerima dari %s:%d (%s)", kon->kon_host, kon->kon_port, strerror(errno));
  }
}

static Val *net_terima_umum(Val **args, int n, int sebagai_bita, const char *fn) {
  ARG_ASSERT(n >= 1 && args[0]->type == VAL_KONEKSI, "%s() butuh (koneksi, [maks])", fn);
  Val *kon = args[0];
  if (kon->kon_fd < 0)
    loop_lempar_jenis("GalatJaringan", "%s(): koneksi sudah tertutup", fn);
  int maks = (n >= 2 && args[1]->type == VAL_NUM) ? (int)args[1]->num : 4096;
  if (maks <= 0)
    maks = 4096;
  if (maks > 65536)
    maks = 65536; /* satu panggilan membaca paling banyak 64 KiB */
  unsigned char *buf = malloc((size_t)maks + 1);
  int got = net_baca(kon, buf, maks);
  buf[got] = 0;
  Val *r;
  if (sebagai_bita) {
    r = val_bita(buf, (size_t)got);
  } else {
    if (memchr(buf, 0, (size_t)got)) {
      free(buf);
      loop_lempar_jenis("GalatNilai", "terima(): data berisi byte 0x00, gunakan terimaBita()");
    }
    r = val_teks_n((const char *)buf, (size_t)got);
  }
  free(buf);
  return r;
}

static Val *bawaan_terima(Val **args, int n, int line) {
  return net_terima_umum(args, n, 0, "terima");
}
static Val *bawaan_terimaBita(Val **args, int n, int line) {
  return net_terima_umum(args, n, 1, "terimaBita");
}

static Val *bawaan_tutupKoneksi(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_KONEKSI, "tutupKoneksi() butuh (koneksi)");
  Val *kon = args[0];
  if (kon->kon_ssl) {
    SSL_shutdown((SSL *)kon->kon_ssl);
    SSL_free((SSL *)kon->kon_ssl);
    kon->kon_ssl = NULL;
  }
  if (kon->kon_ctx) {
    SSL_CTX_free((SSL_CTX *)kon->kon_ctx);
    kon->kon_ctx = NULL;
  }
  if (kon->kon_fd >= 0) {
    close(kon->kon_fd);
    kon->kon_fd = -1;
  }
  return val_kosong();
}

/* HTTP GET sederhana. Mengembalikan body (malloc, NUL-terminated) dan panjangnya. */
static unsigned char *net_httpAmbil(const char *url, int timeout, char *out_host, size_t *out_len) {
  char skema[16], host[256];
  char *path = NULL;
  int port;
  net_parse_url(url, skema, host, &port, &path);
  int tls = (strcmp(skema, "https") == 0);
  if (out_host)
    snprintf(out_host, 256, "%s", host);
  char alasan[256];
  int fd = net_tcpSambung(host, port, timeout, alasan, sizeof(alasan));
  if (fd < 0) {
    free(path);
    loop_lempar_jenis("GalatJaringan", "gagal konek ke %s:%d (%s)", host, port, alasan);
  }
  SSL_CTX *ctx = NULL;
  SSL *ssl = NULL;
  if (tls) {
    ssl = net_tls_mulai(fd, host, &ctx, alasan, sizeof(alasan));
    if (!ssl) {
      free(path);
      close(fd);
      loop_lempar_jenis("GalatJaringan", "TLS ke %s:%d gagal (%s)", host, port, alasan);
    }
  }
  Val kon;
  memset(&kon, 0, sizeof(kon));
  kon.type = VAL_KONEKSI;
  kon.kon_fd = fd;
  kon.kon_tls = tls;
  kon.kon_ssl = ssl;
  kon.kon_host = host;
  kon.kon_port = port;
  Sb req;
  sb_awal(&req);
  sb_fmt(&req, "GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: Loop/%s\r\nConnection: close\r\n\r\n",
         path, host, LOOP_VERSI);
  free(path);
  /* galat jaringan di bawah ini dilempar; fd/ssl dibersihkan lewat handler
     sederhana: kita tutup dulu sebelum melempar */
  Handler h;
  h.prev = g_handler;
  int frames_simpan = g_frames_n;
  const char *berkas_simpan = g_berkas_aktif;
  g_handler = &h;
  size_t cap = 65536, len = 0;
  unsigned char *out = malloc(cap + 1);
  if (setjmp(h.jb) != 0) {
    g_handler = h.prev;
    g_frames_n = frames_simpan;
    g_berkas_aktif = berkas_simpan;
    if (ssl) { SSL_free(ssl); }
    if (ctx) SSL_CTX_free(ctx);
    close(fd);
    free(out);
    free(req.p);
    Val *g = g_galat_aktif;
    g_galat_aktif = NULL;
    loop_lempar(g);
  }
  net_kirim_semua(&kon, (const unsigned char *)req.p, req.n);
  free(req.p);
  req.p = NULL;
  for (;;) {
    if (len + 4097 > cap) {
      cap *= 2;
      out = realloc(out, cap + 1);
    }
    int got = net_baca(&kon, out + len, 4096);
    if (got <= 0)
      break;
    len += (size_t)got;
    if (len > g_maks_ukuran)
      loop_lempar_jenis("GalatBatas", "respons HTTP melewati batas %zu byte", g_maks_ukuran);
  }
  g_handler = h.prev;
  out[len] = 0;
  if (ssl) {
    SSL_shutdown(ssl);
    SSL_free(ssl);
  }
  if (ctx)
    SSL_CTX_free(ctx);
  close(fd);
  size_t off = 0;
  for (size_t i = 0; i + 3 < len; i++)
    if (out[i] == '\r' && out[i + 1] == '\n' && out[i + 2] == '\r' && out[i + 3] == '\n') {
      off = i + 4;
      break;
    }
  if (off) {
    memmove(out, out + off, len - off + 1);
    len -= off;
  }
  *out_len = len;
  return out;
}

static Val *bawaan_ambil(Val **args, int n, int line) {
  ARG_ASSERT((n == 1 || n == 2) && args[0]->type == VAL_STR, "ambil() butuh (url, [timeoutDetik])");
  int to = net_timeout_arg(args, n, 1, "ambil()");
  size_t len;
  unsigned char *body = net_httpAmbil(args[0]->str, to, NULL, &len);
  if (memchr(body, 0, len)) {
    free(body);
    loop_lempar_jenis("GalatNilai", "ambil(): respons berisi byte 0x00, gunakan ambilBita()");
  }
  return val_teks_ambil((char *)body);
}

static Val *bawaan_ambilBita(Val **args, int n, int line) {
  ARG_ASSERT((n == 1 || n == 2) && args[0]->type == VAL_STR, "ambilBita() butuh (url, [timeoutDetik])");
  int to = net_timeout_arg(args, n, 1, "ambilBita()");
  size_t len;
  unsigned char *body = net_httpAmbil(args[0]->str, to, NULL, &len);
  Val *r = val_bita(body, len);
  free(body);
  return r;
}

static Val *bawaan_telusuri(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 || n == 2, "telusuri() butuh (url) atau (url, kedalaman)");
  ARG_ASSERT(args[0]->type == VAL_STR, "telusuri(): argumen pertama harus teks (url)");
  if (n == 2)
    ARG_ASSERT(args[1]->type == VAL_NUM, "telusuri(): kedalaman harus angka");
  char host[256];
  size_t blen;
  unsigned char *body = net_httpAmbil(args[0]->str, g_timeout_jaringan, host, &blen);
  if (memchr(body, 0, blen)) {
    free(body);
    loop_lempar_jenis("GalatNilai", "telusuri(): respons berisi byte 0x00, bukan HTML teks");
  }
  int cap = 64, nlinks = 0;
  Val **links = malloc(sizeof(Val *) * (size_t)cap);
  const char *p = (const char *)body;
  while ((p = strstr(p, "href=")) != NULL) {
    p += 5;
    char quote = *p;
    if (quote != '"' && quote != '\'')
      continue;
    p++;
    const char *end = strchr(p, quote);
    if (!end)
      break;
    size_t llen = (size_t)(end - p);
    if (llen > 0) {
      if (nlinks >= cap) {
        cap *= 2;
        links = realloc(links, sizeof(Val *) * (size_t)cap);
      }
      links[nlinks++] = val_teks_n(p, llen);
    }
    p = end + 1;
  }
  free(body);
  if (nlinks == 0) {
    free(links);
    return val_larik(NULL, 0);
  }
  return val_larik(links, nlinks);
}

static Val *bawaan_tambahFileTeks(Val **args, int n, int line) {
  ARG_ASSERT(n == 2 && args[0]->type == VAL_STR && args[1]->type == VAL_STR,
             "tambahFileTeks() butuh (path, isi)");
  tulis_biner(args[0]->str, (const unsigned char *)args[1]->str,
              strlen(args[1]->str), "ab");
  return val_boolean(1);
}

static Val *bawaan_appendFile(Val **args, int n, int line) {
  ARG_ASSERT(n == 2 && args[0]->type == VAL_STR,
             "appendFile() butuh (path, isi)");
  char *t = val_ke_teks(args[1]);
  tulis_biner(args[0]->str, (const unsigned char *)t, strlen(t), "ab");
  free(t);
  return val_boolean(1);
}

static Val *bawaan_tulis(Val **args, int n, int line) {
  ARG_ASSERT(n == 1, "tulis() butuh 1 argumen");
  if (args[0]->type == VAL_STR)
    printf("%s", args[0]->str ? args[0]->str : "");
  else {
    char *buf = val_ke_teks(args[0]);
    fputs(buf, stdout);
    free(buf);
  }
  return val_kosong();
}

static Val *bawaan_tulisInline(Val **args, int n, int line) {
  ARG_ASSERT(n == 1, "tulisInline() butuh 1 argumen");
  if (args[0]->type == VAL_STR)
    printf("%s", args[0]->str ? args[0]->str : "");
  else {
    char *buf = val_ke_teks(args[0]);
    fputs(buf, stdout);
    free(buf);
  }
  fflush(stdout);
  return val_kosong();
}

static Val *bawaan_tulisStderr(Val **args, int n, int line) {
  ARG_ASSERT(n == 1, "tulisStderr() butuh 1 argumen");
  if (args[0]->type == VAL_STR)
    fprintf(stderr, "%s\n", args[0]->str ? args[0]->str : "");
  else {
    char *buf = val_ke_teks(args[0]);
    fprintf(stderr, "%s\n", buf);
    free(buf);
  }
  return val_kosong();
}

static Val *bawaan_tulisError(Val **args, int n, int line) {
  ARG_ASSERT(n == 1, "tulisError() butuh 1 argumen");
  if (args[0]->type == VAL_STR)
    fprintf(stderr, "[error] %s\n", args[0]->str ? args[0]->str : "");
  else {
    char *buf = val_ke_teks(args[0]);
    fprintf(stderr, "[error] %s\n", buf);
    free(buf);
  }
  return val_kosong();
}

static Val *bawaan_barisBaru(Val **args, int n, int line) {
  printf("\n");
  return val_kosong();
}

static Val *bawaan_waktumilisdetik(Val **args, int n, int line) {
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  double ms = (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
  return val_angka(ms);
}

static Val *bawaan_waktuSekarang(Val **args, int n, int line) {
  return val_angka((double)time(NULL));
}

static Val *bawaan_waktuISO(Val **args, int n, int line) {
  time_t t = time(NULL);
  struct tm tmv;
  gmtime_r(&t, &tmv);
  char buf[64];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tmv);
  return val_teks(buf);
}

static Val *bawaan_tidur(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_NUM,
              "tidur() butuh angka (milidetik)");
  long ms = (long)args[0]->num;
  if (ms > 0)
    usleep((useconds_t)(ms * 1000));
  return val_kosong();
}

static Val *bawaan_keAngka(Val **args, int n, int line) {
  ARG_ASSERT(n == 1, "keAngka() butuh 1 argumen");
  if (args[0]->type == VAL_NUM)
    return val_angka(args[0]->num);
  if (args[0]->type == VAL_BOOL)
    return val_angka(args[0]->bval ? 1 : 0);
  if (args[0]->type == VAL_STR)
    return val_angka(atof(args[0]->str ? args[0]->str : ""));
  loop_lempar_jenis("GalatTipe", "Baris %d: keAngka() tidak bisa untuk tipe %s", line, nama_tipe_awal(args[0]));
  return NULL;
}

static Val *bawaan_keDesimal(Val **args, int n, int line) {
  ARG_ASSERT(n == 1, "keDesimal() butuh 1 argumen");
  if (args[0]->type == VAL_NUM)
    return val_angka(args[0]->num);
  if (args[0]->type == VAL_STR)
    return val_angka(atof(args[0]->str ? args[0]->str : ""));
  loop_lempar_jenis("GalatTipe", "Baris %d: keDesimal() tidak bisa untuk tipe %s", line, nama_tipe_awal(args[0]));
  return NULL;
}

static Val *bawaan_keTeks(Val **args, int n, int line) {
  ARG_ASSERT(n == 1, "keTeks() butuh 1 argumen");
  if (args[0]->type == VAL_BITA) {
    Val *b = args[0];
    if (b->bita_n && memchr(b->bita, 0, b->bita_n))
      loop_lempar_jenis("GalatNilai", "keTeks(): bita berisi byte 0x00, bukan teks");
    if (!utf8_valid(b->bita, b->bita_n))
      loop_lempar_jenis("GalatNilai", "keTeks(): bita bukan UTF-8 yang valid");
    return val_teks_n((const char *)b->bita, b->bita_n);
  }
  return val_teks_ambil(val_ke_teks(args[0]));
}

static Val *bawaan_dariKode(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_NUM, "dariKode() butuh angka");
  char buf[2] = {(char)(int)args[0]->num, 0};
  return val_teks(buf);
}

static Val *bawaan_potong(Val **args, int n, int line) {
  ARG_ASSERT(n >= 2 && args[0]->type == VAL_STR && args[1]->type == VAL_NUM,
              "potong() butuh (teks, awal[, panjang])");
  const char *s = args[0]->str ? args[0]->str : "";
  int len = (int)strlen(s);
  int awal = (int)args[1]->num;
  if (awal < 0)
    awal = len + awal;
  if (awal < 0)
    awal = 0;
  int ambil =
      (n >= 3 && args[2]->type == VAL_NUM) ? (int)args[2]->num : (len - awal);
  if (awal >= len || ambil <= 0)
    return val_teks("");
  if (awal + ambil > len)
    ambil = len - awal;
  char *buf = malloc((size_t)ambil + 1);
  memcpy(buf, s + awal, (size_t)ambil);
  buf[ambil] = 0;
  Val *r = val_teks(buf);
  free(buf);
  return r;
}

static Val *bawaan_cariTeks(Val **args, int n, int line) {
  ARG_ASSERT(n == 2 && args[0]->type == VAL_STR && args[1]->type == VAL_STR,
              "cariTeks() butuh (teks, cari)");
  const char *hay = args[0]->str ? args[0]->str : "";
  const char *needle = args[1]->str ? args[1]->str : "";
  const char *p = strstr(hay, needle);
  return val_angka(p ? (double)(p - hay) : -1);
}

static Val *bawaan_gantiTeks(Val **args, int n, int line) {
  ARG_ASSERT(n == 3 && args[0]->type == VAL_STR && args[1]->type == VAL_STR &&
                  args[2]->type == VAL_STR,
              "gantiTeks() butuh (teks, cari, ganti)");
  const char *s = args[0]->str ? args[0]->str : "";
  const char *cari = args[1]->str ? args[1]->str : "";
  const char *ganti = args[2]->str ? args[2]->str : "";
  size_t cari_len = strlen(cari);
  if (cari_len == 0)
    return val_teks(s);
  size_t cap = strlen(s) + 1, len = 0;
  char *out = malloc(cap);
  out[0] = 0;
  const char *cur = s;
  while (*cur) {
    const char *p = strstr(cur, cari);
    if (!p) {
      size_t rest = strlen(cur);
      if (len + rest + 1 > cap) {
        cap = len + rest + 1;
        out = realloc(out, cap);
      }
      memcpy(out + len, cur, rest);
      len += rest;
      out[len] = 0;
      break;
    }
    size_t pre = (size_t)(p - cur);
    size_t ganti_len = strlen(ganti);
    if (len + pre + ganti_len + 1 > cap) {
      cap = (len + pre + ganti_len + 1) * 2;
      out = realloc(out, cap);
    }
    memcpy(out + len, cur, pre);
    len += pre;
    memcpy(out + len, ganti, ganti_len);
    len += ganti_len;
    out[len] = 0;
    cur = p + cari_len;
  }
  Val *r = val_teks(out);
  free(out);
  return r;
}

static Val *bawaan_hurufBesar(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_STR, "hurufBesar() butuh teks");
  char *s = strdup(args[0]->str ? args[0]->str : "");
  for (char *p = s; *p; p++)
    *p = (char)toupper((unsigned char)*p);
  Val *r = val_teks(s);
  free(s);
  return r;
}

static Val *bawaan_hurufKecil(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_STR, "hurufKecil() butuh teks");
  char *s = strdup(args[0]->str ? args[0]->str : "");
  for (char *p = s; *p; p++)
    *p = (char)tolower((unsigned char)*p);
  Val *r = val_teks(s);
  free(s);
  return r;
}

static Val *bawaan_pisah(Val **args, int n, int line) {
  ARG_ASSERT(n == 2 && args[0]->type == VAL_STR && args[1]->type == VAL_STR,
              "pisah() butuh (teks, pemisah)");
  const char *s = args[0]->str ? args[0]->str : "";
  const char *sep = args[1]->str ? args[1]->str : "";
  size_t sep_len = strlen(sep);
  int cap = 8, cnt = 0;
  Val **out = malloc(sizeof(Val *) * cap);
  if (sep_len == 0) {
    int len = (int)strlen(s);
    for (int i = 0; i < len; i++) {
      if (cnt >= cap) {
        cap *= 2;
        out = realloc(out, sizeof(Val *) * cap);
      }
      char buf[2] = {s[i], 0};
      out[cnt++] = val_teks(buf);
    }
    return val_larik(out, cnt);
  }
  const char *cur = s;
  for (;;) {
    const char *p = strstr(cur, sep);
    if (cnt >= cap) {
      cap *= 2;
      out = realloc(out, sizeof(Val *) * cap);
    }
    if (!p) {
      out[cnt++] = val_teks(cur);
      break;
    }
    size_t plen = (size_t)(p - cur);
    char *piece = malloc(plen + 1);
    memcpy(piece, cur, plen);
    piece[plen] = 0;
    out[cnt++] = val_teks(piece);
    free(piece);
    cur = p + sep_len;
  }
  return val_larik(out, cnt);
}

static Val *bawaan_trimTeks(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_STR, "trimTeks() butuh teks");
  const char *s = args[0]->str ? args[0]->str : "";
  int a = 0, b = (int)strlen(s) - 1;
  while (a <= b && isspace((unsigned char)s[a]))
    a++;
  while (b >= a && isspace((unsigned char)s[b]))
    b--;
  int len = b - a + 1;
  if (len < 0)
    len = 0;
  char *buf = malloc((size_t)len + 1);
  memcpy(buf, s + a, (size_t)len);
  buf[len] = 0;
  Val *r = val_teks(buf);
  free(buf);
  return r;
}

static Val *bawaan_mulaiDengan(Val **args, int n, int line) {
  ARG_ASSERT(n == 2 && args[0]->type == VAL_STR && args[1]->type == VAL_STR,
              "mulaiDengan() butuh (teks, awalan)");
  const char *s = args[0]->str ? args[0]->str : "";
  const char *p = args[1]->str ? args[1]->str : "";
  return val_boolean(strncmp(s, p, strlen(p)) == 0);
}

static Val *bawaan_akhiriDengan(Val **args, int n, int line) {
  ARG_ASSERT(n == 2 && args[0]->type == VAL_STR && args[1]->type == VAL_STR,
              "akhiriDengan() butuh (teks, akhiran)");
  const char *s = args[0]->str ? args[0]->str : "";
  const char *p = args[1]->str ? args[1]->str : "";
  size_t sl = strlen(s), pl = strlen(p);
  if (pl > sl)
    return val_boolean(0);
  return val_boolean(strcmp(s + (sl - pl), p) == 0);
}

static Val *bawaan_regexCocok(Val **args, int n, int line) {
  ARG_ASSERT(n == 2 && args[0]->type == VAL_STR && args[1]->type == VAL_STR,
              "regexCocok() butuh (teks, pola)");
  regex_t re;
  if (regcomp(&re, args[1]->str, REG_EXTENDED) != 0)
    loop_lempar_jenis("GalatNilai", "Baris %d: regexCocok() pola tidak valid", line);
  int rc = regexec(&re, args[0]->str ? args[0]->str : "", 0, NULL, 0);
  regfree(&re);
  return val_boolean(rc == 0);
}

static Val *bawaan_regexGanti(Val **args, int n, int line) {
  ARG_ASSERT(n == 3 && args[0]->type == VAL_STR && args[1]->type == VAL_STR &&
                  args[2]->type == VAL_STR,
              "regexGanti() butuh (teks, pola, ganti)");
  regex_t re;
  if (regcomp(&re, args[1]->str, REG_EXTENDED) != 0)
    loop_lempar_jenis("GalatNilai", "Baris %d: regexGanti() pola tidak valid", line);
  const char *s = args[0]->str ? args[0]->str : "";
  const char *ganti = args[2]->str ? args[2]->str : "";
  size_t cap = strlen(s) + 1, len = 0;
  char *out = malloc(cap);
  out[0] = 0;
  const char *cur = s;
  regmatch_t m;
  while (*cur && regexec(&re, cur, 1, &m, 0) == 0) {
    size_t pre = (size_t)m.rm_so;
    size_t glen = strlen(ganti);
    if (len + pre + glen + 1 > cap) {
      cap = (len + pre + glen + 1) * 2;
      out = realloc(out, cap);
    }
    memcpy(out + len, cur, pre);
    len += pre;
    memcpy(out + len, ganti, glen);
    len += glen;
    out[len] = 0;
    if (m.rm_eo == m.rm_so) {
      if (cur[m.rm_eo] == 0)
        break;
      if (len + 1 + 1 > cap) {
        cap *= 2;
        out = realloc(out, cap);
      }
      out[len++] = cur[m.rm_eo];
      out[len] = 0;
      cur += m.rm_eo + 1;
    } else {
      cur += m.rm_eo;
    }
  }
  size_t rest = strlen(cur);
  if (len + rest + 1 > cap) {
    cap = len + rest + 1;
    out = realloc(out, cap);
  }
  memcpy(out + len, cur, rest);
  len += rest;
  out[len] = 0;
  regfree(&re);
  Val *r = val_teks(out);
  free(out);
  return r;
}

static Val *bawaan_terakhirIndexDari(Val **args, int n, int line) {
  ARG_ASSERT(n == 2 && args[0]->type == VAL_STR && args[1]->type == VAL_STR,
              "terakhirIndexDari() butuh (teks, cari)");
  const char *hay = args[0]->str ? args[0]->str : "";
  const char *needle = args[1]->str ? args[1]->str : "";
  size_t nlen = strlen(needle);
  if (nlen == 0)
    return val_angka((double)strlen(hay));
  const char *last = NULL, *cur = hay;
  for (;;) {
    const char *p = strstr(cur, needle);
    if (!p)
      break;
    last = p;
    cur = p + nlen;
  }
  return val_angka(last ? (double)(last - hay) : -1);
}

static Val *bawaan_dorong(Val **args, int n, int line) {
  ARG_ASSERT(n == 2 && args[0]->type == VAL_ARRAY,
              "dorong() butuh (larik, nilai)");
  Val *a = args[0];
  int sz = a->arr_n + 1;
  Val **arr = malloc(sizeof(Val *) * (size_t)sz);
  for (int i = 0; i < a->arr_n; i++)
    arr[i] = val_referensi(a->arr[i]);
  arr[a->arr_n] = val_referensi(args[1]);
  return val_larik(arr, sz);
}

static Val *bawaan_hapusTerakhir(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_ARRAY,
              "hapusTerakhir() butuh larik");
  Val *a = args[0];
  int sz = a->arr_n > 0 ? a->arr_n - 1 : 0;
  Val **arr = sz > 0 ? malloc(sizeof(Val *) * (size_t)sz) : NULL;
  for (int i = 0; i < sz; i++)
    arr[i] = val_referensi(a->arr[i]);
  return val_larik(arr, sz);
}

static Val *bawaan_hapusKunci(Val **args, int n, int line) {
  ARG_ASSERT(n == 2 && args[0]->type == VAL_ARRAY && args[1]->type == VAL_NUM,
              "hapusKunci() butuh (larik, indeks)");
  Val *a = args[0];
  int idx = (int)args[1]->num;
  if (idx < 0 || idx >= a->arr_n) {
    Val **arr = a->arr_n > 0 ? malloc(sizeof(Val *) * a->arr_n) : NULL;
    for (int i = 0; i < a->arr_n; i++)
      arr[i] = val_referensi(a->arr[i]);
    return val_larik(arr, a->arr_n);
  }
  int sz = a->arr_n - 1;
  Val **arr = sz > 0 ? malloc(sizeof(Val *) * (size_t)sz) : NULL;
  int j = 0;
  for (int i = 0; i < a->arr_n; i++)
    if (i != idx)
      arr[j++] = val_referensi(a->arr[i]);
  return val_larik(arr, sz);
}

static Val *bawaan_hapusLarik(Val **args, int n, int line) {
  ARG_ASSERT(n == 2 && args[0]->type == VAL_ARRAY,
              "hapusLarik() butuh (larik, nilai)");
  Val *a = args[0], *target = args[1];
  int found = -1;
  for (int i = 0; i < a->arr_n; i++) {
    Val *e = a->arr[i];
    if (e->type != target->type)
      continue;
    int sama = 0;
    if (e->type == VAL_NUM)
      sama = (e->num == target->num);
    else if (e->type == VAL_STR)
      sama =
          (strcmp(e->str ? e->str : "", target->str ? target->str : "") == 0);
    else if (e->type == VAL_BOOL)
      sama = (e->bval == target->bval);
    if (sama) {
      found = i;
      break;
    }
  }
  if (found < 0) {
    Val **arr = a->arr_n > 0 ? malloc(sizeof(Val *) * a->arr_n) : NULL;
    for (int i = 0; i < a->arr_n; i++)
      arr[i] = val_referensi(a->arr[i]);
    return val_larik(arr, a->arr_n);
  }
  int sz = a->arr_n - 1;
  Val **arr = sz > 0 ? malloc(sizeof(Val *) * (size_t)sz) : NULL;
  int j = 0;
  for (int i = 0; i < a->arr_n; i++)
    if (i != found)
      arr[j++] = val_referensi(a->arr[i]);
  return val_larik(arr, sz);
}

static Val *bawaan_iris(Val **args, int n, int line) {
  ARG_ASSERT(n == 3 && args[1]->type == VAL_NUM && args[2]->type == VAL_NUM,
              "iris() butuh (larik|teks, awal, akhir)");
  int awal = (int)args[1]->num, akhir = (int)args[2]->num;
  if (args[0]->type == VAL_ARRAY) {
    Val *arr = args[0];
    if (awal < 0)
      awal = 0;
    if (akhir >= arr->arr_n)
      akhir = arr->arr_n - 1;
    int sz = akhir - awal + 1;
    if (sz <= 0)
      return val_larik(NULL, 0);
    Val **out = malloc(sizeof(Val *) * (size_t)sz);
    for (int i = 0; i < sz; i++)
      out[i] = val_referensi(arr->arr[awal + i]);
    return val_larik(out, sz);
  }
  if (args[0]->type == VAL_STR) {
    const char *s = args[0]->str ? args[0]->str : "";
    int len = (int)strlen(s);
    if (awal < 0)
      awal = 0;
    if (akhir >= len)
      akhir = len - 1;
    int sz = akhir - awal + 1;
    if (sz <= 0)
      return val_teks("");
    char *buf = malloc((size_t)sz + 1);
    memcpy(buf, s + awal, (size_t)sz);
    buf[sz] = 0;
    Val *r = val_teks(buf);
    free(buf);
    return r;
  }
  loop_lempar_jenis("GalatTipe", "Baris %d: iris() butuh larik atau teks", line);
  return NULL;
}

static Val *bawaan_adaDalam(Val **args, int n, int line) {
  ARG_ASSERT(n == 2 && args[0]->type == VAL_ARRAY,
              "adaDalam() butuh (larik, nilai)");
  Val *a = args[0], *target = args[1];
  for (int i = 0; i < a->arr_n; i++) {
    Val *e = a->arr[i];
    if (e->type != target->type)
      continue;
    if (e->type == VAL_NUM && e->num == target->num)
      return val_boolean(1);
    if (e->type == VAL_STR &&
        strcmp(e->str ? e->str : "", target->str ? target->str : "") == 0)
      return val_boolean(1);
    if (e->type == VAL_BOOL && e->bval == target->bval)
      return val_boolean(1);
  }
  return val_boolean(0);
}

static Val *bawaan_saring(Val **args, int n, int line) {
  ARG_ASSERT(n == 2 && args[0]->type == VAL_ARRAY && args[1]->type == VAL_FUNC,
              "saring() butuh (larik, fungsi)");
  Val *a = args[0];
  int cap = a->arr_n > 0 ? a->arr_n : 1, cnt = 0;
  Val **out = malloc(sizeof(Val *) * (size_t)cap);
  for (int i = 0; i < a->arr_n; i++) {
    Val *fargs[1] = {a->arr[i]};
    Val *r = panggil_nilai_fungsi(args[1], fargs, 1, line);
    int keep =
        (r->type == VAL_BOOL && r->bval) || (r->type == VAL_NUM && r->num != 0);
    val_hapus(r);
    if (keep)
      out[cnt++] = val_referensi(a->arr[i]);
  }
  return val_larik(out, cnt);
}

static Val *bawaan_peta(Val **args, int n, int line) {
  ARG_ASSERT(n == 2 && args[0]->type == VAL_ARRAY && args[1]->type == VAL_FUNC,
              "peta() butuh (larik, fungsi)");
  Val *a = args[0];
  Val **out = a->arr_n > 0 ? malloc(sizeof(Val *) * (size_t)a->arr_n) : NULL;
  for (int i = 0; i < a->arr_n; i++) {
    Val *fargs[1] = {a->arr[i]};
    out[i] = panggil_nilai_fungsi(args[1], fargs, 1, line);
  }
  return val_larik(out, a->arr_n);
}

static Val *bawaan_kurangi(Val **args, int n, int line) {
  ARG_ASSERT(n == 3 && args[0]->type == VAL_ARRAY && args[1]->type == VAL_FUNC,
              "kurangi() butuh (larik, fungsi, nilaiAwal)");
  Val *a = args[0];
  Val *acc = val_referensi(args[2]);
  for (int i = 0; i < a->arr_n; i++) {
    Val *fargs[2] = {acc, a->arr[i]};
    Val *r = panggil_nilai_fungsi(args[1], fargs, 2, line);
    val_hapus(acc);
    acc = r;
  }
  return acc;
}

static Val *bawaan_kunciObjek(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_ARRAY, "kunciObjek() butuh larik");
  Val *a = args[0];
  Val **out = a->arr_n > 0 ? malloc(sizeof(Val *) * (size_t)a->arr_n) : NULL;
  for (int i = 0; i < a->arr_n; i++)
    out[i] = val_angka(i);
  return val_larik(out, a->arr_n);
}

static Val *val_salinDalam(Val *v) {
  switch (v->type) {
  case VAL_STR:
    return val_teks(v->str ? v->str : "");
  case VAL_NUM:
    return val_angka(v->num);
  case VAL_BOOL:
    return val_boolean(v->bval);
  case VAL_ARRAY: {
    Val **out = v->arr_n > 0 ? malloc(sizeof(Val *) * (size_t)v->arr_n) : NULL;
    for (int i = 0; i < v->arr_n; i++)
      out[i] = val_salinDalam(v->arr[i]);
    return val_larik(out, v->arr_n);
  }
  default:
    return val_referensi(v);
  }
}

static Val *bawaan_salin(Val **args, int n, int line) {
  ARG_ASSERT(n == 1, "salin() butuh 1 argumen");
  return val_salinDalam(args[0]);
}

static Val *bawaan_lantai(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_NUM, "lantai() butuh angka");
  return val_angka(floor(args[0]->num));
}

static Val *bawaan_langit(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_NUM, "langit() butuh angka");
  return val_angka(ceil(args[0]->num));
}

static Val *bawaan_absolut(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_NUM, "absolut() butuh angka");
  return val_angka(fabs(args[0]->num));
}

static Val *bawaan_acakAngka(Val **args, int n, int line) {
  if (n >= 2 && args[0]->type == VAL_NUM && args[1]->type == VAL_NUM) {
    double lo = args[0]->num, hi = args[1]->num;
    double r = lo + ((double)rand() / ((double)RAND_MAX + 1.0)) * (hi - lo);
    return val_angka(r);
  }
  return val_angka((double)rand() / ((double)RAND_MAX + 1.0));
}

static Val *bawaan_log(Val **args, int n, int line) {
  ARG_ASSERT(n >= 1 && args[0]->type == VAL_NUM, "log() butuh angka");
  if (n >= 2 && args[1]->type == VAL_NUM)
    return val_angka(log(args[0]->num) / log(args[1]->num));
  return val_angka(log(args[0]->num));
}

static Val *bawaan_kosinus(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_NUM, "kosinus() butuh angka");
  return val_angka(cos(args[0]->num));
}

static Val *bawaan_sinus(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_NUM, "sinus() butuh angka");
  return val_angka(sin(args[0]->num));
}

static Val *bawaan_akarKuadrat(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_NUM, "akarKuadrat() butuh angka");
  if (args[0]->num < 0)
    loop_lempar_jenis("GalatNilai", "Baris %d: akarKuadrat() dari angka negatif",
              line);
  return val_angka(sqrt(args[0]->num));
}

static Val *bawaan_PI(Val **args, int n, int line) {
  return val_angka(3.14159265358979323846);
}

static Val *bawaan_bitAND(Val **args, int n, int line) {
  ARG_ASSERT(n == 2 && args[0]->type == VAL_NUM && args[1]->type == VAL_NUM,
              "bitAND() butuh 2 angka");
  return val_angka((double)((int64_t)args[0]->num & (int64_t)args[1]->num));
}

static Val *bawaan_bitOR(Val **args, int n, int line) {
  ARG_ASSERT(n == 2 && args[0]->type == VAL_NUM && args[1]->type == VAL_NUM,
              "bitOR() butuh 2 angka");
  return val_angka((double)((int64_t)args[0]->num | (int64_t)args[1]->num));
}

static Val *bawaan_bitXOR(Val **args, int n, int line) {
  ARG_ASSERT(n == 2 && args[0]->type == VAL_NUM && args[1]->type == VAL_NUM,
              "bitXOR() butuh 2 angka");
  return val_angka((double)((int64_t)args[0]->num ^ (int64_t)args[1]->num));
}

static Val *bawaan_bitKiri(Val **args, int n, int line) {
  ARG_ASSERT(n == 2 && args[0]->type == VAL_NUM && args[1]->type == VAL_NUM,
              "bitKiri() butuh 2 angka");
  return val_angka((double)((int64_t)args[0]->num << (int64_t)args[1]->num));
}

static Val *bawaan_bitKanan(Val **args, int n, int line) {
  ARG_ASSERT(n == 2 && args[0]->type == VAL_NUM && args[1]->type == VAL_NUM,
              "bitKanan() butuh 2 angka");
  return val_angka((double)((int64_t)args[0]->num >> (int64_t)args[1]->num));
}

static Val *bawaan_bitNOT(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_NUM, "bitNOT() butuh 1 angka");
  return val_angka((double)(~(int64_t)args[0]->num));
}

static Val *bawaan_sistemEnvGet(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_STR,
              "_sistemEnvGet() butuh nama teks");
  const char *v = getenv(args[0]->str);
  return val_teks(v ? v : "");
}

static Val *bawaan_sistemEnvSet(Val **args, int n, int line) {
  ARG_ASSERT(n == 2 && args[0]->type == VAL_STR && args[1]->type == VAL_STR,
              "_sistemEnvSet() butuh (nama, nilai)");
  return val_boolean(setenv(args[0]->str, args[1]->str, 1) == 0);
}

static Val *bawaan_sistemEnvHapus(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_STR,
              "_sistemEnvHapus() butuh nama teks");
  return val_boolean(unsetenv(args[0]->str) == 0);
}

static Val *bawaan_sistemEnvSemua(Val **args, int n, int line) {
  int cnt = 0;
  for (char **e = environ; *e; e++)
    cnt++;
  Val **out = cnt > 0 ? malloc(sizeof(Val *) * (size_t)cnt) : NULL;
  for (int i = 0; i < cnt; i++)
    out[i] = val_teks(environ[i]);
  return val_larik(out, cnt);
}

static Val *bawaan_sistemArgumen(Val **args, int n, int line) {
  return val_referensi(env_ambil(g_global_env, "argumen"));
}

static Val *bawaan_sistemDirKerja(Val **args, int n, int line) {
  char buf[4096];
  if (!getcwd(buf, sizeof(buf)))
    return val_teks("");
  return val_teks(buf);
}

static Val *bawaan_sistemPindahDir(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_STR,
              "_sistemPindahDir() butuh path teks");
  return val_boolean(chdir(args[0]->str) == 0);
}

static Val *bawaan_sistemPathSep(Val **args, int n, int line) {
  return val_teks("/");
}

static Val *bawaan_sistemNamaOS(Val **args, int n, int line) {
  return val_teks("linux");
}

static Val *bawaan_sistemArsitektur(Val **args, int n, int line) {
  struct utsname u;
  if (uname(&u) != 0)
    return val_teks("?");
  return val_teks(u.machine);
}

static Val *bawaan_sistemJumlahCPU(Val **args, int n, int line) {
  long c = sysconf(_SC_NPROCESSORS_ONLN);
  return val_angka(c > 0 ? (double)c : 1);
}

static Val *bawaan_sistemPID(Val **args, int n, int line) {
  return val_angka((double)getpid());
}

static Val *bawaan_sistemKeluar(Val **args, int n, int line) {
  int code = (n >= 1 && args[0]->type == VAL_NUM) ? (int)args[0]->num : 0;
  exit(code);
  return val_kosong();
}

static Val *bawaan_sistemJalankan(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_STR,
              "_sistemJalankan() butuh teks perintah");
  int rc = system(args[0]->str);
  if (rc == -1)
    return val_angka(-1);
  if (WIFEXITED(rc))
    return val_angka(WEXITSTATUS(rc));
  return val_angka(-1);
}

static Val *bawaan_trimKiri(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_STR, "trimKiri() butuh teks");
  const char *s = args[0]->str ? args[0]->str : "";
  int a = 0, b = (int)strlen(s);
  while (a < b && isspace((unsigned char)s[a]))
    a++;
  char *buf = strdup(s + a);
  Val *r = val_teks(buf);
  free(buf);
  return r;
}

static Val *bawaan_trimKanan(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_STR, "trimKanan() butuh teks");
  const char *s = args[0]->str ? args[0]->str : "";
  int b = (int)strlen(s) - 1;
  while (b >= 0 && isspace((unsigned char)s[b]))
    b--;
  char *buf = malloc(b + 2);
  memcpy(buf, s, b + 1);
  buf[b + 1] = 0;
  Val *r = val_teks(buf);
  free(buf);
  return r;
}

static Val *bawaan_hurufKapital(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_STR, "hurufKapital() butuh teks");
  const char *s = args[0]->str ? args[0]->str : "";
  char *buf = strdup(s);
  int cap = 1;
  for (int i = 0; buf[i]; i++) {
    if (isspace((unsigned char)buf[i]) || buf[i] == '.' || buf[i] == '!' ||
        buf[i] == '?') {
      cap = 1;
    } else if (cap && isalpha((unsigned char)buf[i])) {
      buf[i] = toupper((unsigned char)buf[i]);
      cap = 0;
    } else {
      buf[i] = tolower((unsigned char)buf[i]);
    }
  }
  Val *r = val_teks(buf);
  free(buf);
  return r;
}

static Val *bawaan_balikTeks(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_STR, "balikTeks() butuh teks");
  const char *s = args[0]->str ? args[0]->str : "";
  int len = (int)strlen(s);
  char *buf = malloc(len + 1);
  for (int i = 0; i < len; i++)
    buf[i] = s[len - 1 - i];
  buf[len] = 0;
  Val *r = val_teks(buf);
  free(buf);
  return r;
}

static Val *bawaan_hapusIndeks(Val **args, int n, int line) {
  ARG_ASSERT(n == 2 && args[0]->type == VAL_ARRAY && args[1]->type == VAL_NUM,
              "hapusIndeks() butuh (larik, indeks)");
  Val *a = args[0];
  int idx = (int)args[1]->num;
  if (idx < 0 || idx >= a->arr_n) {
    Val **arr = a->arr_n > 0 ? malloc(sizeof(Val *) * a->arr_n) : NULL;
    for (int i = 0; i < a->arr_n; i++)
      arr[i] = val_referensi(a->arr[i]);
    return val_larik(arr, a->arr_n);
  }
  int sz = a->arr_n - 1;
  Val **arr = sz > 0 ? malloc(sizeof(Val *) * (size_t)sz) : NULL;
  int j = 0;
  for (int i = 0; i < a->arr_n; i++)
    if (i != idx)
      arr[j++] = val_referensi(a->arr[i]);
  return val_larik(arr, sz);
}

static Val *bawaan_sisip(Val **args, int n, int line) {
  ARG_ASSERT(n == 3 && args[0]->type == VAL_ARRAY && args[1]->type == VAL_NUM,
              "sisip() butuh (larik, indeks, nilai)");
  Val *a = args[0];
  int idx = (int)args[1]->num;
  if (idx < 0)
    idx = 0;
  if (idx > a->arr_n)
    idx = a->arr_n;
  int sz = a->arr_n + 1;
  Val **arr = malloc(sizeof(Val *) * (size_t)sz);
  int j = 0;
  for (int i = 0; i < sz; i++) {
    if (i == idx)
      arr[i] = val_referensi(args[2]);
    else
      arr[i] = val_referensi(a->arr[j++]);
  }
  return val_larik(arr, sz);
}

static Val *bawaan_ambilIndeks(Val **args, int n, int line) {
  ARG_ASSERT(n == 2 && args[0]->type == VAL_ARRAY && args[1]->type == VAL_NUM,
              "ambilIndeks() butuh (larik, indeks)");
  Val *a = args[0];
  int idx = (int)args[1]->num;
  if (idx < 0)
    idx = a->arr_n + idx;
  if (idx < 0 || idx >= a->arr_n)
    return val_kosong();
  return val_referensi(a->arr[idx]);
}

static Val *bawaan_isiIndeks(Val **args, int n, int line) {
  ARG_ASSERT(n == 3 && args[0]->type == VAL_ARRAY && args[1]->type == VAL_NUM,
              "isiIndeks() butuh (larik, indeks, nilai)");
  Val *a = args[0];
  int idx = (int)args[1]->num;
  if (idx < 0)
    idx = a->arr_n + idx;
  if (idx < 0 || idx >= a->arr_n)
    return val_referensi(a);
  val_hapus(a->arr[idx]);
  a->arr[idx] = val_referensi(args[2]);
  return val_referensi(a);
}

static Val *bawaan_potongArray(Val **args, int n, int line) {
  ARG_ASSERT(n == 3 && args[0]->type == VAL_ARRAY &&
                  args[1]->type == VAL_NUM && args[2]->type == VAL_NUM,
              "potongArray() butuh (larik, mulai, banyak)");
  Val *a = args[0];
  int m = (int)args[1]->num;
  int ambil = (int)args[2]->num;
  if (m < 0)
    m = a->arr_n + m;
  if (m < 0)
    m = 0;
  if (m >= a->arr_n || ambil <= 0)
    return val_larik(NULL, 0);
  if (m + ambil > a->arr_n)
    ambil = a->arr_n - m;
  Val **arr = malloc(sizeof(Val *) * (size_t)ambil);
  for (int i = 0; i < ambil; i++)
    arr[i] = val_referensi(a->arr[m + i]);
  return val_larik(arr, ambil);
}

static Val *bawaan_unik(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_ARRAY, "unik() butuh larik");
  Val *a = args[0];
  Val **out = NULL;
  int cnt = 0, cap = 0;
  for (int i = 0; i < a->arr_n; i++) {
    int found = 0;
    for (int j = 0; j < cnt; j++) {
      Val *e = a->arr[i], *o = out[j];
      if (e->type == o->type) {
        if (e->type == VAL_NUM && e->num == o->num) {
          found = 1;
          break;
        }
        if (e->type == VAL_STR &&
            strcmp(e->str ? e->str : "", o->str ? o->str : "") == 0) {
          found = 1;
          break;
        }
        if (e->type == VAL_BOOL && e->bval == o->bval) {
          found = 1;
          break;
        }
      }
    }
    if (!found) {
      if (cnt >= cap) {
        cap = cap ? cap * 2 : 8;
        out = realloc(out, sizeof(Val *) * cap);
      }
      out[cnt++] = val_referensi(a->arr[i]);
    }
  }
  return val_larik(out, cnt);
}

static Val *bawaan_urutTurun(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_ARRAY, "urutTurun() butuh larik");
  Val *sorted = bawaan_urut(args, n, line);
  Val *reversed = bawaan_balik(&sorted, 1, line);
  val_hapus(sorted);
  return reversed;
}

static Val *bawaan_cariTerakhir(Val **args, int n, int line) {
  ARG_ASSERT(n == 2 && args[0]->type == VAL_ARRAY,
              "cariTerakhir() butuh (larik, nilai)");
  Val *a = args[0], *target = args[1];
  for (int i = a->arr_n - 1; i >= 0; i--) {
    Val *e = a->arr[i];
    if (e->type == target->type) {
      if (e->type == VAL_NUM && e->num == target->num)
        return val_angka(i);
      if (e->type == VAL_STR &&
          strcmp(e->str ? e->str : "", target->str ? target->str : "") == 0)
        return val_angka(i);
      if (e->type == VAL_BOOL && e->bval == target->bval)
        return val_angka(i);
    }
  }
  return val_angka(-1);
}

static Val *bawaan_ada(Val **args, int n, int line) {
  ARG_ASSERT(n == 2 && args[0]->type == VAL_ARRAY,
              "ada() butuh (larik, nilai)");
  return bawaan_adaDalam(args, n, line);
}

static Val *bawaan_median(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_ARRAY && args[0]->arr_n > 0,
              "median() butuh larik isi");
  Val *sorted = bawaan_urut(args, n, line);
  int mid = sorted->arr_n / 2;
  double med;
  if (sorted->arr_n % 2 == 1)
    med = sorted->arr[mid]->num;
  else
    med = (sorted->arr[mid - 1]->num + sorted->arr[mid]->num) / 2.0;
  val_hapus(sorted);
  return val_angka(med);
}

static Val *bawaan_modus(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_ARRAY && args[0]->arr_n > 0,
              "modus() butuh larik isi");
  Val *a = args[0];
  Val **uniq = NULL;
  int *freq = NULL;
  int cnt = 0, cap = 0;
  for (int i = 0; i < a->arr_n; i++) {
    int found = -1;
    for (int j = 0; j < cnt; j++) {
      Val *e = a->arr[i], *u = uniq[j];
      if (e->type == u->type) {
        if (e->type == VAL_NUM && e->num == u->num) {
          found = j;
          break;
        }
        if (e->type == VAL_STR &&
            strcmp(e->str ? e->str : "", u->str ? u->str : "") == 0) {
          found = j;
          break;
        }
        if (e->type == VAL_BOOL && e->bval == u->bval) {
          found = j;
          break;
        }
      }
    }
    if (found == -1) {
      if (cnt >= cap) {
        cap = cap ? cap * 2 : 8;
        uniq = realloc(uniq, sizeof(Val *) * cap);
        freq = realloc(freq, sizeof(int) * cap);
      }
      uniq[cnt] = val_referensi(a->arr[i]);
      freq[cnt++] = 1;
    } else {
      freq[found]++;
    }
  }
  int maxf = 0, maxi = 0;
  for (int i = 0; i < cnt; i++) {
    if (freq[i] > maxf) {
      maxf = freq[i];
      maxi = i;
    }
  }
  Val *r = val_referensi(uniq[maxi]);
  for (int i = 0; i < cnt; i++)
    val_hapus(uniq[i]);
  free(uniq);
  free(freq);
  return r;
}

static Val *bawaan_pangkat(Val **args, int n, int line) {
  ARG_ASSERT(n == 2 && args[0]->type == VAL_NUM && args[1]->type == VAL_NUM,
              "pangkat() butuh (alas, eksponen)");
  return val_angka(pow(args[0]->num, args[1]->num));
}

static Val *bawaan_faktorial(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_NUM && args[0]->num >= 0,
              "faktorial() butuh angka >= 0");
  long long result = 1;
  for (int i = 2; i <= (int)args[0]->num; i++)
    result *= i;
  return val_angka((double)result);
}

static Val *bawaan_fibonacci(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_NUM && args[0]->num >= 0,
              "fibonacci() butuh angka >= 0");
  int N = (int)args[0]->num;
  if (N <= 0)
    return val_angka(0);
  if (N == 1)
    return val_angka(1);
  long long a = 0, b = 1;
  for (int i = 2; i <= N; i++) {
    long long c = a + b;
    a = b;
    b = c;
  }
  return val_angka((double)b);
}

static Val *bawaan_keBoolean(Val **args, int n, int line) {
  ARG_ASSERT(n == 1, "keBoolean() butuh 1 argumen");
  Val *v = args[0];
  /* Boolean sudah boolean: keBoolean(salah) harus salah. Sebelumnya VAL_BOOL
   tidak ditangani dan jatuh ke 'return benar' di bawah. */
  if (v->type == VAL_BOOL)
    return val_boolean(v->bval != 0);
  if (v->type == VAL_NULL || (v->type == VAL_NUM && v->num == 0))
    return val_boolean(0);
  if (v->type == VAL_STR && v->str && strlen(v->str) == 0)
    return val_boolean(0);
  if (v->type == VAL_ARRAY && v->arr_n == 0)
    return val_boolean(0);
  return val_boolean(1);
}

static Val *bawaan_tipeData(Val **args, int n, int line) {
  return bawaan_tipe(args, n, line);
}

static Val *bawaan_isAngka(Val **args, int n, int line) {
  ARG_ASSERT(n == 1, "isAngka() butuh 1 argumen");
  return val_boolean(args[0]->type == VAL_NUM);
}

static Val *bawaan_isTeks(Val **args, int n, int line) {
  ARG_ASSERT(n == 1, "isTeks() butuh 1 argumen");
  return val_boolean(args[0]->type == VAL_STR);
}

static Val *bawaan_isLarik(Val **args, int n, int line) {
  ARG_ASSERT(n == 1, "isLarik() butuh 1 argumen");
  return val_boolean(args[0]->type == VAL_ARRAY);
}

static Val *bawaan_isKamus(Val **args, int n, int line) {
  ARG_ASSERT(n == 1, "isKamus() butuh 1 argumen");
  if (args[0]->type != VAL_ARRAY)
    return val_boolean(0);
  Val *a = args[0];
  for (int i = 0; i < a->arr_n; i++) {
    if (a->arr[i]->type != VAL_ARRAY || a->arr[i]->arr_n != 2)
      return val_boolean(0);
  }
  return val_boolean(1);
}

static Val *bawaan_isBoolean(Val **args, int n, int line) {
  ARG_ASSERT(n == 1, "isBoolean() butuh 1 argumen");
  return val_boolean(args[0]->type == VAL_BOOL);
}

static Val *bawaan_isNull(Val **args, int n, int line) {
  ARG_ASSERT(n == 1, "isNull() butuh 1 argumen");
  return val_boolean(args[0]->type == VAL_NULL);
}

static Val *bawaan_cetak(Val **args, int n, int line) {
  for (int i = 0; i < n; i++) {
    if (i > 0)
      printf(" ");
    if (args[i]->type == VAL_STR) {
      printf("%s", args[i]->str ? args[i]->str : "");
    } else {
      char *buf = val_ke_teks(args[i]);
      fputs(buf, stdout);
      free(buf);
    }
  }
  printf("\n");
  return val_kosong();
}

static Val *bawaan_kamusBaru(Val **args, int n, int line) {
  return val_larik(NULL, 0);
}

static Val *bawaan_kamusSet(Val **args, int n, int line) {
  ARG_ASSERT(n == 3, "kamusSet() butuh (kamus, key, value)");
  ARG_ASSERT(args[0]->type == VAL_ARRAY,
              "kamusSet(): argumen pertama harus kamus");
  Val *d = args[0];
  for (int i = 0; i < d->arr_n; i++) {
    LOOP_ASSERT(d->arr[i]->type == VAL_ARRAY && d->arr[i]->arr_n == 2,
                "kamus invalid: harus [key, value]");
    Val *k = d->arr[i]->arr[0];
    if (k->type == args[1]->type) {
      int sama = 0;
      if (k->type == VAL_NUM && k->num == args[1]->num)
        sama = 1;
      else if (k->type == VAL_STR &&
               strcmp(k->str ? k->str : "", args[1]->str ? args[1]->str : "") ==
                   0)
        sama = 1;
      else if (k->type == VAL_BOOL && k->bval == args[1]->bval)
        sama = 1;
      if (sama) {
        val_hapus(d->arr[i]->arr[1]);
        d->arr[i]->arr[1] = val_referensi(args[2]);
        return val_referensi(d);
      }
    }
  }
  Val **pair_arr = malloc(sizeof(Val *) * 2);
  pair_arr[0] = val_referensi(args[1]);
  pair_arr[1] = val_referensi(args[2]);
  Val *pair = val_larik(pair_arr, 2);
  Val **new_arr = malloc(sizeof(Val *) * (d->arr_n + 1));
  for (int i = 0; i < d->arr_n; i++)
    new_arr[i] = val_referensi(d->arr[i]);
  new_arr[d->arr_n] = pair;
  return val_larik(new_arr, d->arr_n + 1);
}

static Val *bawaan_kamusAmbil(Val **args, int n, int line) {
  ARG_ASSERT(n == 2, "kamusAmbil() butuh (kamus, key)");
  ARG_ASSERT(args[0]->type == VAL_ARRAY,
              "kamusAmbil(): argumen pertama harus kamus");
  Val *d = args[0];
  for (int i = 0; i < d->arr_n; i++) {
    Val *k = d->arr[i]->arr[0];
    if (k->type == args[1]->type) {
      int sama = 0;
      if (k->type == VAL_NUM && k->num == args[1]->num)
        sama = 1;
      else if (k->type == VAL_STR &&
               strcmp(k->str ? k->str : "", args[1]->str ? args[1]->str : "") ==
                   0)
        sama = 1;
      else if (k->type == VAL_BOOL && k->bval == args[1]->bval)
        sama = 1;
      if (sama)
        return val_referensi(d->arr[i]->arr[1]);
    }
  }
  return val_teks("");
}

static Val *bawaan_kamusAda(Val **args, int n, int line) {
  ARG_ASSERT(n == 2, "kamusAda() butuh (kamus, key)");
  ARG_ASSERT(args[0]->type == VAL_ARRAY,
              "kamusAda(): argumen pertama harus kamus");
  Val *d = args[0];
  for (int i = 0; i < d->arr_n; i++) {
    Val *k = d->arr[i]->arr[0];
    if (k->type == args[1]->type) {
      int sama = 0;
      if (k->type == VAL_NUM && k->num == args[1]->num)
        sama = 1;
      else if (k->type == VAL_STR &&
               strcmp(k->str ? k->str : "", args[1]->str ? args[1]->str : "") ==
                   0)
        sama = 1;
      else if (k->type == VAL_BOOL && k->bval == args[1]->bval)
        sama = 1;
      if (sama)
        return val_boolean(1);
    }
  }
  return val_boolean(0);
}

static Val *bawaan_kamusHapus(Val **args, int n, int line) {
  ARG_ASSERT(n == 2, "kamusHapus() butuh (kamus, key)");
  ARG_ASSERT(args[0]->type == VAL_ARRAY,
              "kamusHapus(): argumen pertama harus kamus");
  Val *d = args[0];
  int found = -1;
  for (int i = 0; i < d->arr_n; i++) {
    Val *k = d->arr[i]->arr[0];
    if (k->type == args[1]->type) {
      int sama = 0;
      if (k->type == VAL_NUM && k->num == args[1]->num)
        sama = 1;
      else if (k->type == VAL_STR &&
               strcmp(k->str ? k->str : "", args[1]->str ? args[1]->str : "") ==
                   0)
        sama = 1;
      else if (k->type == VAL_BOOL && k->bval == args[1]->bval)
        sama = 1;
      if (sama) {
        found = i;
        break;
      }
    }
  }
  if (found == -1)
    return val_referensi(d);
  int sz = d->arr_n - 1;
  Val **arr = sz > 0 ? malloc(sizeof(Val *) * (size_t)sz) : NULL;
  int j = 0;
  for (int i = 0; i < d->arr_n; i++) {
    if (i != found)
      arr[j++] = val_referensi(d->arr[i]);
  }
  return val_larik(arr, sz);
}

static Val *bawaan_kamusKunci(Val **args, int n, int line) {
  ARG_ASSERT(n == 1, "kamusKunci() butuh (kamus)");
  ARG_ASSERT(args[0]->type == VAL_ARRAY, "kamusKunci(): argumen harus kamus");
  Val *d = args[0];
  Val **arr = d->arr_n > 0 ? malloc(sizeof(Val *) * (size_t)d->arr_n) : NULL;
  for (int i = 0; i < d->arr_n; i++)
    arr[i] = val_referensi(d->arr[i]->arr[0]);
  return val_larik(arr, d->arr_n);
}

static Val *bawaan_kamusNilai(Val **args, int n, int line) {
  ARG_ASSERT(n == 1, "kamusNilai() butuh (kamus)");
  ARG_ASSERT(args[0]->type == VAL_ARRAY, "kamusNilai(): argumen harus kamus");
  Val *d = args[0];
  Val **arr = d->arr_n > 0 ? malloc(sizeof(Val *) * (size_t)d->arr_n) : NULL;
  for (int i = 0; i < d->arr_n; i++)
    arr[i] = val_referensi(d->arr[i]->arr[1]);
  return val_larik(arr, d->arr_n);
}

static Val *bawaan_kamusUkuran(Val **args, int n, int line) {
  ARG_ASSERT(n == 1, "kamusUkuran() butuh (kamus)");
  ARG_ASSERT(args[0]->type == VAL_ARRAY, "kamusUkuran(): argumen harus kamus");
  return val_angka((double)args[0]->arr_n);
}

static Val *bawaan_kamusKosong(Val **args, int n, int line) {
  ARG_ASSERT(n == 1, "kamusKosong() butuh (kamus)");
  ARG_ASSERT(args[0]->type == VAL_ARRAY, "kamusKosong(): argumen harus kamus");
  return val_boolean(args[0]->arr_n == 0);
}

static Val *bawaan_kamusGabung(Val **args, int n, int line) {
  ARG_ASSERT(n == 2, "kamusGabung() butuh (kamus1, kamus2)");
  ARG_ASSERT(args[0]->type == VAL_ARRAY && args[1]->type == VAL_ARRAY,
              "kamusGabung(): argumen harus kamus");
  Val *d1 = args[0], *d2 = args[1];
  Val **arr = malloc(sizeof(Val *) * (d1->arr_n + d2->arr_n));
  int idx = 0;
  for (int i = 0; i < d1->arr_n; i++)
    arr[idx++] = val_referensi(d1->arr[i]);
  for (int i = 0; i < d2->arr_n; i++)
    arr[idx++] = val_referensi(d2->arr[i]);
  return val_larik(arr, idx);
}

/* ===================================================================== */
/* ===== Builtin baru: galat, bita, berkas, kripto native (OpenSSL) ===== */
/* ===================================================================== */

static const char *nama_tipe(const Val *v) {
  switch (v->type) {
  case VAL_NUM: return "angka";
  case VAL_STR: return "teks";
  case VAL_BOOL: return "bool";
  case VAL_ARRAY: return "larik";
  case VAL_FUNC: return "fungsi";
  case VAL_PRIMA: return "kunci_prima";
  case VAL_ENKRIPSI: return "terenkripsi";
  case VAL_GANDA: return "ganda";
  case VAL_KONEKSI: return "koneksi";
  case VAL_BITA: return "bita";
  case VAL_GALAT: return "galat";
  default: return "kosong";
  }
}

/* ---- galat ---- */
static Val *bawaan_galat(Val **args, int n, int line) {
  ARG_ASSERT(n == 2 || n == 3, "galat() butuh (jenis, pesan) atau (jenis, pesan, sebab)");
  ARG_ASSERT(args[0]->type == VAL_STR && args[1]->type == VAL_STR,
             "galat(): jenis dan pesan harus teks");
  if (args[0]->str[0] == 0)
    loop_lempar_jenis("GalatNilai", "galat(): jenis tidak boleh kosong");
  Val *sebab = NULL;
  if (n == 3) {
    ARG_ASSERT(args[2]->type == VAL_GALAT || args[2]->type == VAL_NULL,
               "galat(): sebab harus objek galat atau kosong");
    if (args[2]->type == VAL_GALAT)
      sebab = args[2];
  }
  return val_galat(args[0]->str, args[1]->str, sebab);
}

static Val *galat_arg(Val **args, int n, const char *fn) {
  if (n != 1 || args[0]->type != VAL_GALAT)
    loop_lempar_jenis("GalatTipe", "%s() butuh 1 objek galat", fn);
  return args[0];
}
static Val *bawaan_jenisGalat(Val **args, int n, int line) {
  return val_teks(galat_arg(args, n, "jenisGalat")->galat_jenis);
}
static Val *bawaan_pesanGalat(Val **args, int n, int line) {
  return val_teks(galat_arg(args, n, "pesanGalat")->str);
}
static Val *bawaan_lokasiGalat(Val **args, int n, int line) {
  Val *g = galat_arg(args, n, "lokasiGalat");
  return val_teks(g->galat_lokasi ? g->galat_lokasi : "");
}
static Val *bawaan_sebabGalat(Val **args, int n, int line) {
  Val *g = galat_arg(args, n, "sebabGalat");
  return g->galat_sebab ? val_referensi(g->galat_sebab) : val_kosong();
}
static Val *bawaan_jejakGalat(Val **args, int n, int line) {
  Val *g = galat_arg(args, n, "jejakGalat");
  return val_teks(g->galat_jejak ? g->galat_jejak : "");
}

/* ---- bita ---- */
static void ambil_byte(Val *v, const char *fn, const char *arg,
                       const unsigned char **p, size_t *len) {
  if (v->type == VAL_BITA) {
    *p = v->bita;
    *len = v->bita_n;
  } else if (v->type == VAL_STR) {
    *p = (const unsigned char *)v->str;
    *len = strlen(v->str);
  } else {
    loop_lempar_jenis("GalatTipe", "%s: %s harus bita atau teks, dapat %s", fn,
                      arg, nama_tipe(v));
  }
}

static int utf8_valid(const unsigned char *s, size_t n) {
  size_t i = 0;
  while (i < n) {
    unsigned char c = s[i];
    if (c < 0x80) { i++; continue; }
    int extra;
    uint32_t cp;
    if (c >= 0xC2 && c <= 0xDF) { extra = 1; cp = c & 0x1F; }
    else if (c >= 0xE0 && c <= 0xEF) { extra = 2; cp = c & 0x0F; }
    else if (c >= 0xF0 && c <= 0xF4) { extra = 3; cp = c & 0x07; }
    else return 0;
    if (i + (size_t)extra >= n) return 0;
    for (int k = 1; k <= extra; k++) {
      if ((s[i + k] & 0xC0) != 0x80) return 0;
      cp = (cp << 6) | (s[i + k] & 0x3F);
    }
    if (extra == 2 && (cp < 0x800 || (cp >= 0xD800 && cp <= 0xDFFF))) return 0;
    if (extra == 3 && (cp < 0x10000 || cp > 0x10FFFF)) return 0;
    i += (size_t)extra + 1;
  }
  return 1;
}

static Val *bawaan_keBita(Val **args, int n, int line) {
  ARG_ASSERT(n == 1, "keBita() butuh 1 argumen (teks, bita, atau larik angka 0-255)");
  Val *a = args[0];
  if (a->type == VAL_STR)
    return val_bita((const unsigned char *)a->str, strlen(a->str));
  if (a->type == VAL_BITA)
    return val_bita(a->bita, a->bita_n);
  if (a->type == VAL_ARRAY) {
    periksa_batas((size_t)a->arr_n, "larik");
    unsigned char *buf = malloc(a->arr_n ? (size_t)a->arr_n : 1);
    for (int i = 0; i < a->arr_n; i++) {
      Val *e = a->arr[i];
      if (e->type != VAL_NUM || e->num < 0 || e->num > 255 ||
          e->num != floor(e->num)) {
        free(buf);
        loop_lempar_jenis("GalatNilai",
                          "keBita(): elemen ke-%d harus bilangan bulat 0 sampai 255", i);
      }
      buf[i] = (unsigned char)e->num;
    }
    Val *r = val_bita(buf, (size_t)a->arr_n);
    free(buf);
    return r;
  }
  loop_lempar_jenis("GalatTipe", "keBita() tidak bisa untuk tipe %s", nama_tipe(a));
}

static Val *bawaan_keHex(Val **args, int n, int line) {
  ARG_ASSERT(n == 1, "keHex() butuh 1 argumen");
  const unsigned char *p; size_t len;
  ambil_byte(args[0], "keHex()", "argumen", &p, &len);
  periksa_batas(len * 2, "keluaran hex");
  char *h = malloc(len * 2 + 1);
  static const char D[] = "0123456789abcdef";
  for (size_t i = 0; i < len; i++) { h[2*i] = D[p[i] >> 4]; h[2*i+1] = D[p[i] & 15]; }
  h[len * 2] = 0;
  Val *r = val_teks(h);
  free(h);
  return r;
}

static int nilai_hex(int c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

static Val *bawaan_dariHex(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_STR, "dariHex() butuh (teks hex)");
  const char *s = args[0]->str;
  size_t L = strlen(s);
  if (L % 2 != 0)
    loop_lempar_jenis("GalatNilai", "dariHex(): panjang hex harus genap (%zu)", L);
  unsigned char *b = malloc(L / 2 + 1);
  for (size_t i = 0; i < L / 2; i++) {
    int hi = nilai_hex((unsigned char)s[2*i]), lo = nilai_hex((unsigned char)s[2*i+1]);
    if (hi < 0 || lo < 0) {
      free(b);
      loop_lempar_jenis("GalatNilai", "dariHex(): karakter bukan hex pada posisi %zu", 2*i);
    }
    b[i] = (unsigned char)(hi * 16 + lo);
  }
  Val *r = val_bita(b, L / 2);
  free(b);
  return r;
}

static const char B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static Val *bawaan_keBase64(Val **args, int n, int line) {
  ARG_ASSERT(n == 1, "keBase64() butuh 1 argumen");
  const unsigned char *p; size_t len;
  ambil_byte(args[0], "keBase64()", "argumen", &p, &len);
  periksa_batas(len / 3 * 4 + 4, "keluaran base64");
  char *o = malloc((len + 2) / 3 * 4 + 1);
  size_t k = 0;
  for (size_t i = 0; i < len; i += 3) {
    uint32_t v = (uint32_t)p[i] << 16;
    if (i + 1 < len) v |= (uint32_t)p[i+1] << 8;
    if (i + 2 < len) v |= p[i+2];
    o[k++] = B64[(v >> 18) & 63];
    o[k++] = B64[(v >> 12) & 63];
    o[k++] = i + 1 < len ? B64[(v >> 6) & 63] : '=';
    o[k++] = i + 2 < len ? B64[v & 63] : '=';
  }
  o[k] = 0;
  Val *r = val_teks(o);
  free(o);
  return r;
}

static Val *bawaan_dariBase64(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_STR, "dariBase64() butuh (teks base64)");
  const char *s = args[0]->str;
  size_t L = strlen(s);
  if (L % 4 != 0)
    loop_lempar_jenis("GalatNilai", "dariBase64(): panjang harus kelipatan 4 (%zu)", L);
  unsigned char *b = malloc(L / 4 * 3 + 1);
  size_t k = 0;
  for (size_t i = 0; i < L; i += 4) {
    int v[4], pad = 0;
    for (int j = 0; j < 4; j++) {
      char c = s[i + j];
      if (c == '=') {
        if (i + 4 != L || j < 2) { free(b); loop_lempar_jenis("GalatNilai", "dariBase64(): padding '=' salah tempat"); }
        v[j] = 0; pad++;
      } else {
        if (pad) { free(b); loop_lempar_jenis("GalatNilai", "dariBase64(): karakter setelah padding"); }
        const char *q = strchr(B64, c);
        if (!c || !q) { free(b); loop_lempar_jenis("GalatNilai", "dariBase64(): karakter tidak valid pada posisi %zu", i + j); }
        v[j] = (int)(q - B64);
      }
    }
    uint32_t w = ((uint32_t)v[0] << 18) | ((uint32_t)v[1] << 12) | ((uint32_t)v[2] << 6) | (uint32_t)v[3];
    b[k++] = (unsigned char)(w >> 16);
    if (pad < 2) b[k++] = (unsigned char)(w >> 8);
    if (pad < 1) b[k++] = (unsigned char)w;
  }
  Val *r = val_bita(b, k);
  free(b);
  return r;
}

static Val *bita_arg(Val **args, int i, const char *fn) {
  if (args[i]->type != VAL_BITA)
    loop_lempar_jenis("GalatTipe", "%s: argumen ke-%d harus bita, dapat %s", fn, i + 1,
                      nama_tipe(args[i]));
  return args[i];
}

static Val *bawaan_panjangBita(Val **args, int n, int line) {
  ARG_ASSERT(n == 1, "panjangBita() butuh 1 argumen");
  return val_angka((double)bita_arg(args, 0, "panjangBita()")->bita_n);
}

static size_t indeks_arg(Val *v, const char *fn, const char *nama) {
  if (v->type != VAL_NUM)
    loop_lempar_jenis("GalatTipe", "%s: %s harus angka", fn, nama);
  if (v->num < 0 || v->num != floor(v->num))
    loop_lempar_jenis("GalatIndeks", "%s: %s harus bilangan bulat >= 0", fn, nama);
  return (size_t)v->num;
}

static Val *bawaan_potongBita(Val **args, int n, int line) {
  ARG_ASSERT(n == 3, "potongBita() butuh (bita, awal, akhir)");
  Val *b = bita_arg(args, 0, "potongBita()");
  size_t a = indeks_arg(args[1], "potongBita()", "awal");
  size_t e = indeks_arg(args[2], "potongBita()", "akhir");
  if (a > e || e > b->bita_n)
    loop_lempar_jenis("GalatIndeks",
                      "potongBita(): rentang [%zu, %zu) di luar bita (panjang %zu)", a, e, b->bita_n);
  return val_bita(b->bita + a, e - a);
}

static Val *bawaan_sambungBita(Val **args, int n, int line) {
  ARG_ASSERT(n == 2, "sambungBita() butuh (bita, bita)");
  Val *a = bita_arg(args, 0, "sambungBita()");
  Val *b = bita_arg(args, 1, "sambungBita()");
  periksa_batas(a->bita_n + b->bita_n, "hasil sambungBita");
  unsigned char *buf = malloc(a->bita_n + b->bita_n + 1);
  if (a->bita_n) memcpy(buf, a->bita, a->bita_n);
  if (b->bita_n) memcpy(buf + a->bita_n, b->bita, b->bita_n);
  Val *r = val_bita(buf, a->bita_n + b->bita_n);
  free(buf);
  return r;
}

/* ---- berkas ---- */
static unsigned char *baca_biner(const char *path, size_t *out_n) {
  FILE *f = fopen(path, "rb");
  if (!f)
    loop_lempar_jenis("GalatBerkas", "tidak bisa membuka '%s': %s", path, strerror(errno));
  size_t cap = 4096, len = 0;
  unsigned char *buf = malloc(cap);
  for (;;) {
    if (len == cap) {
      cap *= 2;
      buf = realloc(buf, cap);
    }
    size_t got = fread(buf + len, 1, cap - len, f);
    len += got;
    if (len > g_maks_ukuran) {
      free(buf);
      fclose(f);
      loop_lempar_jenis("GalatBatas", "berkas '%s' melewati batas %zu byte (atur LOOP_MAKS_UKURAN)", path, g_maks_ukuran);
    }
    if (got == 0)
      break;
  }
  if (ferror(f)) {
    int e = errno;
    free(buf);
    fclose(f);
    loop_lempar_jenis("GalatBerkas", "gagal membaca '%s': %s", path, strerror(e));
  }
  fclose(f);
  *out_n = len;
  return buf;
}

static void tulis_biner(const char *path, const unsigned char *d, size_t n, const char *mode) {
  FILE *f = fopen(path, mode);
  if (!f)
    loop_lempar_jenis("GalatBerkas", "tidak bisa menulis '%s': %s", path, strerror(errno));
  int ok = (n == 0) || fwrite(d, 1, n, f) == n;
  int e = errno;
  if (fclose(f) != 0 && ok) { ok = 0; e = errno; }
  if (!ok)
    loop_lempar_jenis("GalatBerkas", "gagal menulis '%s': %s", path, strerror(e));
}

static void tulis_atomik(const char *path, const unsigned char *d, size_t n) {
  size_t pl = strlen(path);
  char *tmp = malloc(pl + 16);
  snprintf(tmp, pl + 16, "%s.tmpXXXXXX", path);
  char pesan[1200];
  int fd = mkstemp(tmp);
  if (fd < 0) {
    snprintf(pesan, sizeof(pesan), "tidak bisa membuat berkas sementara untuk '%s': %s", path, strerror(errno));
    free(tmp);
    loop_lempar_jenis("GalatBerkas", "%s", pesan);
  }
  struct stat st;
  mode_t mode;
  if (stat(path, &st) == 0) {
    mode = st.st_mode & 07777;
  } else {
    mode_t um = umask(0);
    umask(um);
    mode = 0666 & ~um;
  }
  int gagal = 0;
  if (fchmod(fd, mode) != 0) gagal = errno;
  size_t off = 0;
  while (!gagal && off < n) {
    ssize_t w = write(fd, d + off, n - off);
    if (w < 0) {
      if (errno == EINTR) continue;
      gagal = errno;
    } else {
      off += (size_t)w;
    }
  }
  if (!gagal && fsync(fd) != 0) gagal = errno;
  if (close(fd) != 0 && !gagal) gagal = errno;
  if (!gagal && rename(tmp, path) != 0) gagal = errno;
  if (gagal) {
    unlink(tmp);
    snprintf(pesan, sizeof(pesan), "gagal menulis '%s' secara atomik: %s", path, strerror(gagal));
    free(tmp);
    loop_lempar_jenis("GalatBerkas", "%s", pesan);
  }
  free(tmp);
}

static Val *bawaan_adaBerkas(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_STR, "adaBerkas() butuh (path)");
  struct stat st;
  return val_boolean(stat(args[0]->str, &st) == 0);
}

static Val *bawaan_bacaBita(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_STR, "bacaBita() butuh (path)");
  size_t len;
  unsigned char *b = baca_biner(args[0]->str, &len);
  Val *r = val_bita(b, len);
  free(b);
  return r;
}

static Val *bawaan_tulisBita(Val **args, int n, int line) {
  ARG_ASSERT(n == 2 && args[0]->type == VAL_STR, "tulisBita() butuh (path, bita)");
  Val *b = bita_arg(args, 1, "tulisBita()");
  tulis_biner(args[0]->str, b->bita, b->bita_n, "wb");
  return val_boolean(1);
}

static Val *bawaan_tulisAtomik(Val **args, int n, int line) {
  ARG_ASSERT(n == 2 && args[0]->type == VAL_STR, "tulisAtomik() butuh (path, bita)");
  const unsigned char *p; size_t len;
  ambil_byte(args[1], "tulisAtomik()", "isi", &p, &len);
  tulis_atomik(args[0]->str, p, len);
  return val_teks(args[0]->str);
}

static Val *bawaan_pindahBerkas(Val **args, int n, int line) {
  ARG_ASSERT(n == 2 && args[0]->type == VAL_STR && args[1]->type == VAL_STR,
             "pindahBerkas() butuh (asal, tujuan)");
  const char *a = args[0]->str, *b = args[1]->str;
  if (rename(a, b) == 0)
    return val_teks(b);
  if (errno != EXDEV)
    loop_lempar_jenis("GalatBerkas", "gagal memindah '%s' ke '%s': %s", a, b, strerror(errno));
  /* beda filesystem: salin ke tujuan secara atomik lalu hapus asal */
  size_t len;
  unsigned char *d = baca_biner(a, &len);
  tulis_atomik(b, d, len);
  free(d);
  if (unlink(a) != 0)
    loop_lempar_jenis("GalatBerkas", "tersalin ke '%s' tetapi gagal menghapus '%s': %s", b, a, strerror(errno));
  return val_teks(b);
}

static Val *bawaan_hapusBerkas(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_STR, "hapusBerkas() butuh (path)");
  if (unlink(args[0]->str) != 0)
    loop_lempar_jenis("GalatBerkas", "gagal menghapus '%s': %s", args[0]->str, strerror(errno));
  return val_boolean(1);
}

/* ---- kripto native (OpenSSL EVP) ---- */
static Val *bawaan_acakAman(Val **args, int n, int line) {
  ARG_ASSERT(n == 1 && args[0]->type == VAL_NUM, "acakAman() butuh (jumlah byte)");
  double d = args[0]->num;
  if (!(d >= 0) || d != floor(d))
    loop_lempar_jenis("GalatNilai", "acakAman(): jumlah byte harus bilangan bulat >= 0");
  if (d > (double)g_maks_ukuran)
    loop_lempar_jenis("GalatBatas", "acakAman(): jumlah byte melewati batas");
  size_t len = (size_t)d;
  unsigned char *b = malloc(len ? len : 1);
  if (len && RAND_bytes(b, (int)len) != 1) {
    free(b);
    loop_lempar_jenis("GalatSistem", "acakAman(): sumber acak sistem gagal");
  }
  Val *r = val_bita(b, len);
  OPENSSL_cleanse(b, len);
  free(b);
  return r;
}

static Val *bawaan_turunkanKunci(Val **args, int n, int line) {
  ARG_ASSERT(n == 3 && args[2]->type == VAL_NUM, "turunkanKunci() butuh (sandi, garam, iterasi)");
  const unsigned char *pw, *gr; size_t pl, gl;
  ambil_byte(args[0], "turunkanKunci()", "sandi", &pw, &pl);
  ambil_byte(args[1], "turunkanKunci()", "garam", &gr, &gl);
  if (pl == 0)
    loop_lempar_jenis("GalatNilai", "turunkanKunci(): sandi tidak boleh kosong");
  if (gl == 0)
    loop_lempar_jenis("GalatNilai", "turunkanKunci(): garam tidak boleh kosong");
  double it = args[2]->num;
  if (!(it >= 1) || it != floor(it) || it > 2147483647.0)
    loop_lempar_jenis("GalatNilai", "turunkanKunci(): iterasi harus bilangan bulat >= 1");
  unsigned char out[32];
  if (PKCS5_PBKDF2_HMAC((const char *)pw, (int)pl, gr, (int)gl, (int)it, EVP_sha256(), 32, out) != 1)
    loop_lempar_jenis("GalatSistem", "turunkanKunci(): PBKDF2 gagal");
  Val *r = val_bita(out, 32);
  OPENSSL_cleanse(out, sizeof(out));
  return r;
}

static Val *bawaan_hmacSha256(Val **args, int n, int line) {
  ARG_ASSERT(n == 2, "hmacSha256() butuh (kunci, data)");
  const unsigned char *k, *d; size_t kl, dl;
  ambil_byte(args[0], "hmacSha256()", "kunci", &k, &kl);
  ambil_byte(args[1], "hmacSha256()", "data", &d, &dl);
  if (kl == 0)
    loop_lempar_jenis("GalatNilai", "hmacSha256(): kunci tidak boleh kosong");
  unsigned char out[EVP_MAX_MD_SIZE];
  unsigned int ol = 0;
  if (!HMAC(EVP_sha256(), k, (int)kl, d, dl, out, &ol))
    loop_lempar_jenis("GalatSistem", "hmacSha256(): HMAC gagal");
  return val_bita(out, ol);
}

#define GCM_NONCE 12
#define GCM_TAG 16

static Val *bawaan_enkripsiGCM(Val **args, int n, int line) {
  ARG_ASSERT(n == 2, "enkripsiGCM() butuh (kunci, data)");
  Val *kunci = bita_arg(args, 0, "enkripsiGCM()");
  if (kunci->bita_n != 32)
    loop_lempar_jenis("GalatNilai", "enkripsiGCM(): kunci harus 32 byte (dapat %zu)", kunci->bita_n);
  const unsigned char *d; size_t dl;
  ambil_byte(args[1], "enkripsiGCM()", "data", &d, &dl);
  periksa_batas(dl + GCM_NONCE + GCM_TAG, "data enkripsiGCM");
  if (dl > 0x7fffffffu - 64)
    loop_lempar_jenis("GalatBatas", "enkripsiGCM(): data terlalu besar");
  unsigned char *out = malloc(GCM_NONCE + dl + GCM_TAG + 16);
  if (RAND_bytes(out, GCM_NONCE) != 1) {
    free(out);
    loop_lempar_jenis("GalatSistem", "enkripsiGCM(): sumber acak sistem gagal");
  }
  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  int len = 0, total = 0, ok = ctx != NULL;
  ok = ok && EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) == 1;
  ok = ok && EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, GCM_NONCE, NULL) == 1;
  ok = ok && EVP_EncryptInit_ex(ctx, NULL, NULL, kunci->bita, out) == 1;
  if (ok && dl > 0)
    ok = EVP_EncryptUpdate(ctx, out + GCM_NONCE, &len, d, (int)dl) == 1;
  total = len;
  ok = ok && EVP_EncryptFinal_ex(ctx, out + GCM_NONCE + total, &len) == 1;
  total += len;
  ok = ok && EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, GCM_TAG, out + GCM_NONCE + total) == 1;
  if (ctx) EVP_CIPHER_CTX_free(ctx);
  if (!ok) {
    free(out);
    loop_lempar_jenis("GalatSistem", "enkripsiGCM(): enkripsi gagal");
  }
  Val *r = val_bita(out, GCM_NONCE + (size_t)total + GCM_TAG);
  free(out);
  return r;
}

static Val *bawaan_dekripsiGCM(Val **args, int n, int line) {
  ARG_ASSERT(n == 2, "dekripsiGCM() butuh (kunci, paket)");
  Val *kunci = bita_arg(args, 0, "dekripsiGCM()");
  Val *paket = bita_arg(args, 1, "dekripsiGCM()");
  if (kunci->bita_n != 32)
    loop_lempar_jenis("GalatNilai", "dekripsiGCM(): kunci harus 32 byte (dapat %zu)", kunci->bita_n);
  if (paket->bita_n < GCM_NONCE + GCM_TAG)
    loop_lempar_jenis("GalatNilai", "dekripsiGCM(): paket terlalu pendek (%zu byte, minimal %d)", paket->bita_n, GCM_NONCE + GCM_TAG);
  size_t cl = paket->bita_n - GCM_NONCE - GCM_TAG;
  if (cl > 0x7fffffffu - 64)
    loop_lempar_jenis("GalatBatas", "dekripsiGCM(): paket terlalu besar");
  unsigned char *pt = malloc(cl + 16);
  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  int len = 0, total = 0, ok = ctx != NULL;
  ok = ok && EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) == 1;
  ok = ok && EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, GCM_NONCE, NULL) == 1;
  ok = ok && EVP_DecryptInit_ex(ctx, NULL, NULL, kunci->bita, paket->bita) == 1;
  if (ok && cl > 0)
    ok = EVP_DecryptUpdate(ctx, pt, &len, paket->bita + GCM_NONCE, (int)cl) == 1;
  total = len;
  unsigned char tag[GCM_TAG];
  memcpy(tag, paket->bita + GCM_NONCE + cl, GCM_TAG);
  ok = ok && EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, GCM_TAG, tag) == 1;
  /* Final memverifikasi tag; gagal = kunci salah ATAU data diubah. */
  int verif = ok && EVP_DecryptFinal_ex(ctx, pt + total, &len) > 0;
  if (ctx) EVP_CIPHER_CTX_free(ctx);
  if (!verif) {
    OPENSSL_cleanse(pt, cl + 16);
    free(pt);
    loop_lempar_jenis("GalatKunci", "autentikasi gagal: kunci salah atau data telah diubah");
  }
  total += len;
  Val *r = val_bita(pt, (size_t)total);
  OPENSSL_cleanse(pt, cl + 16);
  free(pt);
  return r;
}

/* SEMENTARA (migrasi format .aes lama): AES-128-CBC + PKCS7, IV di 16 byte
   pertama. Format lama TIDAK ter-autentikasi; jangan dipakai untuk data baru.
   Dihapus setelah dua rilis bersama dekripsiBerkasV0. */
static Val *bawaan_dekripsiCBCLama(Val **args, int n, int line) {
  ARG_ASSERT(n == 2, "dekripsiCBCLama() butuh (kunci16, iv_dan_ciphertext)");
  Val *k = bita_arg(args, 0, "dekripsiCBCLama()");
  Val *d = bita_arg(args, 1, "dekripsiCBCLama()");
  if (k->bita_n != 16)
    loop_lempar_jenis("GalatNilai", "dekripsiCBCLama(): kunci harus 16 byte");
  if (d->bita_n < 32 || d->bita_n % 16 != 0)
    loop_lempar_jenis("GalatNilai",
                      "dekripsiCBCLama(): data harus IV 16 byte + ciphertext kelipatan 16 byte");
  size_t cl = d->bita_n - 16;
  unsigned char *pt = malloc(cl + 16);
  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  int len = 0, total = 0, ok = ctx != NULL;
  ok = ok && EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, k->bita, d->bita) == 1;
  ok = ok && EVP_DecryptUpdate(ctx, pt, &len, d->bita + 16, (int)cl) == 1;
  total = len;
  ok = ok && EVP_DecryptFinal_ex(ctx, pt + total, &len) == 1;
  if (ctx) EVP_CIPHER_CTX_free(ctx);
  if (!ok) {
    free(pt);
    loop_lempar_jenis("GalatKunci", "dekripsiCBCLama(): sandi salah atau data lama rusak (padding tidak valid)");
  }
  total += len;
  Val *r = val_bita(pt, (size_t)total);
  free(pt);
  return r;
}

static Val *bawaan_samaWaktuKonstan(Val **args, int n, int line) {
  ARG_ASSERT(n == 2, "samaWaktuKonstan() butuh (a, b)");
  const unsigned char *a, *b; size_t al, bl;
  ambil_byte(args[0], "samaWaktuKonstan()", "a", &a, &al);
  ambil_byte(args[1], "samaWaktuKonstan()", "b", &b, &bl);
  if (al != bl)
    return val_boolean(0);
  return val_boolean(al == 0 || CRYPTO_memcmp(a, b, al) == 0);
}

typedef struct {
  const char *nama;
  BuiltinFn fn;
} BuiltinEntry;
static const BuiltinEntry BUILTINS[] = {
    {"panjang", bawaan_panjang},
    {"jumlah", bawaan_jumlah},
    {"maks", bawaan_maks},
    {"min", bawaan_min},
    {"rerata", bawaan_rerata},
    {"urut", bawaan_urut},
    {"balik", bawaan_balik},
    {"angkaTeks", bawaan_angkaTeks},
    {"teksAngka", bawaan_teksAngka},
    {"masukan", bawaan_masukan},
    {"tipe", bawaan_tipe},
    {"tambah", bawaan_tambah},
    {"gabung", bawaan_gabung},
    {"prima", bawaan_prima},
    {"acak", bawaan_acak},
    {"ganda", bawaan_ganda},
    {"kodeKarakter", bawaan_kodeKarakter},
    {"karakter", bawaan_karakter},
    {"bulatkan", bawaan_bulatkan},
    {"irisLarik", bawaan_irisLarik},
    {"bacaFileTeks", bawaan_bacaFileTeks},
    {"tulisFileTeks", bawaan_tulisFileTeks},
    {"gabungTeks", bawaan_gabungTeks},
    {"cetakTanpaBaris", bawaan_cetakTanpaBaris},
    {"eksekusi", bawaan_eksekusi},
    {"syscall", bawaan_syscall},
    {"sambung", bawaan_sambung},
    {"tlsSambung", bawaan_tlsSambung},
    {"kirim", bawaan_kirim},
    {"terima", bawaan_terima},
    {"tutupKoneksi", bawaan_tutupKoneksi},
    {"ambil", bawaan_ambil},
    {"telusuri", bawaan_telusuri},
    {"tambahFileTeks", bawaan_tambahFileTeks},
    {"appendFile", bawaan_appendFile},
    {"tulis", bawaan_tulis},
    {"tulisInline", bawaan_tulisInline},
    {"tulisStderr", bawaan_tulisStderr},
    {"tulisError", bawaan_tulisError},
    {"barisBaru", bawaan_barisBaru},
    {"waktumilisdetik", bawaan_waktumilisdetik},
    {"waktuSekarang", bawaan_waktuSekarang},
    {"waktuISO", bawaan_waktuISO},
    {"tidur", bawaan_tidur},
    {"keAngka", bawaan_keAngka},
    {"keDesimal", bawaan_keDesimal},
    {"keTeks", bawaan_keTeks},
    {"dariKode", bawaan_dariKode},
    {"potong", bawaan_potong},
    {"cariTeks", bawaan_cariTeks},
    {"gantiTeks", bawaan_gantiTeks},
    {"ganti", bawaan_gantiTeks},
    {"hurufBesar", bawaan_hurufBesar},
    {"hurufKecil", bawaan_hurufKecil},
    {"naik", bawaan_hurufBesar},
    {"turunkan", bawaan_hurufKecil},
    {"pisah", bawaan_pisah},
    {"trimTeks", bawaan_trimTeks},
    {"trim", bawaan_trimTeks},
    {"mulaiDengan", bawaan_mulaiDengan},
    {"akhiriDengan", bawaan_akhiriDengan},
    {"regexGanti", bawaan_regexGanti},
    {"regexCocok", bawaan_regexCocok},
    {"terakhirIndexDari", bawaan_terakhirIndexDari},
    {"dorong", bawaan_dorong},
    {"hapusTerakhir", bawaan_hapusTerakhir},
    {"hapusKunci", bawaan_hapusKunci},
    {"hapusLarik", bawaan_hapusLarik},
    {"iris", bawaan_iris},
    {"adaDalam", bawaan_adaDalam},
    {"saring", bawaan_saring},
    {"peta", bawaan_peta},
    {"kurangi", bawaan_kurangi},
    {"jenisNilai", bawaan_tipe},
    {"kunciObjek", bawaan_kunciObjek},
    {"salin", bawaan_salin},
    {"lantai", bawaan_lantai},
    {"langit", bawaan_langit},
    {"absolut", bawaan_absolut},
    {"acakAngka", bawaan_acakAngka},
    {"log", bawaan_log},
    {"kosinus", bawaan_kosinus},
    {"sinus", bawaan_sinus},
    {"akarKuadrat", bawaan_akarKuadrat},
    {"PI", bawaan_PI},
    {"bitAND", bawaan_bitAND},
    {"bitOR", bawaan_bitOR},
    {"bitXOR", bawaan_bitXOR},
    {"bitKiri", bawaan_bitKiri},
    {"bitKanan", bawaan_bitKanan},
    {"bitNOT", bawaan_bitNOT},
    {"_sistemEnvGet", bawaan_sistemEnvGet},
    {"_sistemEnvSet", bawaan_sistemEnvSet},
    {"_sistemEnvHapus", bawaan_sistemEnvHapus},
    {"_sistemEnvSemua", bawaan_sistemEnvSemua},
    {"_sistemArgumen", bawaan_sistemArgumen},
    {"_sistemDirKerja", bawaan_sistemDirKerja},
    {"_sistemPindahDir", bawaan_sistemPindahDir},
    {"_sistemPathSep", bawaan_sistemPathSep},
    {"_sistemNamaOS", bawaan_sistemNamaOS},
    {"_sistemArsitektur", bawaan_sistemArsitektur},
    {"_sistemJumlahCPU", bawaan_sistemJumlahCPU},
    {"_sistemPID", bawaan_sistemPID},
    {"_sistemKeluar", bawaan_sistemKeluar},
    {"_sistemJalankan", bawaan_sistemJalankan},
    {"trimKiri", bawaan_trimKiri},
    {"trimKanan", bawaan_trimKanan},
    {"hurufKapital", bawaan_hurufKapital},
    {"balikTeks", bawaan_balikTeks},
    {"hapusIndeks", bawaan_hapusIndeks},
    {"sisip", bawaan_sisip},
    {"ambilIndeks", bawaan_ambilIndeks},
    {"isiIndeks", bawaan_isiIndeks},
    {"potongArray", bawaan_potongArray},
    {"unik", bawaan_unik},
    {"urutTurun", bawaan_urutTurun},
    {"cariTerakhir", bawaan_cariTerakhir},
    {"ada", bawaan_ada},
    {"median", bawaan_median},
    {"modus", bawaan_modus},
    {"pangkat", bawaan_pangkat},
    {"faktorial", bawaan_faktorial},
    {"fibonacci", bawaan_fibonacci},
    {"keBoolean", bawaan_keBoolean},
    {"tipeData", bawaan_tipeData},
    {"isAngka", bawaan_isAngka},
    {"isTeks", bawaan_isTeks},
    {"isLarik", bawaan_isLarik},
    {"isKamus", bawaan_isKamus},
    {"isBoolean", bawaan_isBoolean},
    {"isNull", bawaan_isNull},
    {"cetak", bawaan_cetak},
    {"kamusBaru", bawaan_kamusBaru},
    {"kamusSet", bawaan_kamusSet},
    {"kamusAmbil", bawaan_kamusAmbil},
    {"kamusAda", bawaan_kamusAda},
    {"kamusHapus", bawaan_kamusHapus},
    {"kamusKunci", bawaan_kamusKunci},
    {"kamusNilai", bawaan_kamusNilai},
    {"kamusUkuran", bawaan_kamusUkuran},
    {"kamusKosong", bawaan_kamusKosong},
    {"kamusGabung", bawaan_kamusGabung},
     {"terimaBita", bawaan_terimaBita},
     {"ambilBita", bawaan_ambilBita},
     {"galat", bawaan_galat},
     {"jenisGalat", bawaan_jenisGalat},
     {"pesanGalat", bawaan_pesanGalat},
     {"lokasiGalat", bawaan_lokasiGalat},
     {"sebabGalat", bawaan_sebabGalat},
     {"jejakGalat", bawaan_jejakGalat},
     {"keBita", bawaan_keBita},
     {"keHex", bawaan_keHex},
     {"dariHex", bawaan_dariHex},
     {"keBase64", bawaan_keBase64},
     {"dariBase64", bawaan_dariBase64},
     {"panjangBita", bawaan_panjangBita},
     {"potongBita", bawaan_potongBita},
     {"sambungBita", bawaan_sambungBita},
     {"adaBerkas", bawaan_adaBerkas},
     {"bacaBita", bawaan_bacaBita},
     {"tulisBita", bawaan_tulisBita},
     {"tulisAtomik", bawaan_tulisAtomik},
     {"pindahBerkas", bawaan_pindahBerkas},
     {"hapusBerkas", bawaan_hapusBerkas},
     {"acakAman", bawaan_acakAman},
     {"turunkanKunci", bawaan_turunkanKunci},
     {"hmacSha256", bawaan_hmacSha256},
     {"enkripsiGCM", bawaan_enkripsiGCM},
     {"dekripsiGCM", bawaan_dekripsiGCM},
     {"samaWaktuKonstan", bawaan_samaWaktuKonstan},
     {"dekripsiCBCLama", bawaan_dekripsiCBCLama},
    {NULL, NULL}};

static BuiltinFn cari_bawaan(const char *name) {
  for (int i = 0; BUILTINS[i].nama; i++)
    if (strcmp(BUILTINS[i].nama, name) == 0)
      return BUILTINS[i].fn;
  return NULL;
}

static void validasi_tipe(const char *tipe, Val *v, const char *nama,
                          int line) {
  if (!tipe[0])
    return;
  if (strcmp(tipe, "angka") == 0 && v->type != VAL_NUM)
    loop_lempar_jenis("GalatTipe", "Baris %d: Variabel '%s' harus angka", line, nama);
  if (strcmp(tipe, "teks") == 0 && v->type != VAL_STR)
    loop_lempar_jenis("GalatTipe", "Baris %d: Variabel '%s' harus teks", line, nama);
  if (strcmp(tipe, "bool") == 0 && v->type != VAL_BOOL)
    loop_lempar_jenis("GalatTipe", "Baris %d: Variabel '%s' harus bool", line, nama);
  if (strcmp(tipe, "larik") == 0 && v->type != VAL_ARRAY)
    loop_lempar_jenis("GalatTipe", "Baris %d: Variabel '%s' harus larik", line, nama);
  if (strcmp(tipe, "bita") == 0 && v->type != VAL_BITA)
    loop_lempar_jenis("GalatTipe", "Baris %d: Variabel '%s' harus bita", line, nama);
}

static Val *eval_panggil(const char *fname, Node **arg_nodes, int n_args,
                         Env *env, int line);
static Val *evaluasi(Node *n, Env *env) {
  if (!n)
    return val_kosong();
  if (n->line > 0)
    g_baris_kini = n->line;
  switch (n->type) {
  case N_LIT_NUM:
    return val_angka(n->num);
  case N_LIT_BOOL:
    return val_boolean(n->bval);
  case N_LIT_NULL:
    return val_kosong();
  case N_LIT_STR:
    return val_teks(n->str);
  case N_STRING_INTERP: {
    Sb out;
    sb_awal(&out);
    for (int i = 0; i < n->n_interp; i++) {
      InterpPart *p = &n->interp[i];
      if (!p->is_expr) {
        sb_str(&out, p->text);
      } else {
        Lexer *L2 = calloc(1, sizeof(Lexer));
        if (!L2)
          loop_galat("Out of memory (string interp lexer)");
        L2->src = p->text;
        tokenisasi(L2);
        Parser P2;
        P2.tokens = L2->tokens;
        P2.pos = 0;
        P2.n = L2->n_tokens;
        Node *expr = parse_expr(&P2);
        free(L2);
        Val *v = evaluasi(expr, env);
        char *tmp = val_ke_teks(v);
        sb_str(&out, tmp);
        free(tmp);
        val_hapus(v);
      }
    }
    return val_teks_ambil(out.p);
  }
  case N_VAR:
    return val_referensi(env_ambil(env, n->str));
  case N_ARRAY: {
    Val **arr = malloc(sizeof(Val *) * n->n_elems);
    for (int i = 0; i < n->n_elems; i++)
      arr[i] = evaluasi(n->elems[i], env);
    return val_larik(arr, n->n_elems);
  }
  case N_INDEX: {
    Val *obj = evaluasi(n->obj, env);
    Val *idx = evaluasi(n->idx, env);
    if (obj->type != VAL_ARRAY && obj->type != VAL_BITA)
      loop_lempar_jenis("GalatTipe", "Baris %d: Indeks hanya untuk larik atau bita",
                        n->line);
    if (idx->type != VAL_NUM)
      loop_lempar_jenis("GalatTipe", "Baris %d: indeks harus angka", n->line);
    if (obj->type == VAL_BITA) {
      double di = idx->num;
      if (!(di >= 0) || di >= (double)obj->bita_n || di != floor(di))
        loop_lempar_jenis("GalatIndeks",
                          "Baris %d: Indeks %g di luar batas (panjang bita %zu)",
                          n->line, di, obj->bita_n);
      Val *rb = val_angka((double)obj->bita[(size_t)di]);
      val_hapus(obj);
      val_hapus(idx);
      return rb;
    }
    int i = (int)idx->num;
    if (!(i >= 0 && i < obj->arr_n))
      loop_lempar_jenis("GalatIndeks",
                        "Baris %d: Indeks %d di luar batas (panjang %d)", n->line,
                        i, obj->arr_n);
    Val *r = val_referensi(obj->arr[i]);
    val_hapus(obj);
    val_hapus(idx);
    return r;
  }
  case N_BINOP: {
    Val *L, *R;
    if (strcmp(n->op, "dan") == 0) {
      L = evaluasi(n->left, env);
      int lb = (L->type == VAL_BOOL ? L->bval
                                    : L->type != VAL_NULL &&
                                          !(L->type == VAL_NUM && L->num == 0));
      val_hapus(L);
      if (!lb)
        return val_boolean(0);
      R = evaluasi(n->right, env);
      int rb = (R->type == VAL_BOOL ? R->bval
                                    : R->type != VAL_NULL &&
                                          !(R->type == VAL_NUM && R->num == 0));
      val_hapus(R);
      return val_boolean(rb);
    }
    if (strcmp(n->op, "atau") == 0) {
      L = evaluasi(n->left, env);
      int lb = (L->type == VAL_BOOL ? L->bval
                                    : L->type != VAL_NULL &&
                                          !(L->type == VAL_NUM && L->num == 0));
      if (lb) {
        val_hapus(L);
        return val_boolean(1);
      }
      val_hapus(L);
      R = evaluasi(n->right, env);
      int rb = (R->type == VAL_BOOL ? R->bval
                                    : R->type != VAL_NULL &&
                                          !(R->type == VAL_NUM && R->num == 0));
      val_hapus(R);
      return val_boolean(rb);
    }
    L = evaluasi(n->left, env);
    R = evaluasi(n->right, env);
    Val *res = NULL;
    if (strcmp(n->op, "+") == 0) {
      if (L->type == VAL_STR || R->type == VAL_STR || L->type == VAL_GALAT ||
          R->type == VAL_GALAT) {
        res = gabung_dua(L, R);
      } else if (L->type == VAL_BITA || R->type == VAL_BITA) {
        loop_lempar_jenis("GalatTipe",
                          "Baris %d: bita tidak bisa dijumlah dengan + (gunakan sambungBita)",
                          n->line);
      } else
        res = val_angka(L->num + R->num);
    } else if (strcmp(n->op, "-") == 0)
      res = val_angka(L->num - R->num);
    else if (strcmp(n->op, "*") == 0)
      res = val_angka(L->num * R->num);
    else if (strcmp(n->op, "/") == 0) {
      if (R->num == 0)
        loop_lempar_jenis("GalatNilai", "Baris %d: pembagian dengan nol",
                          n->line);
      res = val_angka(L->num / R->num);
    } else if (strcmp(n->op, "%") == 0) {
      if (R->num == 0)
        loop_lempar_jenis("GalatNilai", "Baris %d: modulo dengan nol",
                          n->line);
      res = val_angka(fmod(L->num, R->num));
    }
    else if (strcmp(n->op, "==") == 0) {
      res = val_boolean(val_sama(L, R));
    } else if (strcmp(n->op, "!=") == 0) {
      res = val_boolean(!val_sama(L, R));
    } else if (strcmp(n->op, "<") == 0)
      res = val_boolean(L->num < R->num);
    else if (strcmp(n->op, ">") == 0)
      res = val_boolean(L->num > R->num);
    else if (strcmp(n->op, "<=") == 0)
      res = val_boolean(L->num <= R->num);
    else if (strcmp(n->op, ">=") == 0)
      res = val_boolean(L->num >= R->num);
    else
      loop_galat("Baris %d: Operator tak dikenal: '%s'", n->line, n->op);
    val_hapus(L);
    val_hapus(R);
    return res;
  }
  case N_UNIOP: {
    Val *v = evaluasi(n->left, env);
    if (strcmp(n->op, "-") == 0) {
      Val *r = val_angka(-v->num);
      val_hapus(v);
      return r;
    }
    if (strcmp(n->op, "tidak") == 0) {
      int b = (v->type == VAL_BOOL) ? !v->bval
                                    : (v->type == VAL_NULL ||
                                       (v->type == VAL_NUM && v->num == 0));
      val_hapus(v);
      return val_boolean(b);
    }
    val_hapus(v);
    return val_kosong();
  }
  case N_ASSIGN: {
    Val *v = evaluasi(n->right, env);
    if (n->var_tipe && n->var_tipe[0]) {
      validasi_tipe(n->var_tipe, v, n->varname, n->line);
      env_deklarasi(env, n->varname,
                    v); /* deklarasi: tidak menembus frame fungsi */
    } else {
      env_atur(env, n->varname, v); /* assignment biasa: cari ke atas */
    }
    Val *r = val_referensi(v);
    val_hapus(v);
    return r;
  }
  case N_ASSIGN_COMPOUND: {
    Val *cur = val_referensi(env_ambil(env, n->varname));
    Val *rhs = evaluasi(n->right, env);
    Val *res;
    char op = n->op[0];
    if (op == '+') {
      if (cur->type == VAL_STR) {
        res = gabung_dua(cur, rhs);
      } else
        res = val_angka(cur->num + rhs->num);
    } else if (op == '-')
      res = val_angka(cur->num - rhs->num);
    else if (op == '*')
      res = val_angka(cur->num * rhs->num);
    else if (op == '/') {
      if (rhs->num == 0)
        loop_lempar_jenis("GalatNilai", "Baris %d: pembagian dengan nol",
                          n->line);
      res = val_angka(cur->num / rhs->num);
    } else if (op == '%') {
      if (rhs->num == 0)
        loop_lempar_jenis("GalatNilai", "Baris %d: modulo dengan nol", n->line);
      res = val_angka(fmod(cur->num, rhs->num));
    }
    else
      res = val_kosong();
    env_atur(env, n->varname, res);
    val_hapus(cur);
    val_hapus(rhs);
    Val *r = val_referensi(res);
    val_hapus(res);
    return r;
  }
  case N_CETAK: {
    Val *v = evaluasi(n->left, env);
    char *buf = val_ke_teks(v);
    fputs(buf, stdout);
    fputc('\n', stdout);
    free(buf);
    Val *r = val_referensi(v);
    val_hapus(v);
    return r;
  }
  case N_JALANKAN: {
    Val *v = evaluasi(n->left, env);
    if (v->type != VAL_STR) {
      val_hapus(v);
      loop_lempar_jenis("GalatTipe", "Baris %d: jalankan() butuh teks perintah",
                        n->line);
    }
    int rc = system(v->str);
    Val *r;
    if (rc == -1)
      r = val_angka(-1);
    else if (WIFEXITED(rc))
      r = val_angka(WEXITSTATUS(rc));
    else
      r = val_angka(-1);
    val_hapus(v);
    return r;
  }
  case N_BACA_FILE: {
    FILE *f = fopen(n->str, "r");
    if (!f)
      loop_lempar_jenis("GalatBerkas", "Baris %d: tidak bisa membuka %s: %s",
                        n->line, n->str, strerror(errno));
    char *kode = baca_file_penuh(f);
    fclose(f);
    Val *r = jalankan_kode(kode, env, n->str);
    free(kode);
    return r;
  }
  case N_TULIS_FILE: {
    Val *isi = evaluasi(n->left, env);
    char *teks = val_ke_teks(isi);
    val_hapus(isi);
    FILE *f = fopen(n->str, "w");
    if (!f) {
      free(teks);
      loop_lempar_jenis("GalatBerkas", "Baris %d: tidak bisa menulis %s: %s",
                        n->line, n->str, strerror(errno));
    }
    size_t tl = strlen(teks);
    int ok = fwrite(teks, 1, tl, f) == tl;
    ok = (fclose(f) == 0) && ok;
    free(teks);
    if (!ok)
      loop_lempar_jenis("GalatBerkas", "Baris %d: gagal menulis %s: %s",
                        n->line, n->str, strerror(errno));
    return val_kosong();
  }
  case N_PAKAI: {
    for (int i = 0; i < g_pakai_n_loaded; i++) {
      if (strcmp(g_pakai_loaded[i], n->str) == 0)
        return val_kosong();
    }
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/%s.lp", g_stdlib_dir, n->str);
    FILE *f = fopen(path, "r");
    if (!f)
      loop_lempar_jenis("GalatBerkas",
                        "Baris %d: pakai: modul '%s' tidak ditemukan (dicari di %s)",
                        n->line, n->str, path);
    char *kode = baca_file_penuh(f);
    fclose(f);
    Val *r = jalankan_kode(kode, env, path);
    free(kode);
    if (g_pakai_n_loaded < 64) {
      g_pakai_loaded[g_pakai_n_loaded++] = strdup(n->str);
    }
    return r;
  }
  case N_FUNGSI:
    env_atur(env, n->fname, val_fungsi(n));
    return val_kosong();
  case N_PANGGIL: {
    return eval_panggil(n->fname, n->args, n->n_args, env, n->line);
  }
  case N_KEMBALI: {
    g_kembali_v = evaluasi(n->left, env);
    g_kembali = 1;
    return val_kosong();
  }
  case N_HENTIKAN:
    g_hentikan = 1;
    return val_kosong();
  case N_LEWATI:
    g_lewati = 1;
    return val_kosong();
  case N_JIKA: {
    Val *cond = evaluasi(n->cond, env);
    int ok = (cond->type == VAL_BOOL)
                 ? cond->bval
                 : (cond->type != VAL_NULL &&
                    !(cond->type == VAL_NUM && cond->num == 0));
    val_hapus(cond);
    Env *sub = env_baru(env);
    if (ok) {
      for (int i = 0;
           i < n->n_body && !g_kembali && !g_hentikan && !g_lewati;
           i++)
        val_hapus(evaluasi(n->body[i], sub));
    } else if (n->selain) {
      for (int i = 0; i < n->n_selain && !g_kembali && !g_hentikan &&
                      !g_lewati;
           i++)
        val_hapus(evaluasi(n->selain[i], sub));
    }
    env_hapus(sub);
    return val_kosong();
  }
  case N_SELAMA: {
    for (;;) {
      Val *cond = evaluasi(n->cond, env);
      int ok = (cond->type == VAL_BOOL)
                   ? cond->bval
                   : (cond->type != VAL_NULL &&
                      !(cond->type == VAL_NUM && cond->num == 0));
      val_hapus(cond);
      if (!ok)
        break;
      Env *sub = env_baru(env);
      for (int i = 0;
           i < n->n_body && !g_kembali && !g_hentikan && !g_lewati;
           i++)
        val_hapus(evaluasi(n->body[i], sub));
      env_hapus(sub);
      if (g_hentikan) {
        g_hentikan = 0;
        break;
      }
      if (g_lewati) {
        g_lewati = 0;
        continue;
      }
      if (g_kembali)
        break;
    }
    return val_kosong();
  }
  case N_ULANG: {
    Val *hv = evaluasi(n->left, env);
    int count = (int)hv->num;
    val_hapus(hv);
    for (int ci = 0; ci < count; ci++) {
      Env *sub = env_baru(env);
      for (int i = 0;
           i < n->n_body && !g_kembali && !g_hentikan && !g_lewati;
           i++)
        val_hapus(evaluasi(n->body[i], sub));
      env_hapus(sub);
      if (g_hentikan) {
        g_hentikan = 0;
        break;
      }
      if (g_lewati) {
        g_lewati = 0;
        continue;
      }
      if (g_kembali)
        break;
    }
    return val_kosong();
  }
  case N_PILIH: {
    Val *eksp = evaluasi(n->left, env);
    int matched = 0;
    for (int i = 0; i < n->n_kasus; i++) {
      Val *kval = evaluasi(n->kasus_val[i], env);
      int eq = val_sama(eksp, kval);
      val_hapus(kval);
      if (eq) {
        Env *sub = env_baru(env);
        val_hapus(evaluasi(n->kasus_body[i], sub));
        env_hapus(sub);
        matched = 1;
        break;
      }
    }
    if (!matched && n->lainnya) {
      Env *sub = env_baru(env);
      val_hapus(evaluasi(n->lainnya, sub));
      env_hapus(sub);
    }
    val_hapus(eksp);
    return val_kosong();
  }
  case N_COBA: {
    Val *pending = NULL;
    Env *sub = env_baru(env);
    Val *g = evaluasi_blok_terlindung(n->body, n->n_body, sub);
    env_hapus(sub);
    if (g && n->has_tangkap) {
      Env *ce = env_baru(env);
      /* Galat tersedia sebagai variabel `e` (objek galat) di blok tangkap. */
      env_aturLokal(ce, "e", g);
      val_hapus(g);
      pending = evaluasi_blok_terlindung(n->tangkap, n->n_tangkap, ce);
      env_hapus(ce);
    } else if (g) {
      pending = g;
    }
    if (n->has_akhirnya) {
      /* akhirnya selalu jalan: normal, galat, kembali/hentikan/lewati. */
      int sk = g_kembali, sh = g_hentikan, sl = g_lewati;
      Val *skv = g_kembali_v;
      g_kembali = g_hentikan = g_lewati = 0;
      g_kembali_v = NULL;
      Env *fe = env_baru(env);
      Val *g2 = evaluasi_blok_terlindung(n->akhirnya, n->n_akhirnya, fe);
      env_hapus(fe);
      if (g2) { /* galat baru di akhirnya menggantikan yang tertunda */
        val_hapus(pending);
        val_hapus(skv);
        pending = g2;
      } else if (g_kembali || g_hentikan || g_lewati) {
        /* akhirnya sendiri keluar lewat kembali/hentikan/lewati: menimpa */
        val_hapus(pending);
        pending = NULL;
        if (!g_kembali)
          val_hapus(skv);
      } else {
        g_kembali = sk;
        g_hentikan = sh;
        g_lewati = sl;
        g_kembali_v = skv;
      }
    }
    if (pending)
      loop_lempar(pending);
    return val_kosong();
  }
  case N_LEMPAR: {
    Val *v = evaluasi(n->left, env);
    if (v->type != VAL_GALAT) {
      val_hapus(v);
      loop_lempar_jenis("GalatTipe",
                        "Baris %d: lempar butuh objek galat, buat dengan "
                        "galat(jenis, pesan)",
                        n->line);
    }
    loop_lempar(v); /* melempar ulang objek asli: jenis, lokasi, jejak utuh */
  }
  case N_PIPA: {
    Val *kiri = evaluasi(n->pipa_kiri, env);
    Val **pargs = malloc(sizeof(Val *) * (n->n_pipa_extra + 1));
    pargs[0] = kiri;
    for (int i = 0; i < n->n_pipa_extra; i++)
      pargs[i + 1] = evaluasi(n->pipa_extra[i], env);
    int total = n->n_pipa_extra + 1;
    BuiltinFn bf = cari_bawaan(n->pipa_fn);
    Val *result;
    if (bf) {
      result = bf(pargs, total, n->line);
    } else {
      Val *fv = env_ambil(env, n->pipa_fn);
      ARG_ASSERT(fv && fv->type == VAL_FUNC, "Baris %d: '%s' bukan fungsi",
                 n->line, n->pipa_fn);
      Node *fn = fv->func_node;
      Env *fe = env_baru(g_global_env);
      fe->is_frame = 1;
      for (int i = 0; i < fn->n_params && i < total; i++)
        env_aturLokal(fe, fn->params[i], pargs[i]);
      frame_masuk(fn->fname, fn, n->line);
      for (int i = 0; i < fn->n_body && !g_kembali; i++)
        val_hapus(evaluasi(fn->body[i], fe));
      frame_keluar();
      result = g_kembali ? g_kembali_v : val_kosong();
      g_kembali = 0;
      g_kembali_v = NULL;
      env_hapus(fe);
    }
    for (int i = 0; i < total; i++)
      val_hapus(pargs[i]);
    free(pargs);
    return result;
  }
  case N_PROGRAM: {
    Val *last = val_kosong();
    for (int i = 0; i < n->n_body && !g_kembali; i++) {
      val_hapus(last);
      last = evaluasi(n->body[i], env);
    }
    return last;
  }
  default:
    loop_galat("Baris %d: Node tak dikenal: %d", n->line, n->type);
  }
  return val_kosong();
}

static Val *eval_panggil(const char *fname, Node **arg_nodes, int n_args,
                         Env *env, int line) {
  BuiltinFn bf = cari_bawaan(fname);
  Val **args = malloc(sizeof(Val *) * n_args);
  for (int i = 0; i < n_args; i++)
    args[i] = evaluasi(arg_nodes[i], env);
  Val *result;
  if (bf) {
    result = bf(args, n_args, line);
  } else {
    Val *fv = env_ambil(env, fname);
    ARG_ASSERT(fv && fv->type == VAL_FUNC, "Baris %d: '%s' bukan fungsi", line,
               fname);
    Node *fn = fv->func_node;
    ARG_ASSERT(n_args == fn->n_params,
                "Baris %d: Fungsi '%s' butuh %d argumen, dapat %d", line, fname,
                fn->n_params, n_args);
    Env *fe = env_baru(g_global_env);
    fe->is_frame = 1;
    for (int i = 0; i < fn->n_params; i++)
      env_aturLokal(fe, fn->params[i], args[i]);
    frame_masuk(fname, fn, line);
    for (int i = 0; i < fn->n_body && !g_kembali; i++)
      val_hapus(evaluasi(fn->body[i], fe));
    frame_keluar();
    result = g_kembali ? g_kembali_v : val_kosong();
    g_kembali = 0;
    g_kembali_v = NULL;
    env_hapus(fe);
  }
  for (int i = 0; i < n_args; i++)
    val_hapus(args[i]);
  free(args);
  return result;
}

static Val *panggil_nilai_fungsi(Val *fv, Val **args, int n_args, int line) {
  ARG_ASSERT(fv && fv->type == VAL_FUNC, "Baris %d: nilai bukan fungsi", line);
  Node *fn = fv->func_node;
  ARG_ASSERT(n_args == fn->n_params,
              "Baris %d: Fungsi '%s' butuh %d argumen, dapat %d", line,
              fn->fname ? fn->fname : "?", fn->n_params, n_args);
  Env *fe = env_baru(g_global_env);
  fe->is_frame = 1;
  for (int i = 0; i < fn->n_params; i++)
    env_aturLokal(fe, fn->params[i], args[i]);
  frame_masuk(fn->fname, fn, line);
  for (int i = 0; i < fn->n_body && !g_kembali; i++)
    val_hapus(evaluasi(fn->body[i], fe));
  frame_keluar();
  Val *result = g_kembali ? g_kembali_v : val_kosong();
  g_kembali = 0;
  g_kembali_v = NULL;
  env_hapus(fe);
  return result;
}

static const char *intern_berkas(const char *path) {
  static char *daftar[256];
  static int n = 0;
  for (int i = 0; i < n; i++)
    if (strcmp(daftar[i], path) == 0)
      return daftar[i];
  char *c = strdup(path);
  if (n < 256)
    daftar[n++] = c;
  return c;
}

static Val *jalankan_kode(const char *kode, Env *env, const char *berkas) {
  const char *lama_parse = g_berkas_parse, *lama_aktif = g_berkas_aktif;
  const char *nama = intern_berkas(berkas ? berkas : "<kode>");
  g_berkas_parse = nama;
  Lexer *L = calloc(1, sizeof(Lexer));
  if (!L)
    loop_galat("Out of memory (jalankan_kode lexer)");
  L->src = kode;
  tokenisasi(L);
  Node *prog = urai_program(L->tokens, L->n_tokens);
  free(L);
  g_berkas_parse = lama_parse;
  g_berkas_aktif = nama;
  Val *r = evaluasi(prog, env);
  g_berkas_aktif = lama_aktif;
  return r;
}

typedef struct {
  char data[1 << 20];
  int dlen;
  char text[1 << 22];
  int tlen;
  char bss[4096];
  int bsslen;
  int label_cnt;
  int str_cnt;
  char varnames[MAX_ENV_VARS][MAX_IDENT];
  int varoffs[MAX_ENV_VARS];
  int n_vars;
  int stack_sz;
  int in_func;
  char func_end[MAX_IDENT];
} Codegen;

/* Tulis ke buffer codegen: tidak memotong, dan melewati kapasitas = GalatBatas
   (sebelumnya dipotong diam-diam di 512 byte dan bisa menimpa memori). */
static void cg_tulis(char *buf, int *len, size_t cap, const char *fmt, ...)
    __attribute__((format(printf, 4, 5)));
static void cg_tulis(char *buf, int *len, size_t cap, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int k = vsnprintf(buf + *len, cap - (size_t)*len, fmt, ap);
  va_end(ap);
  if (k < 0 || (size_t)k >= cap - (size_t)*len)
    loop_lempar_jenis("GalatBatas",
                      "keluaran codegen melewati kapasitas buffer (%zu byte)", cap);
  *len += k;
}

#define CG_EMIT(cg, ...)                                                       \
  cg_tulis((cg)->text, &(cg)->tlen, sizeof((cg)->text), __VA_ARGS__)

#define CG_DATA(cg, ...)                                                       \
  cg_tulis((cg)->data, &(cg)->dlen, sizeof((cg)->data), __VA_ARGS__)

static char *cg_labelBaru(Codegen *cg, const char *pfx) {
  static char lbl[MAX_IDENT];
  snprintf(lbl, MAX_IDENT, "_%s%d", pfx, ++cg->label_cnt);
  return lbl;
}

static char *cg_labelStr(Codegen *cg, const char *s) {
  static char lbl[MAX_IDENT];
  snprintf(lbl, MAX_IDENT, "__str%d", ++cg->str_cnt);
  char *escaped = malloc(strlen(s) * 10 + 16);
  int ei = 0;
  for (int i = 0; s[i]; i++) {
    unsigned char c = (unsigned char)s[i];
    if (c == '\n') {
      memcpy(escaped + ei, "', 0xA, '", 9);
      ei += 9;
    } else if (c == '\t') {
      memcpy(escaped + ei, "', 0x9, '", 9);
      ei += 9;
    } else if (c == '\'') {
      memcpy(escaped + ei, "', 0x27, '", 10);
      ei += 10;
    } else {
      escaped[ei++] = c;
    }
  }
  escaped[ei] = 0;
  int bytes = strlen(s) + 2;
  CG_DATA(cg, "    %s db '%s', 0xA, 0    ; len=%d\n", lbl, escaped, bytes);
  free(escaped);
  return lbl;
}

static int cg_alokasiVar(Codegen *cg, const char *name) {
  for (int i = 0; i < cg->n_vars; i++)
    if (strcmp(cg->varnames[i], name) == 0)
      return cg->varoffs[i];
  cg->stack_sz += 8;
  int off = -cg->stack_sz;
  strncpy(cg->varnames[cg->n_vars], name, MAX_IDENT);
  cg->varoffs[cg->n_vars++] = off;
  return off;
}

static int cg_offsetVar(Codegen *cg, const char *name) {
  for (int i = 0; i < cg->n_vars; i++)
    if (strcmp(cg->varnames[i], name) == 0)
      return cg->varoffs[i];
  return cg_alokasiVar(cg, name);
}

static void cg_ekspresi(Codegen *cg, Node *n);
static void cg_pernyataan(Codegen *cg, Node *n);

static void cg_ekspresi(Codegen *cg, Node *n) {
  if (!n) {
    CG_EMIT(cg, "    xor  rax, rax\n");
    return;
  }
  switch (n->type) {
  case N_LIT_NUM:
    if (n->num == (int64_t)n->num)
      CG_EMIT(cg, "    mov  rax, %lld\n", (long long)(int64_t)n->num);
    else
      CG_EMIT(cg, "    mov  rax, 0  ; float %.6g (integer truncated)\n",
              n->num);
    break;
  case N_LIT_BOOL:
    CG_EMIT(cg, "    mov  rax, %d\n", n->bval);
    break;
  case N_LIT_NULL:
    CG_EMIT(cg, "    xor  rax, rax\n");
    break;
  case N_LIT_STR: {
    char *lbl = cg_labelStr(cg, n->str);
    CG_EMIT(cg, "    mov  rax, %s\n", lbl);
    break;
  }
  case N_VAR: {
    int off = cg_offsetVar(cg, n->str);
    CG_EMIT(cg, "    mov  rax, qword [rbp%+d]\n", off);
    break;
  }
  case N_UNIOP:
    cg_ekspresi(cg, n->left);
    if (strcmp(n->op, "-") == 0)
      CG_EMIT(cg, "    neg  rax\n");
    else if (strcmp(n->op, "tidak") == 0) {
      CG_EMIT(cg, "    test rax, rax\n    setz al\n    movzx rax, al\n");
    }
    break;
  case N_BINOP: {
    const char *op = n->op;
    if (strcmp(op, "dan") == 0) {
      char lf[MAX_IDENT], le[MAX_IDENT];
      snprintf(lf, MAX_IDENT, "_%d_df", ++cg->label_cnt);
      snprintf(le, MAX_IDENT, "_%d_de", cg->label_cnt);
      cg_ekspresi(cg, n->left);
      CG_EMIT(cg, "    test rax, rax\n    jz %s\n", lf);
      cg_ekspresi(cg, n->right);
      CG_EMIT(cg,
              "    test rax, rax\n    jz %s\n    mov rax, 1\n    jmp %s\n%s:\n "
              "   xor rax, rax\n%s:\n",
              lf, le, lf, le);
      break;
    }
    if (strcmp(op, "atau") == 0) {
      char lt[MAX_IDENT], le[MAX_IDENT];
      snprintf(lt, MAX_IDENT, "_%d_ot", ++cg->label_cnt);
      snprintf(le, MAX_IDENT, "_%d_oe", cg->label_cnt);
      cg_ekspresi(cg, n->left);
      CG_EMIT(cg, "    test rax, rax\n    jnz %s\n", lt);
      cg_ekspresi(cg, n->right);
      CG_EMIT(cg,
              "    test rax, rax\n    jnz %s\n    xor rax, rax\n    jmp "
              "%s\n%s:\n    mov rax, 1\n%s:\n",
              lt, le, lt, le);
      break;
    }
    cg_ekspresi(cg, n->left);
    CG_EMIT(cg, "    push rax\n");
    cg_ekspresi(cg, n->right);
    CG_EMIT(cg, "    mov  rcx, rax\n    pop  rax\n");
    if (strcmp(op, "+") == 0)
      CG_EMIT(cg, "    add  rax, rcx\n");
    else if (strcmp(op, "-") == 0)
      CG_EMIT(cg, "    sub  rax, rcx\n");
    else if (strcmp(op, "*") == 0)
      CG_EMIT(cg, "    imul rax, rcx\n");
    else if (strcmp(op, "/") == 0)
      CG_EMIT(cg, "    cqo\n    idiv rcx\n");
    else if (strcmp(op, "%") == 0)
      CG_EMIT(cg, "    cqo\n    idiv rcx\n    mov  rax, rdx\n");
    else {
      char ct[MAX_IDENT], ce[MAX_IDENT];
      snprintf(ct, MAX_IDENT, "_%d_ct", ++cg->label_cnt);
      snprintf(ce, MAX_IDENT, "_%d_ce", cg->label_cnt);
      CG_EMIT(cg, "    cmp  rax, rcx\n");
      const char *jmp = "je";
      if (strcmp(op, "!=") == 0)
        jmp = "jne";
      else if (strcmp(op, "<") == 0)
        jmp = "jl";
      else if (strcmp(op, ">") == 0)
        jmp = "jg";
      else if (strcmp(op, "<=") == 0)
        jmp = "jle";
      else if (strcmp(op, ">=") == 0)
        jmp = "jge";
      CG_EMIT(cg,
              "    %s %s\n    xor  rax, rax\n    jmp  %s\n%s:\n    mov  rax, "
              "1\n%s:\n",
              jmp, ct, ce, ct, ce);
    }
    break;
  }
  case N_PANGGIL: {
    const char *regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
    if (strcmp(n->fname, "tambah") == 0 && n->n_args == 2) {
      cg_ekspresi(cg, n->args[0]);
      CG_EMIT(cg, "    push rax\n");
      cg_ekspresi(cg, n->args[1]);
      CG_EMIT(cg, "    mov rcx, rax\n    pop rax\n    add rax, rcx\n");
      break;
    }
    for (int i = 0; i < n->n_args && i < 6; i++) {
      cg_ekspresi(cg, n->args[i]);
      CG_EMIT(cg, "    push rax\n");
    }
    for (int i = n->n_args - 1; i >= 0 && i < 6; i--)
      CG_EMIT(cg, "    pop  %s\n", regs[i]);
    CG_EMIT(cg, "    call %s\n", n->fname);
    break;
  }
  case N_ASSIGN: {
    cg_ekspresi(cg, n->right);
    int off = cg_alokasiVar(cg, n->varname);
    CG_EMIT(cg, "    mov  qword [rbp%+d], rax\n", off);
    break;
  }
  default:
    fprintf(stderr,
            "[LOOP KOMPILASI] Baris %d: node ekspresi tipe %d belum "
            "didukung codegen NASM (mis. larik, indeks larik, atau "
            "interpolasi teks). Kompilasi dihentikan di sini alih-alih "
            "diam-diam menghasilkan biner yang salah (nol).\n",
            n->line, n->type);
    exit(1);
  }
}

static const char *REGS[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

static void cg_fungsi(Codegen *cg, Node *fn) {
  char sv_names[MAX_ENV_VARS][MAX_IDENT];
  int sv_offs[MAX_ENV_VARS];
  int sv_n = cg->n_vars, sv_sz = cg->stack_sz, sv_in = cg->in_func;
  char sv_end[MAX_IDENT];
  memcpy(sv_names, cg->varnames, sizeof(sv_names));
  memcpy(sv_offs, cg->varoffs, sizeof(sv_offs));
  strncpy(sv_end, cg->func_end, MAX_IDENT);
  cg->n_vars = 0;
  cg->stack_sz = 0;
  cg->in_func = 1;
  snprintf(cg->func_end, MAX_IDENT, "_%s_end", fn->fname);
  CG_EMIT(cg, "\n; fungsi %s\n%s:\n    push rbp\n    mov  rbp, rsp\n",
          fn->fname, fn->fname);
  int ph = cg->tlen;
  CG_EMIT(cg, "    sub  rsp, %-6d         ; placeholder\n", 0);
  for (int i = 0; i < fn->n_params && i < 6; i++) {
    int off = cg_alokasiVar(cg, fn->params[i]);
    CG_EMIT(cg, "    mov  qword [rbp%+d], %s\n", off, REGS[i]);
  }
  for (int i = 0; i < fn->n_body; i++)
    cg_pernyataan(cg, fn->body[i]);
  CG_EMIT(cg, "    xor  rax, rax\n%s:\n", cg->func_end);
  int aligned = ((cg->stack_sz + 15) / 16) * 16;
  if (aligned == 0)
    aligned = 16;
  char patch[32];
  snprintf(patch, 32, "    sub  rsp, %d", aligned);
  memcpy(cg->text + ph, patch, strlen(patch));
  CG_EMIT(cg, "    mov  rsp, rbp\n    pop  rbp\n    ret\n");
  cg->n_vars = sv_n;
  cg->stack_sz = sv_sz;
  cg->in_func = sv_in;
  memcpy(cg->varnames, sv_names, sizeof(sv_names));
  memcpy(cg->varoffs, sv_offs, sizeof(sv_offs));
  strncpy(cg->func_end, sv_end, MAX_IDENT);
}

static void cg_pernyataan(Codegen *cg, Node *n) {
  if (!n)
    return;
  switch (n->type) {
  case N_FUNGSI:
    cg_fungsi(cg, n);
    break;
  case N_ASSIGN:
  case N_ASSIGN_COMPOUND: {
    cg_ekspresi(cg, n->right);
    int off = cg_alokasiVar(cg, n->varname);
    if (n->type == N_ASSIGN_COMPOUND) {
      CG_EMIT(cg, "    mov  rcx, rax\n    mov  rax, qword [rbp%+d]\n", off);
      char op = n->op[0];
      if (op == '+')
        CG_EMIT(cg, "    add  rax, rcx\n");
      else if (op == '-')
        CG_EMIT(cg, "    sub  rax, rcx\n");
      else if (op == '*')
        CG_EMIT(cg, "    imul rax, rcx\n");
      else if (op == '/')
        CG_EMIT(cg, "    cqo\n    idiv rcx\n");
      else if (op == '%')
        CG_EMIT(cg, "    cqo\n    idiv rcx\n    mov rax, rdx\n");
    }
    CG_EMIT(cg, "    mov  qword [rbp%+d], rax\n", off);
    break;
  }
  case N_CETAK: {
    Node *e = n->left;
    if (e->type == N_LIT_STR) {
      char *lbl = cg_labelStr(cg, e->str);
      int bytes = strlen(e->str) + 2;
      CG_EMIT(cg,
              "    mov  rax, 1\n    mov  rdi, 1\n    mov  rsi, %s\n    mov  "
              "rdx, %d\n    syscall\n",
              lbl, bytes);
    } else {
      cg_ekspresi(cg, e);
      CG_EMIT(cg, "    mov  rdi, rax\n    call __print_int\n");
    }
    break;
  }
  case N_JIKA: {
    char lsel[MAX_IDENT], lend[MAX_IDENT];
    snprintf(lsel, MAX_IDENT, "_%d_jsel", ++cg->label_cnt);
    snprintf(lend, MAX_IDENT, "_%d_jend", cg->label_cnt);
    cg_ekspresi(cg, n->cond);
    CG_EMIT(cg, "    test rax, rax\n    jz   %s\n", lsel);
    for (int i = 0; i < n->n_body; i++)
      cg_pernyataan(cg, n->body[i]);
    CG_EMIT(cg, "    jmp  %s\n%s:\n", lend, lsel);
    for (int i = 0; i < n->n_selain; i++)
      cg_pernyataan(cg, n->selain[i]);
    CG_EMIT(cg, "%s:\n", lend);
    break;
  }
  case N_SELAMA: {
    char lcek[MAX_IDENT], lend[MAX_IDENT];
    snprintf(lcek, MAX_IDENT, "_%d_wc", ++cg->label_cnt);
    snprintf(lend, MAX_IDENT, "_%d_we", cg->label_cnt);
    CG_EMIT(cg, "%s:\n", lcek);
    cg_ekspresi(cg, n->cond);
    CG_EMIT(cg, "    test rax, rax\n    jz   %s\n", lend);
    for (int i = 0; i < n->n_body; i++)
      cg_pernyataan(cg, n->body[i]);
    CG_EMIT(cg, "    jmp  %s\n%s:\n", lcek, lend);
    break;
  }
  case N_ULANG: {
    char lctr[MAX_IDENT], lcek[MAX_IDENT], lend[MAX_IDENT];
    snprintf(lctr, MAX_IDENT, "__uctr%d", ++cg->label_cnt);
    snprintf(lcek, MAX_IDENT, "_%d_uc", cg->label_cnt);
    snprintf(lend, MAX_IDENT, "_%d_ue", cg->label_cnt);
    cg_ekspresi(cg, n->left);
    int off = cg_alokasiVar(cg, lctr);
    CG_EMIT(cg,
            "    mov  qword [rbp%+d], rax\n%s:\n    cmp  qword [rbp%+d], 0\n   "
            " jle  %s\n",
            off, lcek, off, lend);
    for (int i = 0; i < n->n_body; i++)
      cg_pernyataan(cg, n->body[i]);
    CG_EMIT(cg, "    dec  qword [rbp%+d]\n    jmp  %s\n%s:\n", off, lcek, lend);
    break;
  }
  case N_KEMBALI:
    cg_ekspresi(cg, n->left);
    if (cg->func_end[0])
      CG_EMIT(cg, "    jmp  %s\n", cg->func_end);
    break;
  case N_PANGGIL:
    cg_ekspresi(cg, n);
    break;
  default:
    cg_ekspresi(cg, n);
    break;
  }
}

static void cg_pembantu(Codegen *cg) {
  CG_EMIT(cg,
          "\n__int_to_str:\n"
          "    push rbp\n    mov  rbp, rsp\n"
          "    push rbx\n    push r12\n    push r13\n"
          "    mov  rax, rdi\n"
          "    mov  rbx, __intbuf + 31\n"
          "    mov  byte [rbx], 0x0A\n    dec  rbx\n"
          "    xor  r12, r12\n    test rax, rax\n    jge  .pos\n"
          "    mov  r12, 1\n    neg  rax\n"
          ".pos:\n    mov  r13, 10\n"
          ".lp:\n    xor  rdx, rdx\n    div  r13\n"
          "    add  dl, 0x30\n    mov  [rbx], dl\n    dec  rbx\n"
          "    test rax, rax\n    jnz  .lp\n"
          "    test r12, r12\n    jz   .done\n"
          "    mov  byte [rbx], 0x2D\n    dec  rbx\n"
          ".done:\n    inc  rbx\n    mov  rsi, rbx\n"
          "    mov  rdx, __intbuf + 32\n    sub  rdx, rbx\n"
          "    pop  r13\n    pop  r12\n    pop  rbx\n    pop  rbp\n    ret\n"
          "\n__print_int:\n"
          "    push rbp\n    mov  rbp, rsp\n"
          "    call __int_to_str\n"
          "    mov  rax, 1\n    mov  rdi, 1\n    syscall\n"
          "    pop  rbp\n    ret\n");
}

static char *nasm_hasilkan(Node *prog) {
  Codegen *cg = calloc(1, sizeof(Codegen));
  cg->bsslen = snprintf(cg->bss, sizeof(cg->bss),
                        "section .bss\n    __intbuf resb 32\n");
  CG_EMIT(cg, "\nglobal _start\n_start:\n    push rbp\n    mov  rbp, rsp\n");
  int ph = cg->tlen;
  CG_EMIT(cg, "    sub  rsp, %-6d         ; placeholder\n", 0);
  for (int i = 0; i < prog->n_body; i++) {
    if (prog->body[i]->type != N_FUNGSI)
      cg_pernyataan(cg, prog->body[i]);
  }
  CG_EMIT(cg, "\n    xor  rdi, rdi\n    mov  rax, 60\n    syscall\n");
  int aligned = ((cg->stack_sz + 15) / 16) * 16;
  if (aligned == 0)
    aligned = 16;
  char patch[32];
  snprintf(patch, 32, "    sub  rsp, %d", aligned);
  memcpy(cg->text + ph, patch, strlen(patch));
  for (int i = 0; i < prog->n_body; i++)
    if (prog->body[i]->type == N_FUNGSI)
      cg_fungsi(cg, prog->body[i]);
  cg_pembantu(cg);
  int outsz = 64 + cg->dlen + cg->bsslen + cg->tlen + 64;
  char *out = malloc(outsz);
  snprintf(out, outsz,
           "; Loop Compiler v%s - NASM x86-64 Linux\n"
           "section .data\n%s\n%s\nsection .text\n%s\n",
           LOOP_VERSI, cg->data, cg->bss, cg->text);
  free(cg);
  return out;
}

static char *baca_file(const char *path) {
  FILE *f = fopen(path, "r");
  if (!f) {
    fprintf(stderr, "[LOOP] File tidak ditemukan: %s\n", path);
    exit(1);
  }
  char *buf = baca_file_penuh(f);
  fclose(f);
  return buf;
}

static int repl_masih_gantung(Token *toks, int n) {
  int depth = 0;
  for (int i = 0; i < n; i++) {
    TokenType t = toks[i].type;
    if (t == TK_LBRACE || t == TK_JIKA)
      depth++;
    else if (t == TK_RBRACE || t == TK_AKHIR)
      depth--;
  }
  return depth > 0;
}

static int repl_node_adalah_stmt(NodeType t) {
  return t == N_ASSIGN || t == N_ASSIGN_COMPOUND || t == N_CETAK ||
         t == N_BACA_FILE || t == N_TULIS_FILE || t == N_PAKAI || t == N_JIKA ||
         t == N_SELAMA || t == N_ULANG || t == N_PILIH || t == N_COBA ||
         t == N_LEMPAR ||
         t == N_FUNGSI || t == N_KEMBALI || t == N_HENTIKAN || t == N_LEWATI ||
         t == N_PROGRAM;
}

static void jalankan_repl(void) {
  printf("Loop %s (main, %s %s) [GCC %s] on linux\n", LOOP_VERSI, __DATE__,
         __TIME__, __VERSION__);
  printf("Ketik pernyataan Loop. Ctrl-D buat keluar.\n");
  g_global_env = env_baru(NULL);
  Val *argumen_val = val_larik(NULL, 0);
  env_atur(g_global_env, "argumen", argumen_val);
  val_hapus(argumen_val);
  g_in_repl = 1;
  g_berkas_parse = g_berkas_aktif = "<repl>";
  static char accum[65536];
  char linebuf[4096];
  accum[0] = 0;
  for (;;) {
    if (setjmp(g_repl_jmp)) {
      g_handler = NULL;
      g_frames_n = 0;
      g_berkas_aktif = "<repl>";
      g_kembali = g_hentikan = g_lewati = 0;
      accum[0] = 0;
    }
    printf(accum[0] ? "... " : ">>> ");
    fflush(stdout);
    if (!fgets(linebuf, sizeof(linebuf), stdin)) {
      printf("\n");
      break;
    }
    if (strlen(accum) + strlen(linebuf) + 1 >= sizeof(accum)) {
      fprintf(stderr, "[LOOP] Baris terlalu panjang, direset.\n");
      accum[0] = 0;
      continue;
    }
    strcat(accum, linebuf);
    int kosong = 1;
    for (char *p = accum; *p; p++) {
      if (!isspace((unsigned char)*p)) {
        kosong = 0;
        break;
      }
    }
    if (kosong) {
      accum[0] = 0;
      continue;
    }
    Lexer *L = calloc(1, sizeof(Lexer));
    if (!L) {
      fprintf(stderr, "[LOOP] Fatal: out of memory\n");
      break;
    }
    L->src = accum;
    tokenisasi(L);
    if (repl_masih_gantung(L->tokens, L->n_tokens)) {
      free(L);
      continue;
    }
    Node *prog = urai_program(L->tokens, L->n_tokens);
    free(L);
    Val *last = NULL;
    NodeType last_type = N_PROGRAM;
    for (int i = 0; i < prog->n_body; i++) {
      if (last)
        val_hapus(last);
      last_type = prog->body[i]->type;
      last = evaluasi(prog->body[i], g_global_env);
    }
    if (last && last->type != VAL_NULL && !repl_node_adalah_stmt(last_type)) {
      char *buf = val_ke_teks(last);
      printf("%s\n", buf);
      free(buf);
    }
    if (last)
      val_hapus(last);
    accum[0] = 0;
  }
  env_hapus(g_global_env);
}

static int loop_main(int argc, char **argv);

/* Interpreter rekursif memakai banyak stack C per pemanggilan fungsi Loop
   (sekitar 30 KB). Dengan stack bawaan 8 MB, rekursi ~250 tingkat sudah
   segfault. Jalankan di thread berstack besar supaya batas MAX_CALL_STACK
   (GalatBatas) yang tercapai lebih dulu, bukan segfault. */
typedef struct {
  int argc;
  char **argv;
  int rc;
} MainArgs;

static void *loop_main_thread(void *p) {
  MainArgs *m = (MainArgs *)p;
  m->rc = loop_main(m->argc, m->argv);
  return NULL;
}

int main(int argc, char **argv) {
  MainArgs m = {argc, argv, 0};
  pthread_attr_t attr;
  pthread_t th;
  if (pthread_attr_init(&attr) == 0 &&
      pthread_attr_setstacksize(&attr, (size_t)768 * 1024 * 1024) == 0 &&
      pthread_create(&th, &attr, loop_main_thread, &m) == 0) {
    pthread_join(th, NULL);
    return m.rc;
  }
  return loop_main(argc, argv); /* gagal membuat thread: jalan di stack utama */
}

static int loop_main(int argc, char **argv) {
  inisialisasi_dirStdlib();
  {
    const char *tj = getenv("LOOP_TIMEOUT_JARINGAN");
    if (tj && atoi(tj) > 0)
      g_timeout_jaringan = atoi(tj);
    const char *mk = getenv("LOOP_MAKS_UKURAN");
    if (mk && mk[0]) {
      char *endp = NULL;
      unsigned long long v = strtoull(mk, &endp, 10);
      if (endp && *endp == 0 && v > 0)
        g_maks_ukuran = (size_t)v;
    }
    /* acakAngka untuk simulasi: di-seed per proses (bukan untuk kripto). */
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    srand((unsigned)(ts.tv_nsec ^ (ts.tv_sec << 8) ^ ((unsigned)getpid() << 16)));
  }
  if (argc < 2) {
    jalankan_repl();
    return 0;
  }
  const char *filepath = NULL;
  int do_nasm = 0;
  int do_debug = 0;
  const char *outfile = NULL;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--nasm") == 0 || strcmp(argv[i], "--kompilasi") == 0) {
      do_nasm = 1;
    } else if (strcmp(argv[i], "--target") == 0 && i + 1 < argc) {
      if (strcmp(argv[i + 1], "nasm") == 0)
        do_nasm = 1;
      i++;
    } else if (strcmp(argv[i], "--debug") == 0) {
      do_debug = 1;
    } else if ((strcmp(argv[i], "-o") == 0 ||
                strcmp(argv[i], "--output") == 0) &&
               i + 1 < argc) {
      outfile = argv[++i];
    } else if (argv[i][0] != '-') {
      if (!filepath)
        filepath = argv[i];
    } else {
      fprintf(stderr,
              "[LOOP] Peringatan: flag tidak dikenal '%s' - diabaikan\n",
              argv[i]);
    }
  }
  if (!filepath) {
    fprintf(stderr, "Error: tidak ada file .lp\n");
    return 1;
  }
  char *kode = baca_file(filepath);
  g_berkas_parse = g_berkas_aktif = intern_berkas(filepath);
  if (do_debug) {
    fprintf(stderr, "[LOOP DEBUG] File    : %s\n", filepath);
    fprintf(stderr, "[LOOP DEBUG] Ukuran  : %d byte\n", (int)strlen(kode));
    fprintf(stderr, "[LOOP DEBUG] Mode    : %s\n",
            do_nasm ? "NASM codegen" : "interpreter");
  }
  Lexer *L = calloc(1, sizeof(Lexer));
  if (!L) {
    fprintf(stderr, "[LOOP] Fatal: out of memory\n");
    free(kode);
    return 1;
  }
  L->src = kode;
  tokenisasi(L);
  if (do_debug) {
    fprintf(stderr, "[LOOP DEBUG] Token   : %d\n", L->n_tokens);
  }
  Node *prog = urai_program(L->tokens, L->n_tokens);
  free(L);
  if (do_debug) {
    fprintf(stderr, "[LOOP DEBUG] Node top-level: %d\n", prog->n_body);
  }
  if (do_nasm) {
    char *nasm = nasm_hasilkan(prog);
    if (outfile) {
      FILE *f = fopen(outfile, "w");
      if (!f) {
        fprintf(stderr, "Tidak bisa buka output: %s\n", outfile);
        free(kode);
        return 1;
      }
      fputs(nasm, f);
      fclose(f);
      fprintf(stderr, "[LOOP] NASM → %s\n", outfile);
      fprintf(stderr, "       Untuk menjalankan:\n");
      fprintf(stderr,
              "       nasm -f elf64 %s -o out.o && ld out.o -o out && ./out\n",
              outfile);
    } else {
      puts(nasm);
    }
    free(nasm);
  } else {
    g_global_env = env_baru(NULL);
    int aan = argc - 1;
    Val **av = malloc(sizeof(Val *) * (size_t)(aan > 0 ? aan : 1));
    for (int i = 1; i < argc; i++)
      av[i - 1] = val_teks(argv[i]);
    Val *argumen_val = val_larik(av, aan);
    env_atur(g_global_env, "argumen", argumen_val);
    val_hapus(argumen_val);
    Val *result = evaluasi(prog, g_global_env);
    if (do_debug) {
      fprintf(stderr, "[LOOP DEBUG] Eksekusi selesai.\n");
    }
    val_hapus(result);
    env_hapus(g_global_env);
  }
  free(kode);
  return 0;
}
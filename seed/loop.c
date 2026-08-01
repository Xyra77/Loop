#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <sys/wait.h>
#include <setjmp.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <regex.h>
#include <sys/utsname.h>
#include <libgen.h>
#include <limits.h>

extern char **environ;

#define LOOP_VERSI        "0.6"
#define MAX_TOKEN         65536
#define MAX_IDENT         256
#define MAX_STRING        4096
#define MAX_PARAMS        32
#define MAX_ARGS          32
#define MAX_ARRAY         4096
#define MAX_ENV_VARS      1024
#define MAX_CALL_STACK    256
#define MAX_GABUNG_ELEMEN 64
#define MAX_INTERP_PARTS  128

static jmp_buf g_repl_jmp;
static int g_in_repl = 0;

static void loop_error(const char *fmt, ...) {
    va_list ap;
    fprintf(stderr, "[LOOP ERROR] ");
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    if (g_in_repl) longjmp(g_repl_jmp, 1);
    exit(1);
}

#define LOOP_ASSERT(cond, ...) do { if (!(cond)) loop_error(__VA_ARGS__); } while(0)
typedef enum {
    TK_EOF = 0,
    /* Literal */
    TK_NUMBER, TK_STRING, TK_BOOL_BENAR, TK_BOOL_SALAH,
    /* Identifier */
    TK_IDENT,
    /* Keyword kontrol */
    TK_FUNGSI, TK_KEMBALI,
    TK_JIKA, TK_MAKA, TK_SELAIN, TK_AKHIR,
    TK_SELAMA, TK_ULANG,
    TK_HENTIKAN, TK_LEWATI,
    TK_PILIH, TK_KASUS,
    TK_COBA, TK_TANGKAP,
    /* Keyword tipe */
    TK_ANGKA, TK_TEKS, TK_BOOL_T, TK_LARIK,
    /* Keyword deklarasi */
    TK_MILIK,
    /* Keyword builtin statement */
    TK_CETAK, TK_JALANKAN,
    TK_BACAFILE, TK_TULISFILE, TK_PAKAI,
    /* Keyword logika */
    TK_DAN, TK_ATAU, TK_TIDAK,
    /* Operator */
    TK_PLUS, TK_MINUS, TK_BINTANG, TK_SLASH, TK_PERSEN,
    TK_PLUSEQ, TK_MINUSEQ, TK_STAREQ, TK_SLASHEQ, TK_PRSNEQ,
    TK_EQ, TK_NEQ, TK_LT, TK_GT, TK_LEQ, TK_GEQ,
    TK_ASSIGN,
    TK_PIPA,   /* |> */
    TK_PANAH,  /* -> */
    /* Tanda */
    TK_LPAREN, TK_RPAREN,
    TK_LBRACE, TK_RBRACE,
    TK_LBRACKET, TK_RBRACKET,
    TK_COMMA,
} TokenType;

typedef struct {
    TokenType type;
    char      val[MAX_STRING];
    int       line;
    double    num;   /* untuk TK_NUMBER */
} Token;

typedef struct {
    const char *src;
    int         pos;
    int         line;
    Token       tokens[MAX_TOKEN];
    int         n_tokens;
} Lexer;

static int is_alpha(char c) { return isalpha((unsigned char)c) || c == '_'; }
static int is_alnum(char c) { return isalnum((unsigned char)c) || c == '_'; }

static void lex_advance(Lexer *L) { if (L->src[L->pos]) L->pos++; }

static void skip_whitespace_comment(Lexer *L) {
    for (;;) {
        while (L->src[L->pos] && strchr(" \t\r", L->src[L->pos])) {
            lex_advance(L);
        }
        if (L->src[L->pos] == '\n') { L->line++; lex_advance(L); continue; }
        if (L->src[L->pos] == '/' && L->src[L->pos+1] == '/') {
            while (L->src[L->pos] && L->src[L->pos] != '\n') lex_advance(L);
            continue;
        }
        if (L->src[L->pos] == '/' && L->src[L->pos+1] == '*') {
            lex_advance(L); lex_advance(L);
            while (L->src[L->pos] && !(L->src[L->pos]=='*' && L->src[L->pos+1]=='/')) {
                if (L->src[L->pos] == '\n') L->line++;
                lex_advance(L);
            }
            if (L->src[L->pos]) { lex_advance(L); lex_advance(L); }
            continue;
        }
        break;
    }
}

typedef struct { const char *kw; TokenType type; } KwMap;
static const KwMap KEYWORDS[] = {
    {"fungsi",    TK_FUNGSI},  {"kembali",  TK_KEMBALI},
    {"jika",      TK_JIKA},    {"maka",     TK_MAKA},
    {"selain",    TK_SELAIN},  {"akhir",    TK_AKHIR},
    {"selama",    TK_SELAMA},  {"ulang",    TK_ULANG},
    {"hentikan",  TK_HENTIKAN},{"lewati",   TK_LEWATI},
    {"pilih",     TK_PILIH},   {"kasus",    TK_KASUS},
    {"coba",      TK_COBA},    {"tangkap",  TK_TANGKAP},
    {"angka",     TK_ANGKA},   {"teks",     TK_TEKS},
    {"bool",      TK_BOOL_T},  {"larik",    TK_LARIK},
    {"milik",     TK_MILIK},
    {"cetak",     TK_CETAK},   {"jalankan", TK_JALANKAN},
    {"bacaFile",  TK_BACAFILE},{"tulisFile", TK_TULISFILE},
    {"pakai",     TK_PAKAI},
    {"dan",       TK_DAN},     {"atau",     TK_ATAU},
    {"tidak",     TK_TIDAK},
    {"benar",     TK_BOOL_BENAR}, {"salah", TK_BOOL_SALAH},
    {NULL, 0}
};

static TokenType kw_lookup(const char *s) {
    for (int i = 0; KEYWORDS[i].kw; i++)
        if (strcmp(s, KEYWORDS[i].kw) == 0) return KEYWORDS[i].type;
        return TK_IDENT;
}

static void tokenize(Lexer *L) {
    L->pos = 0; L->line = 1; L->n_tokens = 0;
    for (;;) {
        skip_whitespace_comment(L);
        char c = L->src[L->pos];
        if (!c) { L->tokens[L->n_tokens++] = (Token){TK_EOF, "", L->line, 0}; break; }

        Token t; t.line = L->line;

        /* Number */
        if (isdigit((unsigned char)c) || (c == '0' && (L->src[L->pos+1]=='x'||L->src[L->pos+1]=='X'))) {
            char buf[64]; int bi = 0;
            if (c == '0' && (L->src[L->pos+1]=='x'||L->src[L->pos+1]=='X')) {
                buf[bi++] = L->src[L->pos++]; buf[bi++] = L->src[L->pos++];
                while (isxdigit((unsigned char)L->src[L->pos])) buf[bi++] = L->src[L->pos++];
            } else {
                while (isdigit((unsigned char)L->src[L->pos])) buf[bi++] = L->src[L->pos++];
                if (L->src[L->pos] == '.' && isdigit((unsigned char)L->src[L->pos+1])) {
                    buf[bi++] = L->src[L->pos++];
                    while (isdigit((unsigned char)L->src[L->pos])) buf[bi++] = L->src[L->pos++];
                }
                if (L->src[L->pos]=='e'||L->src[L->pos]=='E') {
                    buf[bi++] = L->src[L->pos++];
                    if (L->src[L->pos]=='+'||L->src[L->pos]=='-') buf[bi++] = L->src[L->pos++];
                    while (isdigit((unsigned char)L->src[L->pos])) buf[bi++] = L->src[L->pos++];
                }
            }
            buf[bi] = 0;
            t.type = TK_NUMBER; t.num = strtod(buf, NULL); snprintf(t.val, MAX_STRING, "%s", buf);
            L->tokens[L->n_tokens++] = t; continue;
        }

        /* String */
        if (c == '"') {
            lex_advance(L); int bi = 0; t.type = TK_STRING;
            while (L->src[L->pos] && L->src[L->pos] != '"') {
                if (L->src[L->pos] == '\\') {
                    lex_advance(L);
                    char esc = L->src[L->pos];
                    switch (esc) {
                        case 'n': t.val[bi++] = '\n'; break;
                        case 't': t.val[bi++] = '\t'; break;
                        case 'r': t.val[bi++] = '\r'; break;
                        case '"': t.val[bi++] = '"';  break;
                        case '\\':t.val[bi++] = '\\'; break;
                        case '0': t.val[bi++] = '\0'; break;
                        default:  t.val[bi++] = '\\'; t.val[bi++] = esc; break;
                    }
                } else {
                    t.val[bi++] = L->src[L->pos];
                }
                lex_advance(L);
            }
            if (L->src[L->pos] == '"') lex_advance(L);
            t.val[bi] = 0; t.num = 0;
            L->tokens[L->n_tokens++] = t; continue;
        }

        /* Identifier / keyword */
        if (is_alpha(c)) {
            int bi = 0; char buf[MAX_IDENT];
            while (is_alnum(L->src[L->pos])) buf[bi++] = L->src[L->pos++];
            buf[bi] = 0;
            t.type = kw_lookup(buf); snprintf(t.val, MAX_STRING, "%s", buf); t.num = 0;
            L->tokens[L->n_tokens++] = t; continue;
        }

        /* Operator dua karakter */
        lex_advance(L);
        char c2 = L->src[L->pos];
        t.val[0] = c; t.val[1] = 0; t.num = 0;
        #define TWOC(a,b,tp) if (c==(a) && c2==(b)) { t.type=(tp); snprintf(t.val,4,"%c%c",(a),(b)); lex_advance(L); L->tokens[L->n_tokens++]=t; continue; }
        TWOC('|','>',TK_PIPA)
        TWOC('-','>',TK_PANAH)
        TWOC('=','=',TK_EQ)
        TWOC('!','=',TK_NEQ)
        TWOC('<','=',TK_LEQ)
        TWOC('>','=',TK_GEQ)
        TWOC('+','=',TK_PLUSEQ)
        TWOC('-','=',TK_MINUSEQ)
        TWOC('*','=',TK_STAREQ)
        TWOC('/','=',TK_SLASHEQ)
        TWOC('%','=',TK_PRSNEQ)
        #undef TWOC
        /* Operator satu karakter */
        switch (c) {
            case '+': t.type=TK_PLUS;     break;
            case '-': t.type=TK_MINUS;    break;
            case '*': t.type=TK_BINTANG;  break;
            case '/': t.type=TK_SLASH;    break;
            case '%': t.type=TK_PERSEN;   break;
            case '<': t.type=TK_LT;       break;
            case '>': t.type=TK_GT;       break;
            case '=': t.type=TK_ASSIGN;   break;
            case '(': t.type=TK_LPAREN;   break;
            case ')': t.type=TK_RPAREN;   break;
            case '{': t.type=TK_LBRACE;   break;
            case '}': t.type=TK_RBRACE;   break;
            case '[': t.type=TK_LBRACKET; break;
            case ']': t.type=TK_RBRACKET; break;
            case ',': t.type=TK_COMMA;    break;
            default:
                loop_error("Baris %d: Karakter tak dikenal: '%c'", L->line, c);
        }
        L->tokens[L->n_tokens++] = t;
    }
}

typedef enum {
    /* Literal & variabel */
    N_LIT_NUM, N_LIT_STR, N_LIT_BOOL, N_LIT_NULL,
    N_VAR, N_ARRAY,
    N_STRING_INTERP,
    /* Operasi */
    N_BINOP, N_UNIOP, N_INDEX,
    /* Statement */
    N_ASSIGN, N_ASSIGN_COMPOUND,
    N_CETAK, N_JALANKAN,
    N_BACA_FILE, N_TULIS_FILE, N_PAKAI,
    N_JIKA, N_SELAMA, N_ULANG,
    N_PILIH,
    N_COBA,
    N_FUNGSI,
    N_KEMBALI, N_HENTIKAN, N_LEWATI,
    N_PANGGIL,
    N_PIPA,
    N_PROGRAM,
} NodeType;

struct Node;
typedef struct Node Node;

/* Bagian string interpolasi - text sebagai pointer heap, bukan array embedded */
typedef struct {
    int   is_expr;  /* 0 = literal, 1 = ekspresi */
    char *text;     /* heap-allocated via strdup */
    Node *expr;
} InterpPart;

struct Node {
    NodeType type;
    int      line;

    /* Literal */
    double  num;
    char   *str;             /* heap: LIT_STR, VAR, path file */
    int     bval;

    /* Operator - kecil, aman embedded */
    char op[8];

    /* Kiri/kanan/kondisi */
    Node *left, *right, *cond;

    /* Tubuh / selain */
    Node **body;   int n_body;
    Node **selain; int n_selain;

    /* Fungsi / panggil */
    char  *fname;            /* heap */
    char **params;           /* heap: array of char*, hanya node fungsi */
    int    n_params;
    Node **args; int n_args;
    int    is_async;
    int    is_variadic;

    /* Return type annotation */
    char  *ret_type;         /* heap, nullable */

    /* Assign */
    char  *varname;          /* heap */
    char  *var_tipe;         /* heap, nullable */
    int    is_const;

    /* Array / set / tuple elemen */
    Node **elems; int n_elems;

    /* Index */
    Node *obj; Node *idx;

    /* Pilih */
    Node **kasus_val;
    Node **kasus_body;
    int    n_kasus;
    Node  *lainnya;

    /* Coba */
    Node **tangkap;    int n_tangkap;
    char  *tangkap_var; /* heap, nullable */

    /* String interpolasi - alokasi heap hanya untuk node N_STRING_INTERP */
    InterpPart *interp;      /* heap array */
    int         n_interp;

    /* Pipa */
    Node  *pipa_kiri;
    char  *pipa_fn;          /* heap */
    Node **pipa_extra; int n_pipa_extra;

    /* Untuk/dalam */
    char  *untuk_var;        /* heap */
    Node  *untuk_iter;

    /* Impor */
    char  *impor_path;       /* heap */
    char  *impor_alias;      /* heap, nullable */
};


static Node *node_new(NodeType t, int line) {
    Node *n = calloc(1, sizeof(Node));
    if (!n) loop_error("Out of memory");
    n->type = t; n->line = line;
    return n;
}

typedef struct {
    Token *tokens;
    int    pos;
    int    n;
} Parser;

static Token *pcur(Parser *P)  { return &P->tokens[P->pos]; }
static Token *ppeek(Parser *P) { int nx = P->pos+1; return nx<P->n ? &P->tokens[nx] : &P->tokens[P->n-1]; }

static Token *peat(Parser *P, TokenType t) {
    Token *cur = pcur(P);
    if (t && cur->type != t)
        loop_error("Baris %d: Diharapkan token %d, dapat %d ('%s')", cur->line, t, cur->type, cur->val);
    P->pos++;
    return cur;
}

static Node **parse_block_brace(Parser *P, int *out_n);
static Node *parse_stmt(Parser *P);
static Node *parse_expr(Parser *P);
static Node *parse_logika(Parser *P);

/* Forward declarations */
static Node *parse_pipa(Parser *P);
static Node *parse_perbandingan(Parser *P);
static Node *parse_tambah_kurang(Parser *P);
static Node *parse_kali_bagi(Parser *P);
static Node *parse_unary(Parser *P);
static Node *parse_primer(Parser *P);

/* String interpolasi parser - interp parts dialokasi heap */
static void parse_string_interp(Node *n, const char *s) {
    /* Alokasi awal array interp di heap */
    int cap = 8;
    n->interp   = malloc(sizeof(InterpPart) * cap);
    n->n_interp = 0;
    if (!n->interp) loop_error("Out of memory (interp)");

    int i = 0, len = (int)strlen(s);
    char buf[MAX_STRING]; int bi = 0;

    while (i < len) {
        if (s[i] == '#' && i+1 < len && s[i+1] == '{') {
            /* Flush teks literal sebelum ekspresi */
            if (bi > 0) {
                buf[bi] = 0;
                if (n->n_interp >= cap) {
                    cap *= 2;
                    n->interp = realloc(n->interp, sizeof(InterpPart)*cap);
                }
                InterpPart *p = &n->interp[n->n_interp++];
                p->is_expr = 0; p->text = strdup(buf); p->expr = NULL;
                bi = 0;
            }
            /* Baca ekspresi di dalam #{...} */
            i += 2; int depth = 1; int ei = 0; char ebuf[MAX_STRING];
            while (i < len && depth > 0) {
                if      (s[i] == '{') depth++;
                else if (s[i] == '}') { depth--; if (depth == 0) break; }
                if (ei < MAX_STRING-1) ebuf[ei++] = s[i];
                i++;
            }
            ebuf[ei] = 0; if (i < len && s[i] == '}') i++;
            if (n->n_interp >= cap) {
                cap *= 2;
                n->interp = realloc(n->interp, sizeof(InterpPart)*cap);
            }
            InterpPart *p = &n->interp[n->n_interp++];
            p->is_expr = 1; p->text = strdup(ebuf); p->expr = NULL;
        } else {
            if (bi < MAX_STRING-1) buf[bi++] = s[i];
            i++;
        }
    }
    /* Flush sisa teks */
    if (bi > 0) {
        buf[bi] = 0;
        if (n->n_interp >= cap) {
            cap *= 2;
            n->interp = realloc(n->interp, sizeof(InterpPart)*cap);
        }
        InterpPart *p = &n->interp[n->n_interp++];
        p->is_expr = 0; p->text = strdup(buf); p->expr = NULL;
    }
}

static int has_interp(const char *s) { return strstr(s, "#{") != NULL; }

static Node **parse_block_brace(Parser *P, int *out_n) {
    peat(P, TK_LBRACE);
    Node **stmts = NULL; int n = 0, cap = 0;
    while (pcur(P)->type != TK_RBRACE && pcur(P)->type != TK_EOF) {
        if (n >= cap) { cap = cap ? cap*2 : 8; stmts = realloc(stmts, sizeof(Node*)*cap); }
        stmts[n++] = parse_stmt(P);
    }
    peat(P, TK_RBRACE);
    *out_n = n; return stmts;
}

/* Block untuk jika (sampai selain/akhir) */
static Node **parse_jika_block(Parser *P, int *out_n, int *hit_selain) {
    Node **stmts = NULL; int n = 0, cap = 0;
    while (pcur(P)->type != TK_EOF) {
        TokenType t = pcur(P)->type;
        if (t == TK_AKHIR || t == TK_SELAIN) break;
        if (n >= cap) { cap = cap ? cap*2 : 8; stmts = realloc(stmts, sizeof(Node*)*cap); }
        stmts[n++] = parse_stmt(P);
    }
    *hit_selain = (pcur(P)->type == TK_SELAIN);
    *out_n = n; return stmts;
}

static Node *parse_stmt(Parser *P) {
    Token *cur = pcur(P);
    int line = cur->line;

    /* Tipe eksplisit: angka x = ... */
    if (cur->type == TK_ANGKA || cur->type == TK_TEKS ||
        cur->type == TK_BOOL_T || cur->type == TK_LARIK) {
        char tipe[MAX_IDENT]; snprintf(tipe, MAX_IDENT, "%s", cur->val);
    peat(P, cur->type);
    Token *id = peat(P, TK_IDENT);
    peat(P, TK_ASSIGN);
    Node *val = parse_expr(P);
    Node *n = node_new(N_ASSIGN, line);
    n->varname = strdup(id->val);
    n->var_tipe = strdup(tipe);
    n->right = val; return n;
        }

        /* Deklarasi: milik nama = nilai */
        if (cur->type == TK_MILIK) {
            peat(P, TK_MILIK);
            Token *id = peat(P, TK_IDENT);
            peat(P, TK_ASSIGN);
            Node *val = parse_expr(P);
            Node *n = node_new(N_ASSIGN, line);
            n->varname = strdup(id->val);
            n->right = val; return n;
        }

        switch (cur->type) {
            /* cetak */
            case TK_CETAK: {
                peat(P, TK_CETAK);
                Node *n = node_new(N_CETAK, line);
                n->left = parse_expr(P); return n;
            }

            /* jalankan */
            case TK_JALANKAN: {
                peat(P, TK_JALANKAN);
                Node *n = node_new(N_JALANKAN, line);
                n->left = parse_expr(P); return n;
            }

            /* bacaFile(path) — EKSEKUSI file sebagai kode Loop (import-like), bukan baca sebagai teks
             * Gunakan bacaFileTeks(path) untuk membaca file sebagai string teks */
            case TK_BACAFILE: {
                peat(P, TK_BACAFILE);
                peat(P, TK_LPAREN);
                Token *path = peat(P, TK_STRING);
                peat(P, TK_RPAREN);
                Node *n = node_new(N_BACA_FILE, line);
                n->str = strdup(path->val); return n;
            }

            /* pakai nama — impor modul standard library bawaan (tanpa path/ekstensi),
             * dicari otomatis di folder stdlib Loop. Beda dari bacaFile(path) yang
             * butuh path lengkap ke file .lp manapun. */
            case TK_PAKAI: {
                peat(P, TK_PAKAI);
                Token *nm = peat(P, TK_IDENT);
                Node *n = node_new(N_PAKAI, line);
                n->str = strdup(nm->val); return n;
            }

            /* tulisFile(path, isi) */
            case TK_TULISFILE: {
                peat(P, TK_TULISFILE);
                peat(P, TK_LPAREN);
                Token *path = peat(P, TK_STRING);
                peat(P, TK_COMMA);
                Node *isi = parse_expr(P);
                peat(P, TK_RPAREN);
                Node *n = node_new(N_TULIS_FILE, line);
                n->str = strdup(path->val);
                n->left = isi; return n;
            }

            /* fungsi nama(params) { ... } */
            case TK_FUNGSI: {
                peat(P, TK_FUNGSI);
                Token *nm = peat(P, TK_IDENT);
                peat(P, TK_LPAREN);
                Node *n = node_new(N_FUNGSI, line);
                n->fname    = strdup(nm->val);
                n->n_params = 0;
                int params_cap = 0;
                n->params   = NULL;
                while (pcur(P)->type != TK_RPAREN && pcur(P)->type != TK_EOF) {
                    if (n->n_params >= params_cap) {
                        params_cap = params_cap ? params_cap*2 : 4;
                        n->params  = realloc(n->params, sizeof(char*)*params_cap);
                    }
                    n->params[n->n_params++] = strdup(peat(P, TK_IDENT)->val);
                    if (pcur(P)->type == TK_COMMA) peat(P, TK_COMMA);
                }
                peat(P, TK_RPAREN);
                n->body = parse_block_brace(P, &n->n_body); return n;
            }

            /* kembali ekspresi */
            case TK_KEMBALI: {
                peat(P, TK_KEMBALI);
                Node *n = node_new(N_KEMBALI, line);
                n->left = parse_expr(P); return n;
            }

            /* hentikan / lewati */
            case TK_HENTIKAN: peat(P, TK_HENTIKAN); return node_new(N_HENTIKAN, line);
            case TK_LEWATI:   peat(P, TK_LEWATI);   return node_new(N_LEWATI, line);

            /* jika kondisi maka ... [selain ...] akhir */
            case TK_JIKA: {
                peat(P, TK_JIKA);
                Node *cond = parse_expr(P);
                peat(P, TK_MAKA);
                Node *n = node_new(N_JIKA, line);
                n->cond = cond;
                int hit_selain = 0;
                n->body = parse_jika_block(P, &n->n_body, &hit_selain);
                if (hit_selain) {
                    peat(P, TK_SELAIN);
                    int dummy = 0;
                    n->selain = parse_jika_block(P, &n->n_selain, &dummy);
                }
                peat(P, TK_AKHIR);
                return n;
            }

            /* selama kondisi { ... } */
            case TK_SELAMA: {
                peat(P, TK_SELAMA);
                Node *cond = parse_expr(P);
                Node *n = node_new(N_SELAMA, line);
                n->cond = cond;
                n->body = parse_block_brace(P, &n->n_body); return n;
            }

            /* ulang N { ... } */
            case TK_ULANG: {
                peat(P, TK_ULANG);
                Node *hitungan = parse_expr(P);
                Node *n = node_new(N_ULANG, line);
                n->left = hitungan;
                n->body = parse_block_brace(P, &n->n_body); return n;
            }

            /* pilih ekspresi { kasus val -> stmt ... selain -> stmt } */
            case TK_PILIH: {
                peat(P, TK_PILIH);
                Node *eksp = parse_expr(P);
                Node *n = node_new(N_PILIH, line);
                n->left = eksp; n->n_kasus = 0; n->lainnya = NULL;
                n->kasus_val  = calloc(MAX_ARGS, sizeof(Node*));
                n->kasus_body = calloc(MAX_ARGS, sizeof(Node*));
                peat(P, TK_LBRACE);
                while (pcur(P)->type != TK_RBRACE && pcur(P)->type != TK_EOF) {
                    if (pcur(P)->type == TK_KASUS) {
                        peat(P, TK_KASUS);
                        n->kasus_val[n->n_kasus] = parse_expr(P);
                        peat(P, TK_PANAH);
                        n->kasus_body[n->n_kasus] = parse_stmt(P);
                        n->n_kasus++;
                    } else if (pcur(P)->type == TK_SELAIN) {
                        peat(P, TK_SELAIN);
                        peat(P, TK_PANAH);
                        n->lainnya = parse_stmt(P);
                    } else { peat(P, 0); } /* skip */
                }
                peat(P, TK_RBRACE); return n;
            }

            /* coba { ... } tangkap { ... } */
            case TK_COBA: {
                peat(P, TK_COBA);
                Node *n = node_new(N_COBA, line);
                n->body = parse_block_brace(P, &n->n_body);
                peat(P, TK_TANGKAP);
                n->tangkap = parse_block_brace(P, &n->n_tangkap);
                return n;
            }

            default: break;
        }

        /* Assign biasa / compound / ekspresi */
        if (cur->type == TK_IDENT) {
            /* Cek compound assign: x += ... */
            TokenType nxt = ppeek(P)->type;
            if (nxt == TK_PLUSEQ || nxt == TK_MINUSEQ ||
                nxt == TK_STAREQ || nxt == TK_SLASHEQ || nxt == TK_PRSNEQ) {
                Token *id = peat(P, TK_IDENT);
            Token *op = peat(P, nxt);
            Node *val = parse_expr(P);
            Node *n = node_new(N_ASSIGN_COMPOUND, line);
            n->varname = strdup(id->val);
            strncpy(n->op, op->val, 8);
            n->right = val; return n;
                }
                /* Assign biasa: x = ... */
                if (nxt == TK_ASSIGN) {
                    Token *id = peat(P, TK_IDENT);
                    peat(P, TK_ASSIGN);
                    Node *val = parse_expr(P);
                    Node *n = node_new(N_ASSIGN, line);
                    n->varname = strdup(id->val);
                    n->right = val; return n;
                }
        }

        return parse_expr(P);
}

static Node *parse_expr(Parser *P) { return parse_pipa(P); }

static Node *parse_pipa(Parser *P) {
    Node *kiri = parse_logika(P);
    while (pcur(P)->type == TK_PIPA) {
        int line = pcur(P)->line;
        peat(P, TK_PIPA);
        Token *fn = peat(P, TK_IDENT);
        Node *n = node_new(N_PIPA, line);
        n->pipa_kiri = kiri;
        n->pipa_fn = strdup(fn->val);
        n->pipa_extra = NULL; n->n_pipa_extra = 0;
        if (pcur(P)->type == TK_LPAREN) {
            peat(P, TK_LPAREN);
            int cap = 0;
            while (pcur(P)->type != TK_RPAREN && pcur(P)->type != TK_EOF) {
                if (n->n_pipa_extra >= cap) { cap = cap ? cap*2 : 4; n->pipa_extra = realloc(n->pipa_extra, sizeof(Node*)*cap); }
                n->pipa_extra[n->n_pipa_extra++] = parse_expr(P);
                if (pcur(P)->type == TK_COMMA) peat(P, TK_COMMA);
            }
            peat(P, TK_RPAREN);
        }
        kiri = n;
    }
    return kiri;
}

static Node *parse_logika(Parser *P) {
    Node *kiri = parse_perbandingan(P);
    while (pcur(P)->type == TK_DAN || pcur(P)->type == TK_ATAU) {
        Token *op = peat(P, pcur(P)->type);
        Node *n = node_new(N_BINOP, op->line);
        strncpy(n->op, op->val, 8);
        n->left = kiri; n->right = parse_perbandingan(P);
        kiri = n;
    }
    return kiri;
}

static Node *parse_perbandingan(Parser *P) {
    Node *kiri = parse_tambah_kurang(P);
    while (pcur(P)->type==TK_EQ||pcur(P)->type==TK_NEQ||
        pcur(P)->type==TK_LT||pcur(P)->type==TK_GT||
        pcur(P)->type==TK_LEQ||pcur(P)->type==TK_GEQ) {
        Token *op = peat(P, pcur(P)->type);
    Node *n = node_new(N_BINOP, op->line);
    strncpy(n->op, op->val, 8);
    n->left = kiri; n->right = parse_tambah_kurang(P);
    kiri = n;
        }
        return kiri;
}

static Node *parse_tambah_kurang(Parser *P) {
    Node *kiri = parse_kali_bagi(P);
    while (pcur(P)->type==TK_PLUS||pcur(P)->type==TK_MINUS) {
        Token *op = peat(P, pcur(P)->type);
        Node *n = node_new(N_BINOP, op->line);
        strncpy(n->op, op->val, 8);
        n->left = kiri; n->right = parse_kali_bagi(P);
        kiri = n;
    }
    return kiri;
}

static Node *parse_kali_bagi(Parser *P) {
    Node *kiri = parse_unary(P);
    while (pcur(P)->type==TK_BINTANG||pcur(P)->type==TK_SLASH||pcur(P)->type==TK_PERSEN) {
        Token *op = peat(P, pcur(P)->type);
        Node *n = node_new(N_BINOP, op->line);
        strncpy(n->op, op->val, 8);
        n->left = kiri; n->right = parse_unary(P);
        kiri = n;
    }
    return kiri;
}

static Node *parse_unary(Parser *P) {
    if (pcur(P)->type == TK_TIDAK) {
        Token *op = peat(P, TK_TIDAK);
        Node *n = node_new(N_UNIOP, op->line);
        strncpy(n->op, "tidak", 8);
        n->left = parse_unary(P); return n;
    }
    if (pcur(P)->type == TK_MINUS) {
        Token *op = peat(P, TK_MINUS);
        Node *n = node_new(N_UNIOP, op->line);
        strncpy(n->op, "-", 8);
        n->left = parse_unary(P); return n;
    }
    return parse_primer(P);
}

static Node *parse_primer(Parser *P) {
    Token *cur = pcur(P);
    int line = cur->line;

    if (cur->type == TK_NUMBER) {
        peat(P, TK_NUMBER);
        Node *n = node_new(N_LIT_NUM, line);
        n->num = cur->num; return n;
    }
    if (cur->type == TK_STRING) {
        peat(P, TK_STRING);
        if (has_interp(cur->val)) {
            Node *n = node_new(N_STRING_INTERP, line);
            parse_string_interp(n, cur->val); return n;
        }
        Node *n = node_new(N_LIT_STR, line);
        n->str = strdup(cur->val); return n;
    }
    if (cur->type == TK_BOOL_BENAR) { peat(P, TK_BOOL_BENAR); Node *n = node_new(N_LIT_BOOL, line); n->bval=1; return n; }
    if (cur->type == TK_BOOL_SALAH) { peat(P, TK_BOOL_SALAH); Node *n = node_new(N_LIT_BOOL, line); n->bval=0; return n; }

    if (cur->type == TK_LBRACKET) {
        peat(P, TK_LBRACKET);
        Node *n = node_new(N_ARRAY, line);
        n->elems = NULL; n->n_elems = 0; int cap = 0;
        while (pcur(P)->type != TK_RBRACKET && pcur(P)->type != TK_EOF) {
            if (n->n_elems >= cap) { cap = cap ? cap*2 : 8; n->elems = realloc(n->elems, sizeof(Node*)*cap); }
            n->elems[n->n_elems++] = parse_expr(P);
            if (pcur(P)->type == TK_COMMA) peat(P, TK_COMMA);
        }
        peat(P, TK_RBRACKET);
        /* index chain: arr[i] */
        Node *base = n;
        while (pcur(P)->type == TK_LBRACKET) {
            peat(P, TK_LBRACKET);
            Node *idx = parse_expr(P);
            peat(P, TK_RBRACKET);
            Node *in = node_new(N_INDEX, line);
            in->obj = base; in->idx = idx; base = in;
        }
        return base;
    }

    if (cur->type == TK_LPAREN) {
        peat(P, TK_LPAREN);
        Node *n = parse_expr(P);
        peat(P, TK_RPAREN); return n;
    }

    if (cur->type == TK_IDENT) {
        Token *id = peat(P, TK_IDENT);
        if (pcur(P)->type == TK_LPAREN) {
            /* Pemanggilan fungsi */
            peat(P, TK_LPAREN);
            Node *n = node_new(N_PANGGIL, line);
            n->fname = strdup(id->val);
            n->args = NULL; n->n_args = 0; int cap = 0;
            while (pcur(P)->type != TK_RPAREN && pcur(P)->type != TK_EOF) {
                if (n->n_args >= cap) { cap = cap ? cap*2 : 4; n->args = realloc(n->args, sizeof(Node*)*cap); }
                n->args[n->n_args++] = parse_expr(P);
                if (pcur(P)->type == TK_COMMA) peat(P, TK_COMMA);
            }
            peat(P, TK_RPAREN);
            Node *base = n;
            while (pcur(P)->type == TK_LBRACKET) {
                peat(P, TK_LBRACKET);
                Node *idx = parse_expr(P);
                peat(P, TK_RBRACKET);
                Node *in = node_new(N_INDEX, line);
                in->obj = base; in->idx = idx; base = in;
            }
            return base;
        }
        Node *n = node_new(N_VAR, line);
        n->str = strdup(id->val);
        /* index chain: var[i] */
        Node *base = n;
        while (pcur(P)->type == TK_LBRACKET) {
            peat(P, TK_LBRACKET);
            Node *idx = parse_expr(P);
            peat(P, TK_RBRACKET);
            Node *in = node_new(N_INDEX, line);
            in->obj = base; in->idx = idx; base = in;
        }
        return base;
    }

    loop_error("Baris %d: Token tak terduga: '%s'", line, cur->val);
    return NULL;
}

static Node *parse_program(Token *tokens, int n_tokens) {
    Parser P; P.tokens = tokens; P.pos = 0; P.n = n_tokens;
    Node *prog = node_new(N_PROGRAM, 1);
    prog->body = NULL; prog->n_body = 0; int cap = 0;
    while (pcur(&P)->type != TK_EOF) {
        if (prog->n_body >= cap) { cap = cap ? cap*2 : 16; prog->body = realloc(prog->body, sizeof(Node*)*cap); }
        prog->body[prog->n_body++] = parse_stmt(&P);
    }
    return prog;
}

typedef enum {
    VAL_NULL = 0,
    VAL_NUM,      /* double */
    VAL_STR,      /* heap string */
    VAL_BOOL,
    VAL_ARRAY,    /* heap array of Val* */
    VAL_FUNC,     /* user-defined function node */
    VAL_PRIMA,    /* kunci prima */
    VAL_ENKRIPSI, /* hasil acak */
    VAL_GANDA,    /* dual state */
    VAL_KONEKSI,  /* soket jaringan, TCP polos atau TLS */
} ValType;

typedef struct Val Val;
struct Val {
    ValType type;
    int     refs;
    /* num */
    double  num;
    /* str */
    char   *str;
    /* bool */
    int     bval;
    /* array */
    Val   **arr;
    int     arr_n;
    /* func */
    Node   *func_node;
    /* prima */
    uint64_t prima_kunci;
    /* enkripsi */
    unsigned char *enc_data;
    int            enc_len;
    char          *enc_hex;
    uint64_t       enc_seed;
    /* ganda */
    double   ganda_asli;
    char    *ganda_enc;
    /* koneksi (jaringan) */
    int      kon_fd;     /* file descriptor soket, -1 kalau tertutup */
    int      kon_tls;    /* 1 kalau pakai TLS */
    void    *kon_ssl;    /* SSL* kalau kon_tls */
    void    *kon_ctx;    /* SSL_CTX* kalau kon_tls */
    char    *kon_host;   /* host tujuan, buat info/format */
    int      kon_port;
};

static Val *val_null(void) {
    Val *v = calloc(1, sizeof(Val)); v->type = VAL_NULL; v->refs = 1; return v;
}
static Val *val_num(double n) {
    Val *v = calloc(1, sizeof(Val)); v->type = VAL_NUM; v->num = n; v->refs = 1; return v;
}
static Val *val_bool(int b) {
    Val *v = calloc(1, sizeof(Val)); v->type = VAL_BOOL; v->bval = b; v->refs = 1; return v;
}
static Val *val_str(const char *s) {
    Val *v = calloc(1, sizeof(Val)); v->type = VAL_STR; v->str = strdup(s); v->refs = 1; return v;
}
static Val *val_array(Val **arr, int n) {
    Val *v = calloc(1, sizeof(Val)); v->type = VAL_ARRAY; v->arr = arr; v->arr_n = n; v->refs = 1; return v;
}
static Val *val_func(Node *fn) {
    Val *v = calloc(1, sizeof(Val)); v->type = VAL_FUNC; v->func_node = fn; v->refs = 1; return v;
}

static void val_free(Val *v) {
    if (!v) return;
    if (--v->refs > 0) return;
    if (v->str)      free(v->str);
    if (v->arr)      { for (int i=0;i<v->arr_n;i++) val_free(v->arr[i]); free(v->arr); }
    if (v->enc_data) free(v->enc_data);
    if (v->enc_hex)  free(v->enc_hex);
    if (v->ganda_enc)free(v->ganda_enc);
    if (v->type == VAL_KONEKSI) {
        if (v->kon_ssl) { SSL_shutdown((SSL*)v->kon_ssl); SSL_free((SSL*)v->kon_ssl); }
        if (v->kon_ctx) SSL_CTX_free((SSL_CTX*)v->kon_ctx);
        if (v->kon_fd >= 0) close(v->kon_fd);
        if (v->kon_host) free(v->kon_host);
    }
    free(v);
}

static Val *val_ref(Val *v) { if (v) v->refs++; return v; }

/* Format nilai ke string untuk cetak */
static void val_format(Val *v, char *buf, int bufsz) {
    if (!v) { snprintf(buf, bufsz, "kosong"); return; }
    switch (v->type) {
        case VAL_NULL:  snprintf(buf, bufsz, "kosong"); break;
        case VAL_BOOL:  snprintf(buf, bufsz, "%s", v->bval ? "benar" : "salah"); break;
        case VAL_NUM: {
            double n = v->num;
            if (n == (int64_t)n) snprintf(buf, bufsz, "%lld", (long long)(int64_t)n);
            else snprintf(buf, bufsz, "%g", n);
            break;
        }
        case VAL_STR:   snprintf(buf, bufsz, "%s", v->str ? v->str : ""); break;
        case VAL_FUNC:  snprintf(buf, bufsz, "<fungsi %s>", v->func_node ? v->func_node->fname : "?"); break;
        case VAL_ARRAY: {
            int off = 0; off += snprintf(buf+off, bufsz-off, "[");
            for (int i = 0; i < v->arr_n && off < bufsz-4; i++) {
                if (i) off += snprintf(buf+off, bufsz-off, ", ");
                char tmp[256]; val_format(v->arr[i], tmp, 256);
                off += snprintf(buf+off, bufsz-off, "%s", tmp);
            }
            snprintf(buf+off, bufsz-off, "]"); break;
        }
        case VAL_PRIMA:
            snprintf(buf, bufsz, "[KunciPrima | kunci=%llu]", (unsigned long long)v->prima_kunci); break;
        case VAL_ENKRIPSI:
            snprintf(buf, bufsz, "[Terenkripsi | hex=%s | seed=%llu]",
                     v->enc_hex ? v->enc_hex : "?", (unsigned long long)v->enc_seed); break;
        case VAL_GANDA:
            snprintf(buf, bufsz, "[Ganda | asli=%g | enc=%s]", v->ganda_asli, v->ganda_enc ? v->ganda_enc : "?"); break;
        case VAL_KONEKSI:
            snprintf(buf, bufsz, "[Koneksi | %s:%d | %s | %s]",
                     v->kon_host ? v->kon_host : "?", v->kon_port,
                     v->kon_tls ? "tls" : "tcp",
                     v->kon_fd >= 0 ? "terbuka" : "tertutup"); break;
        default: snprintf(buf, bufsz, "?"); break;
    }
}

typedef struct Env Env;
struct Env {
    char  names[MAX_ENV_VARS][MAX_IDENT];
    Val  *vals[MAX_ENV_VARS];
    int   n;
    Env  *parent;
};

static Env *env_new(Env *parent) {
    Env *e = calloc(1, sizeof(Env));
    e->parent = parent; e->n = 0; return e;
}

static Val *env_get(Env *e, const char *name) {
    for (; e; e = e->parent)
        for (int i = e->n-1; i >= 0; i--)
            if (strcmp(e->names[i], name) == 0) return e->vals[i];
            loop_error("Variabel tidak ditemukan: '%s'", name);
    return NULL;
}

static void env_set(Env *e, const char *name, Val *v) {
    /* Update jika sudah ada di scope manapun */
    for (Env *cur = e; cur; cur = cur->parent)
        for (int i = cur->n-1; i >= 0; i--)
            if (strcmp(cur->names[i], name) == 0) {
                val_free(cur->vals[i]);
                cur->vals[i] = val_ref(v);
                return;
            }
            /* Deklarasi baru */
            LOOP_ASSERT(e->n < MAX_ENV_VARS, "Terlalu banyak variabel");
        strncpy(e->names[e->n], name, MAX_IDENT);
    e->vals[e->n] = val_ref(v);
    e->n++;
}

static void env_local_set(Env *e, const char *name, Val *v) {
    for (int i = e->n-1; i >= 0; i--)
        if (strcmp(e->names[i], name) == 0) {
            val_free(e->vals[i]); e->vals[i] = val_ref(v); return;
        }
        LOOP_ASSERT(e->n < MAX_ENV_VARS, "Terlalu banyak variabel lokal");
    strncpy(e->names[e->n], name, MAX_IDENT);
    e->vals[e->n] = val_ref(v); e->n++;
}

static void env_free(Env *e) {
    for (int i = 0; i < e->n; i++) val_free(e->vals[i]);
    free(e);
}

/* ---------------------------------------------------------------
 * BILANGAN PRIMA (utilitas)
 * --------------------------------------------------------------- */

static int is_prima(uint64_t n) {
    if (n < 2) return 0;
    if (n == 2) return 1;
    if (n % 2 == 0) return 0;
    for (uint64_t i = 3; i * i <= n; i += 2)
        if (n % i == 0) return 0;
        return 1;
}

static uint64_t next_prima(uint64_t n) {
    n++;
    while (!is_prima(n)) n++;
    return n;
}

static int jumlah_faktor_prima(uint64_t n) {
    int total = 0;
    if (n < 2) return 0;
    for (uint64_t p = 2; p * p <= n; p++) {
        while (n % p == 0) { total++; n /= p; }
    }
    if (n > 1) total++;
    return total;
}

static uint64_t hitung_lapisan(uint64_t n, uint64_t sebelumnya, int is_first) {
    if (n < 2) n = 2;
    int jf = jumlah_faktor_prima(n);
    uint64_t raw = (uint64_t)jf * n;
    uint64_t mod;
    if (is_first) {
        mod = next_prima(n);
        uint64_t hasil = raw % mod;
        if (hasil == 0) hasil = (uint64_t)jf * mod;
        return hasil;
    } else {
        mod = sebelumnya > 1 ? sebelumnya : 2;
        uint64_t hasil = raw % mod;
        if (hasil == 0) hasil = (uint64_t)jf * next_prima(mod);
        return hasil;
    }
}

/* XOR bytes dengan kunci berulang */
static void xor_bytes(unsigned char *dst, const unsigned char *src,
                      int src_len, const unsigned char *key, int key_len) {
    for (int i = 0; i < src_len; i++)
        dst[i] = src[i] ^ key[i % key_len];
                      }

                      /* Fisher-Yates shuffle deterministik */
                      static void shuffle_bytes(unsigned char *data, int len, uint64_t seed) {
                          for (int i = len-1; i > 0; i--) {
                              seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
                              int j = (int)(seed >> 33) % (i+1);
                              unsigned char tmp = data[i]; data[i] = data[j]; data[j] = tmp;
                          }
                      }

                      /* Konversi bytes ke hex string */
                      static char *bytes_to_hex(const unsigned char *data, int len) {
                          char *hex = malloc(len*2+1);
                          for (int i = 0; i < len; i++)
                              snprintf(hex+i*2, 3, "%02x", data[i]);
                          hex[len*2] = 0;
                          return hex;
                      }

                      /* Exception flags */
                      static int  g_kembali   = 0;
                      static Val *g_kembali_v = NULL;
                      static int  g_hentikan  = 0;
                      static int  g_lewati    = 0;
                      static int  g_error     = 0;  /* untuk coba/tangkap */
                      static char g_error_msg[MAX_STRING] = {0};

                      /* Standard library bawaan (untuk keyword 'pakai') */
                      static char g_stdlib_dir[PATH_MAX] = {0};
                      static char *g_pakai_loaded[64];
                      static int   g_pakai_n_loaded = 0;

                      /* Cari folder stdlib: env var LOOP_STDLIB kalau di-set, kalau enggak
                       * pakai "<folder_binary>/Library" (jadi stdlib ikut ke mana pun binary
                       * loop dibawa/diinstall, gak tergantung cwd tempat dipanggil). */
                      static void init_stdlib_dir(void) {
                          const char *override = getenv("LOOP_STDLIB");
                          if (override && override[0]) {
                              strncpy(g_stdlib_dir, override, PATH_MAX - 1);
                              return;
                          }
                          char exe[PATH_MAX] = {0};
                          ssize_t len = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
                          if (len <= 0) {
                              /* Fallback: relatif ke cwd kalau /proc/self/exe gagal (mis. non-Linux) */
                              strncpy(g_stdlib_dir, "Library", PATH_MAX - 1);
                              return;
                          }
                          exe[len] = 0;
                          char *dir = dirname(exe); /* boleh modif buffer sendiri */
                          snprintf(g_stdlib_dir, PATH_MAX, "%s/Library", dir);
                      }

                      static Val *eval(Node *n, Env *env);

                      /* Helper: jalankan kode string baru (untuk bacaFile & string interp) */
                      static Val *run_kode(const char *kode, Env *env);

                      /* Global env untuk fungsi user-defined (dideklarasikan di sini biar builtin
                       *  higher-order seperti peta()/saring()/kurangi() bisa memanggil balik) */
                      static Env *g_global_env = NULL;

                      /* Panggil sebuah Val bertipe VAL_FUNC dengan argumen yang sudah dievaluasi.
                       *  Dipakai oleh builtin higher-order (peta, saring, kurangi). Didefinisikan
                       *  di dekat eval_panggil (butuh eval() penuh), dipakai lebih awal via
                       *  forward declaration ini. */
                      static Val *panggil_nilai_fungsi(Val *fv, Val **args, int n_args, int line);

                      /* Builtins */
                      typedef Val *(*BuiltinFn)(Val **args, int n_args, int line);

                      static Val *builtin_panjang(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1, "panjang() butuh 1 argumen");
                          if (args[0]->type == VAL_STR)   return val_num((double)strlen(args[0]->str));
                          if (args[0]->type == VAL_ARRAY) return val_num((double)args[0]->arr_n);
                          loop_error("Baris %d: panjang() tidak bisa untuk tipe ini", line); return NULL;
                      }

                      static Val *builtin_jumlah(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1 && args[0]->type==VAL_ARRAY, "jumlah() butuh 1 larik");
                          double s = 0;
                          for (int i=0;i<args[0]->arr_n;i++) s += args[0]->arr[i]->num;
                          return val_num(s);
                      }

                      static Val *builtin_maks(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1 && args[0]->type==VAL_ARRAY && args[0]->arr_n>0, "maks() butuh larik isi");
                          double m = args[0]->arr[0]->num;
                          for (int i=1;i<args[0]->arr_n;i++) if (args[0]->arr[i]->num > m) m = args[0]->arr[i]->num;
                          return val_num(m);
                      }

                      static Val *builtin_min(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1 && args[0]->type==VAL_ARRAY && args[0]->arr_n>0, "min() butuh larik isi");
                          double m = args[0]->arr[0]->num;
                          for (int i=1;i<args[0]->arr_n;i++) if (args[0]->arr[i]->num < m) m = args[0]->arr[i]->num;
                          return val_num(m);
                      }

                      static Val *builtin_rerata(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1 && args[0]->type==VAL_ARRAY && args[0]->arr_n>0, "rerata() butuh larik isi");
                          double s = 0;
                          for (int i=0;i<args[0]->arr_n;i++) s += args[0]->arr[i]->num;
                          return val_num(s / args[0]->arr_n);
                      }

                      /* Merge sort */
                      static int cmp_val(const void *a, const void *b) {
                          Val *va = *(Val**)a, *vb = *(Val**)b;
                          if (va->num < vb->num) return -1;
                          if (va->num > vb->num) return 1;
                          return 0;
                      }

                      static Val *builtin_urut(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1 && args[0]->type==VAL_ARRAY, "urut() butuh larik");
                          Val **copy = malloc(sizeof(Val*)*args[0]->arr_n);
                          for (int i=0;i<args[0]->arr_n;i++) { copy[i] = args[0]->arr[i]; copy[i]->refs++; }
                          qsort(copy, args[0]->arr_n, sizeof(Val*), cmp_val);
                          return val_array(copy, args[0]->arr_n);
                      }

                      static Val *builtin_balik(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1 && args[0]->type==VAL_ARRAY, "balik() butuh larik");
                          int sz = args[0]->arr_n;
                          Val **copy = malloc(sizeof(Val*)*sz);
                          for (int i=0;i<sz;i++) { copy[i] = args[0]->arr[sz-1-i]; copy[i]->refs++; }
                          return val_array(copy, sz);
                      }

                      static Val *builtin_angkaTeks(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1, "angkaTeks() butuh 1 argumen");
                          char buf[64];
                          double v = args[0]->num;
                          if (v == (int64_t)v) snprintf(buf, 64, "%lld", (long long)(int64_t)v);
                          else snprintf(buf, 64, "%g", v);
                          return val_str(buf);
                      }

                      static Val *builtin_teksAngka(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1 && args[0]->type==VAL_STR, "teksAngka() butuh teks");
                          return val_num(atof(args[0]->str));
                      }

                      static Val *builtin_masukan(Val **args, int n, int line) {
                          if (n > 0 && args[0]->type == VAL_STR) printf("%s", args[0]->str);
                          char buf[4096]; if (!fgets(buf, 4096, stdin)) buf[0] = 0;
                          int len = strlen(buf); if (len > 0 && buf[len-1]=='\n') buf[len-1]=0;
                          return val_str(buf);
                      }

                      static Val *builtin_tipe(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1, "tipe() butuh 1 argumen");
                          switch (args[0]->type) {
                              case VAL_NUM:     return val_str("angka");
                              case VAL_STR:     return val_str("teks");
                              case VAL_BOOL:    return val_str("bool");
                              case VAL_ARRAY:   return val_str("larik");
                              case VAL_FUNC:    return val_str("fungsi");
                              case VAL_PRIMA:   return val_str("kunci_prima");
                              case VAL_ENKRIPSI:return val_str("terenkripsi");
                              case VAL_GANDA:   return val_str("ganda");
                              case VAL_KONEKSI: return val_str("koneksi");
                              default:          return val_str("kosong");
                          }
                      }

                      static Val *builtin_tambah(Val **args, int n, int line) {
                          LOOP_ASSERT(n==2, "tambah() butuh 2 argumen");
                          Val *a = args[0], *b = args[1];
                          if (a->type==VAL_STR && b->type==VAL_STR) {
                              char *s = malloc(strlen(a->str)+strlen(b->str)+1);
                              strcpy(s, a->str); strcat(s, b->str);
                              Val *r = val_str(s); free(s); return r;
                          }
                          if (a->type==VAL_NUM && b->type==VAL_NUM) return val_num(a->num + b->num);
                          if (a->type==VAL_ARRAY) {
                              int sz = a->arr_n+1;
                              Val **arr = malloc(sizeof(Val*)*sz);
                              for (int i=0;i<a->arr_n;i++) { arr[i] = a->arr[i]; arr[i]->refs++; }
                              arr[a->arr_n] = val_ref(b);
                              return val_array(arr, sz);
                          }
                          if (a->type==VAL_PRIMA && b->type==VAL_PRIMA) {
                              Val *r = calloc(1,sizeof(Val)); r->type=VAL_PRIMA; r->refs=1;
                              r->prima_kunci = a->prima_kunci ^ b->prima_kunci; return r;
                          }
                          /* Fallback: string concat */
                          char ba[512], bb[512];
                          val_format(a, ba, 512); val_format(b, bb, 512);
                          char *s = malloc(strlen(ba)+strlen(bb)+1);
                          strcpy(s, ba); strcat(s, bb);
                          Val *r = val_str(s); free(s); return r;
                      }

                      static Val *builtin_kodeKarakter(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1 && args[0]->type==VAL_STR, "kodeKarakter() butuh teks");
                          unsigned char c = (args[0]->str && args[0]->str[0]) ? (unsigned char)args[0]->str[0] : 0;
                          return val_num((double)c);
                      }

                      static Val *builtin_karakter(Val **args, int n, int line) {
                          LOOP_ASSERT(n==2 && args[0]->type==VAL_STR && args[1]->type==VAL_NUM,
                                      "karakter() butuh (teks, indeks)");
                          const char *s = args[0]->str ? args[0]->str : "";
                          int idx = (int)args[1]->num;
                          int len = (int)strlen(s);
                          if (idx < 0 || idx >= len) return val_str("");
                          char buf[2] = { s[idx], 0 };
                          return val_str(buf);
                      }

                      static Val *builtin_bulatkan(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1 && args[0]->type==VAL_NUM, "bulatkan() butuh angka");
                          double v = args[0]->num;
                          return val_num(v >= 0 ? floor(v + 0.5) : ceil(v - 0.5));
                      }

                      static Val *builtin_irisLarik(Val **args, int n, int line) {
                          LOOP_ASSERT(n==3 && args[0]->type==VAL_ARRAY && args[1]->type==VAL_NUM && args[2]->type==VAL_NUM,
                                      "irisLarik() butuh (larik, awal, akhir)");
                          Val *arr = args[0];
                          int awal = (int)args[1]->num, akhir = (int)args[2]->num;
                          if (awal < 0) awal = 0;
                          if (akhir >= arr->arr_n) akhir = arr->arr_n - 1;
                          if (akhir < awal - 1) { return val_array(NULL, 0); }
                          int sz = akhir - awal + 1;
                          if (sz <= 0) return val_array(NULL, 0);
                          Val **out = malloc(sizeof(Val*) * (size_t)sz);
                          for (int i = 0; i < sz; i++) out[i] = val_ref(arr->arr[awal + i]);
                          return val_array(out, sz);
                      }

                      static Val *builtin_bacaFileTeks(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1 && args[0]->type==VAL_STR, "bacaFileTeks() butuh path teks");
                          FILE *f = fopen(args[0]->str, "rb");
                          if (!f) return val_str("");
                          fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
                          if (sz < 0) { fclose(f); return val_str(""); }
                          char *buf = malloc((size_t)sz + 1);
                          size_t got = fread(buf, 1, (size_t)sz, f);
                          buf[got] = 0;
                          fclose(f);
                          Val *r = val_str(buf);
                          free(buf);
                          return r;
                      }

                      static Val *builtin_tulisFileTeks(Val **args, int n, int line) {
                          LOOP_ASSERT(n==2 && args[0]->type==VAL_STR && args[1]->type==VAL_STR,
                                      "tulisFileTeks() butuh (path, isi)");
                          FILE *f = fopen(args[0]->str, "wb");
                          if (!f) return val_bool(0);
                          fputs(args[1]->str, f);
                          fclose(f);
                          return val_bool(1);
                      }

                      static Val *builtin_eksekusi(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1 && args[0]->type==VAL_STR, "jalankan() butuh teks perintah");
                          int rc = system(args[0]->str);
                          if (rc == -1) return val_num(-1);
                          if (WIFEXITED(rc)) return val_num(WEXITSTATUS(rc));
                          return val_num(-1);
                      }

                      static Val *builtin_gabungTeks(Val **args, int n, int line) {
                          LOOP_ASSERT(n==2 && args[0]->type==VAL_ARRAY && args[1]->type==VAL_STR,
                                      "gabungTeks() butuh (larik, pemisah)");
                          Val *arr = args[0]; const char *sep = args[1]->str;
                          size_t seplen = strlen(sep);
                          size_t cap = 4096, len = 0;
                          char *out = malloc(cap); out[0] = 0;
                          for (int i = 0; i < arr->arr_n; i++) {
                              char tmp[512];
                              const char *piece;
                              if (arr->arr[i]->type == VAL_STR) {
                                  piece = arr->arr[i]->str ? arr->arr[i]->str : "";
                              } else {
                                  val_format(arr->arr[i], tmp, sizeof(tmp));
                                  piece = tmp;
                              }
                              size_t plen = strlen(piece);
                              size_t need = len + plen + (i < arr->arr_n - 1 ? seplen : 0) + 1;
                              if (need > cap) { while (cap < need) cap *= 2; out = realloc(out, cap); }
                              memcpy(out + len, piece, plen); len += plen;
                              if (i < arr->arr_n - 1) { memcpy(out + len, sep, seplen); len += seplen; }
                              out[len] = 0;
                          }
                          Val *r = val_str(out);
                          free(out);
                          return r;
                      }

                      static Val *builtin_cetakTanpaBaris(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1, "cetakTanpaBaris() butuh 1 argumen");
                          if (args[0]->type == VAL_STR) {
                              printf("%s", args[0]->str ? args[0]->str : "");
                          } else {
                              char buf[65536];
                              val_format(args[0], buf, sizeof(buf));
                              printf("%s", buf);
                          }
                          return val_null();
                      }

                      static Val *builtin_gabung(Val **args, int n, int line) {
                          LOOP_ASSERT(n >= 1, "gabung() butuh minimal 1 argumen");
                          /* Simpan sebagai array khusus - representasi gabung */
                          Val **arr = malloc(sizeof(Val*)*n);
                          for (int i=0;i<n;i++) arr[i] = val_ref(args[i]);
                          Val *v = val_array(arr, n);
                          v->type = VAL_ARRAY; /* pakai array biasa, tampil khusus via format */
                          return v;
                      }

                      static Val *builtin_prima(Val **args, int n, int line) {
                          LOOP_ASSERT(n >= 1, "prima() butuh minimal 1 argumen");
                          uint64_t sebelumnya = 0;
                          uint64_t prod = 1;
                          uint64_t mod_prima = 1;
                          for (int i = 0; i < n; i++) {
                              uint64_t ni = (uint64_t)fabs(args[i]->num);
                              if (ni < 2) ni = 2;
                              uint64_t L = hitung_lapisan(ni, sebelumnya, i==0);
                              sebelumnya = L;
                              prod *= L;
                              mod_prima *= next_prima(ni);
                          }
                          uint64_t kunci = mod_prima > 0 ? prod % mod_prima : prod;
                          if (kunci == 0) kunci = prod + n;
                          Val *v = calloc(1, sizeof(Val));
                          v->type = VAL_PRIMA; v->refs = 1; v->prima_kunci = kunci;
                          return v;
                      }

                      static Val *builtin_acak(Val **args, int n, int line) {
                          LOOP_ASSERT(n >= 1, "acak() butuh minimal 1 argumen");
                          /* Ambil bytes dari args[0] */
                          char tmp[MAX_STRING];
                          val_format(args[0], tmp, MAX_STRING);
                          unsigned char *data = (unsigned char*)tmp;
                          int dlen = strlen(tmp);

                          /* Kunci */
                          uint64_t seed;
                          unsigned char kbuf[8];
                          int klen;
                          if (n >= 2 && args[1]->type == VAL_PRIMA) {
                              seed = args[1]->prima_kunci;
                              for (int i=0;i<8;i++) kbuf[7-i] = (seed>>(i*8))&0xFF;
                              klen = 8;
                          } else if (n >= 2) {
                              char kb[256]; val_format(args[1], kb, 256);
                              klen = strlen(kb); if (klen > 8) klen = 8;
                              memcpy(kbuf, kb, klen);
                              seed = 0; for (int i=0;i<klen;i++) seed = seed*256 + kbuf[i];
                          } else {
                              seed = (uint64_t)time(NULL);
                              for (int i=0;i<8;i++) kbuf[7-i] = (seed>>(i*8))&0xFF;
                              klen = 8;
                          }

                          unsigned char *out = malloc(dlen);
                          memcpy(out, data, dlen);
                          shuffle_bytes(out, dlen, seed);
                          xor_bytes(out, out, dlen, kbuf, klen);

                          /* Prima cipher opsional (arg ke-3) */
                          if (n >= 3 && args[2]->type == VAL_NUM) {
                              uint64_t pv = (uint64_t)args[2]->num;
                              LOOP_ASSERT(is_prima(pv), "Baris %d: acak() arg ke-3 harus prima, dapat %llu", line, (unsigned long long)pv);
                              unsigned char pb[8]; int plen = 0;
                              uint64_t tmp2 = pv;
                              while (tmp2 > 0) { pb[plen++] = tmp2 & 0xFF; tmp2 >>= 8; }
                              xor_bytes(out, out, dlen, pb, plen);
                          }

                          char *hex = bytes_to_hex(out, dlen);
                          Val *v = calloc(1, sizeof(Val));
                          v->type = VAL_ENKRIPSI; v->refs = 1;
                          v->enc_data = out; v->enc_len = dlen;
                          v->enc_hex  = hex; v->enc_seed = seed;
                          return v;
                      }

                      static Val *builtin_ganda(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1, "ganda() butuh 1 argumen");
                          double asli = args[0]->num;
                          /* Enkripsi sederhana: prima dari nilai */
                          Val *karg[1] = { val_num(fabs(asli) < 2 ? 2 : fabs(asli)) };
                          Val *kunci = builtin_prima(karg, 1, line);
                          val_free(karg[0]);
                          char src[64];
                          snprintf(src, 64, "%g", asli);
                          unsigned char kbuf[8]; uint64_t k = kunci->prima_kunci;
                          for (int i=0;i<8;i++) kbuf[7-i]=(k>>(i*8))&0xFF;
                          unsigned char out[64]; int slen = strlen(src);
                          xor_bytes(out, (unsigned char*)src, slen, kbuf, 8);
                          val_free(kunci);
                          char *hex = bytes_to_hex(out, slen);
                          Val *v = calloc(1,sizeof(Val));
                          v->type = VAL_GANDA; v->refs = 1;
                          v->ganda_asli = asli; v->ganda_enc = hex;
                          return v;
                      }

                      /* ==================== Jaringan (networking) ==================== */

                      static int net_tcp_connect(const char *host, int port) {
                          char portstr[16];
                          snprintf(portstr, sizeof(portstr), "%d", port);
                          struct addrinfo hints, *res, *rp;
                          memset(&hints, 0, sizeof(hints));
                          hints.ai_family = AF_UNSPEC;
                          hints.ai_socktype = SOCK_STREAM;
                          if (getaddrinfo(host, portstr, &hints, &res) != 0) return -1;
                          int fd = -1;
                          for (rp = res; rp != NULL; rp = rp->ai_next) {
                              fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
                              if (fd < 0) continue;
                              if (connect(fd, rp->ai_addr, rp->ai_addrlen) == 0) break;
                              close(fd); fd = -1;
                          }
                          freeaddrinfo(res);
                          return fd;
                      }

                      /* Parse URL sederhana: skema://host[:port]/path */
                      static int net_parse_url(const char *url, char *skema, char *host, int *port, char *path) {
                          skema[0] = 0; host[0] = 0; path[0] = 0; *port = 0;
                          const char *p = strstr(url, "://");
                          const char *rest;
                          if (p) {
                              int slen = (int)(p - url);
                              if (slen >= 16) slen = 15;
                              memcpy(skema, url, slen); skema[slen] = 0;
                              rest = p + 3;
                          } else {
                              strcpy(skema, "http");
                              rest = url;
                          }
                          const char *slash = strchr(rest, '/');
                          const char *hostend = slash ? slash : rest + strlen(rest);
                          char hostport[512];
                          int hlen = (int)(hostend - rest);
                          if (hlen >= (int)sizeof(hostport)) hlen = (int)sizeof(hostport) - 1;
                          memcpy(hostport, rest, hlen); hostport[hlen] = 0;
                          char *colon = strchr(hostport, ':');
                          if (colon) {
                              *colon = 0;
                              *port = atoi(colon + 1);
                          }
                          strncpy(host, hostport, 255); host[255] = 0;
                          if (*port == 0) *port = (strcmp(skema, "https") == 0) ? 443 : 80;
                          if (slash) strncpy(path, slash, MAX_STRING - 1);
                          else strcpy(path, "/");
                          return 1;
                      }

                      static Val *net_make_koneksi(int fd, int tls, void *ssl, void *ctx, const char *host, int port) {
                          Val *v = calloc(1, sizeof(Val));
                          v->type = VAL_KONEKSI; v->refs = 1;
                          v->kon_fd = fd; v->kon_tls = tls; v->kon_ssl = ssl; v->kon_ctx = ctx;
                          v->kon_host = strdup(host ? host : ""); v->kon_port = port;
                          return v;
                      }

                      static Val *builtin_sambung(Val **args, int n, int line) {
                          LOOP_ASSERT(n==2 && args[0]->type==VAL_STR && args[1]->type==VAL_NUM,
                                      "sambung() butuh (host, port)");
                          int port = (int)args[1]->num;
                          int fd = net_tcp_connect(args[0]->str, port);
                          LOOP_ASSERT(fd >= 0, "Baris %d: sambung() gagal konek ke %s:%d", line, args[0]->str, port);
                          return net_make_koneksi(fd, 0, NULL, NULL, args[0]->str, port);
                      }

                      static Val *builtin_tlsSambung(Val **args, int n, int line) {
                          LOOP_ASSERT(n==2 && args[0]->type==VAL_STR && args[1]->type==VAL_NUM,
                                      "tlsSambung() butuh (host, port)");
                          int port = (int)args[1]->num;
                          int fd = net_tcp_connect(args[0]->str, port);
                          LOOP_ASSERT(fd >= 0, "Baris %d: tlsSambung() gagal konek ke %s:%d", line, args[0]->str, port);

                          SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
                          if (!ctx) { close(fd); loop_error("Baris %d: tlsSambung() gagal buat konteks TLS", line); }
                          SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
                          SSL *ssl = SSL_new(ctx);
                          SSL_set_fd(ssl, fd);
                          SSL_set_tlsext_host_name(ssl, args[0]->str);
                          if (SSL_connect(ssl) != 1) {
                              SSL_free(ssl); SSL_CTX_free(ctx); close(fd);
                              loop_error("Baris %d: tlsSambung() gagal handshake TLS ke %s:%d", line, args[0]->str, port);
                          }
                          return net_make_koneksi(fd, 1, ssl, ctx, args[0]->str, port);
                      }

                      static Val *builtin_kirim(Val **args, int n, int line) {
                          LOOP_ASSERT(n==2 && args[0]->type==VAL_KONEKSI, "kirim() butuh (koneksi, data)");
                          Val *kon = args[0];
                          LOOP_ASSERT(kon->kon_fd >= 0, "Baris %d: kirim() koneksi sudah tertutup", line);
                          char buf[MAX_STRING];
                          const char *data; int len;
                          if (args[1]->type == VAL_STR) { data = args[1]->str ? args[1]->str : ""; len = (int)strlen(data); }
                          else { val_format(args[1], buf, sizeof(buf)); data = buf; len = (int)strlen(buf); }
                          int sent = kon->kon_tls ? SSL_write((SSL*)kon->kon_ssl, data, len)
                          : (int)send(kon->kon_fd, data, len, 0);
                          LOOP_ASSERT(sent >= 0, "Baris %d: kirim() gagal mengirim data", line);
                          return val_num((double)sent);
                      }

                      static Val *builtin_terima(Val **args, int n, int line) {
                          LOOP_ASSERT(n>=1 && args[0]->type==VAL_KONEKSI, "terima() butuh (koneksi, [maks])");
                          Val *kon = args[0];
                          LOOP_ASSERT(kon->kon_fd >= 0, "Baris %d: terima() koneksi sudah tertutup", line);
                          int maks = (n >= 2 && args[1]->type==VAL_NUM) ? (int)args[1]->num : 4096;
                          if (maks <= 0) maks = 4096;
                          if (maks > 65536) maks = 65536;
                          char *buf = malloc(maks + 1);
                          int got = kon->kon_tls ? SSL_read((SSL*)kon->kon_ssl, buf, maks)
                          : (int)recv(kon->kon_fd, buf, maks, 0);
                          if (got < 0) got = 0;
                          buf[got] = 0;
                          Val *r = val_str(buf);
                          free(buf);
                          return r;
                      }

                      static Val *builtin_tutupKoneksi(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1 && args[0]->type==VAL_KONEKSI, "tutupKoneksi() butuh (koneksi)");
                          Val *kon = args[0];
                          if (kon->kon_ssl) { SSL_shutdown((SSL*)kon->kon_ssl); SSL_free((SSL*)kon->kon_ssl); kon->kon_ssl = NULL; }
                          if (kon->kon_ctx) { SSL_CTX_free((SSL_CTX*)kon->kon_ctx); kon->kon_ctx = NULL; }
                          if (kon->kon_fd >= 0) { close(kon->kon_fd); kon->kon_fd = -1; }
                          return val_null();
                      }

                      /* Ambil satu halaman lewat HTTP/HTTPS GET, kembalikan body sebagai teks */
                      static char *net_http_get(const char *url, int line, char *out_host) {
                          char skema[16], host[256], path[MAX_STRING];
                          int port;
                          net_parse_url(url, skema, host, &port, path);
                          int tls = (strcmp(skema, "https") == 0);
                          if (out_host) strncpy(out_host, host, 255);

                          int fd = net_tcp_connect(host, port);
                          LOOP_ASSERT(fd >= 0, "Baris %d: ambil()/telusuri() gagal konek ke %s:%d", line, host, port);

                          SSL_CTX *ctx = NULL; SSL *ssl = NULL;
                          if (tls) {
                              ctx = SSL_CTX_new(TLS_client_method());
                              SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
                              ssl = SSL_new(ctx);
                              SSL_set_fd(ssl, fd);
                              SSL_set_tlsext_host_name(ssl, host);
                              if (SSL_connect(ssl) != 1) {
                                  SSL_free(ssl); SSL_CTX_free(ctx); close(fd);
                                  loop_error("Baris %d: ambil()/telusuri() gagal handshake TLS ke %s", line, host);
                              }
                          }

                          char req[MAX_STRING];
                          snprintf(req, sizeof(req),
                                   "GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: Loop/%s\r\nConnection: close\r\n\r\n",
                                   path, host, LOOP_VERSI);
                          int rlen = (int)strlen(req);
                          if (tls) SSL_write(ssl, req, rlen); else send(fd, req, rlen, 0);

                          size_t cap = 65536, len = 0;
                          char *out = malloc(cap);
                          char chunk[4096];
                          int got;
                          while ((got = tls ? SSL_read(ssl, chunk, sizeof(chunk)) : (int)recv(fd, chunk, sizeof(chunk), 0)) > 0) {
                              if (len + (size_t)got + 1 > cap) { while (len + (size_t)got + 1 > cap) cap *= 2; out = realloc(out, cap); }
                              memcpy(out + len, chunk, got); len += got;
                          }
                          out[len] = 0;

                          if (ssl) { SSL_shutdown(ssl); SSL_free(ssl); }
                          if (ctx) SSL_CTX_free(ctx);
                          close(fd);

                          /* Pisahkan header HTTP dari body */
                          char *body = strstr(out, "\r\n\r\n");
                          if (body) {
                              body += 4;
                              memmove(out, body, strlen(body) + 1);
                          }
                          return out;
                      }

                      static Val *builtin_ambil(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1 && args[0]->type==VAL_STR, "ambil() butuh (url)");
                          char *body = net_http_get(args[0]->str, line, NULL);
                          Val *r = val_str(body);
                          free(body);
                          return r;
                      }

                      static Val *builtin_telusuri(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1 || n==2, "telusuri() butuh (url) atau (url, kedalaman)");
                          LOOP_ASSERT(args[0]->type==VAL_STR, "telusuri(): argumen pertama harus teks (url)");

                          int max_depth = 0;
                          if (n == 2) {
                              LOOP_ASSERT(args[1]->type==VAL_NUM, "telusuri(): kedalaman harus angka");
                              max_depth = (int)args[1]->num;
                              if (max_depth < 0) max_depth = 0;
                              if (max_depth > 5) max_depth = 5;  /* Batasi max crawl depth */
                          }

                          char host[256];
                          char *body = net_http_get(args[0]->str, line, host);
                          if (!body) return val_array(NULL, 0);

                          /* Ekstrak semua link href="..." / href='...' dari body */
                          Val **links = malloc(sizeof(Val*) * MAX_ARRAY);
                          int nlinks = 0;
                          const char *p = body;
                          while ((p = strstr(p, "href=")) != NULL && nlinks < MAX_ARRAY) {
                              p += 5;
                              char quote = *p;
                              if (quote != '"' && quote != '\'') { continue; }
                              p++;
                              const char *end = strchr(p, quote);
                              if (!end) break;
                              int llen = (int)(end - p);
                              if (llen > 0 && llen < MAX_STRING) {
                                  char buf[MAX_STRING];
                                  memcpy(buf, p, llen); buf[llen] = 0;
                                  links[nlinks++] = val_str(buf);
                              }
                              p = end + 1;
                          }
                          free(body);

                          if (nlinks == 0) { free(links); return val_array(NULL, 0); }
                          return val_array(links, nlinks);
                      }

                      /* ---------------------------------------------------------------
                       * IO tambahan (selaras dengan syntaxes/loop.tmLanguage.json v1.4.0)
                       * --------------------------------------------------------------- */

                      static Val *builtin_tambahFileTeks(Val **args, int n, int line) {
                          LOOP_ASSERT(n==2 && args[0]->type==VAL_STR && args[1]->type==VAL_STR,
                                      "tambahFileTeks() butuh (path, isi)");
                          FILE *f = fopen(args[0]->str, "ab");
                          if (!f) return val_bool(0);
                          fputs(args[1]->str, f);
                          fclose(f);
                          return val_bool(1);
                      }

                      static Val *builtin_appendFile(Val **args, int n, int line) {
                          LOOP_ASSERT(n==2 && args[0]->type==VAL_STR, "appendFile() butuh (path, isi)");
                          FILE *f = fopen(args[0]->str, "ab");
                          if (!f) return val_bool(0);
                          if (args[1]->type == VAL_STR) {
                              fputs(args[1]->str, f);
                          } else {
                              char buf[65536]; val_format(args[1], buf, sizeof(buf));
                              fputs(buf, f);
                          }
                          fclose(f);
                          return val_bool(1);
                      }

                      static Val *builtin_tulis(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1, "tulis() butuh 1 argumen");
                          if (args[0]->type == VAL_STR) printf("%s", args[0]->str ? args[0]->str : "");
                          else { char buf[65536]; val_format(args[0], buf, sizeof(buf)); printf("%s", buf); }
                          return val_null();
                      }

                      static Val *builtin_tulisInline(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1, "tulisInline() butuh 1 argumen");
                          if (args[0]->type == VAL_STR) printf("%s", args[0]->str ? args[0]->str : "");
                          else { char buf[65536]; val_format(args[0], buf, sizeof(buf)); printf("%s", buf); }
                          fflush(stdout);
                          return val_null();
                      }

                      static Val *builtin_tulisStderr(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1, "tulisStderr() butuh 1 argumen");
                          if (args[0]->type == VAL_STR) fprintf(stderr, "%s\n", args[0]->str ? args[0]->str : "");
                          else { char buf[65536]; val_format(args[0], buf, sizeof(buf)); fprintf(stderr, "%s\n", buf); }
                          return val_null();
                      }

                      static Val *builtin_tulisError(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1, "tulisError() butuh 1 argumen");
                          if (args[0]->type == VAL_STR) fprintf(stderr, "[error] %s\n", args[0]->str ? args[0]->str : "");
                          else { char buf[65536]; val_format(args[0], buf, sizeof(buf)); fprintf(stderr, "[error] %s\n", buf); }
                          return val_null();
                      }

                      static Val *builtin_barisBaru(Val **args, int n, int line) {
                          printf("\n");
                          return val_null();
                      }

                      static Val *builtin_waktumilisdetik(Val **args, int n, int line) {
                          struct timespec ts; clock_gettime(CLOCK_REALTIME, &ts);
                          double ms = (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
                          return val_num(ms);
                      }

                      static Val *builtin_waktuSekarang(Val **args, int n, int line) {
                          return val_num((double)time(NULL));
                      }

                      static Val *builtin_waktuISO(Val **args, int n, int line) {
                          time_t t = time(NULL);
                          struct tm tmv; gmtime_r(&t, &tmv);
                          char buf[64];
                          strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tmv);
                          return val_str(buf);
                      }

                      static Val *builtin_tidur(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1 && args[0]->type==VAL_NUM, "tidur() butuh angka (milidetik)");
                          long ms = (long)args[0]->num;
                          if (ms > 0) usleep((useconds_t)(ms * 1000));
                          return val_null();
                      }

                      /* ---------------------------------------------------------------
                       * String tambahan
                       * --------------------------------------------------------------- */

                      static Val *builtin_keAngka(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1, "keAngka() butuh 1 argumen");
                          if (args[0]->type == VAL_NUM)  return val_num(args[0]->num);
                          if (args[0]->type == VAL_BOOL) return val_num(args[0]->bval ? 1 : 0);
                          if (args[0]->type == VAL_STR)  return val_num(atof(args[0]->str ? args[0]->str : ""));
                          loop_error("Baris %d: keAngka() tidak bisa untuk tipe ini", line); return NULL;
                      }

                      static Val *builtin_keDesimal(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1, "keDesimal() butuh 1 argumen");
                          if (args[0]->type == VAL_NUM)  return val_num(args[0]->num);
                          if (args[0]->type == VAL_STR)  return val_num(atof(args[0]->str ? args[0]->str : ""));
                          loop_error("Baris %d: keDesimal() tidak bisa untuk tipe ini", line); return NULL;
                      }

                      static Val *builtin_keTeks(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1, "keTeks() butuh 1 argumen");
                          char buf[65536]; val_format(args[0], buf, sizeof(buf));
                          return val_str(buf);
                      }

                      static Val *builtin_dariKode(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1 && args[0]->type==VAL_NUM, "dariKode() butuh angka");
                          char buf[2] = { (char)(int)args[0]->num, 0 };
                          return val_str(buf);
                      }

                      static Val *builtin_potong(Val **args, int n, int line) {
                          LOOP_ASSERT(n>=2 && args[0]->type==VAL_STR && args[1]->type==VAL_NUM,
                                      "potong() butuh (teks, awal[, panjang])");
                          const char *s = args[0]->str ? args[0]->str : "";
                          int len = (int)strlen(s);
                          int awal = (int)args[1]->num;
                          if (awal < 0) awal = len + awal; if (awal < 0) awal = 0;
                          int ambil = (n>=3 && args[2]->type==VAL_NUM) ? (int)args[2]->num : (len - awal);
                          if (awal >= len || ambil <= 0) return val_str("");
                          if (awal + ambil > len) ambil = len - awal;
                          char *buf = malloc((size_t)ambil + 1);
                          memcpy(buf, s + awal, (size_t)ambil); buf[ambil] = 0;
                          Val *r = val_str(buf); free(buf); return r;
                      }

                      static Val *builtin_cariTeks(Val **args, int n, int line) {
                          LOOP_ASSERT(n==2 && args[0]->type==VAL_STR && args[1]->type==VAL_STR,
                                      "cariTeks() butuh (teks, cari)");
                          const char *hay = args[0]->str ? args[0]->str : "";
                          const char *needle = args[1]->str ? args[1]->str : "";
                          const char *p = strstr(hay, needle);
                          return val_num(p ? (double)(p - hay) : -1);
                      }

                      static Val *builtin_gantiTeks(Val **args, int n, int line) {
                          LOOP_ASSERT(n==3 && args[0]->type==VAL_STR && args[1]->type==VAL_STR && args[2]->type==VAL_STR,
                                      "gantiTeks() butuh (teks, cari, ganti)");
                          const char *s = args[0]->str ? args[0]->str : "";
                          const char *cari = args[1]->str ? args[1]->str : "";
                          const char *ganti = args[2]->str ? args[2]->str : "";
                          size_t cari_len = strlen(cari);
                          if (cari_len == 0) return val_str(s);
                          size_t cap = strlen(s) + 1, len = 0;
                          char *out = malloc(cap); out[0] = 0;
                          const char *cur = s;
                          while (*cur) {
                              const char *p = strstr(cur, cari);
                              if (!p) {
                                  size_t rest = strlen(cur);
                                  if (len + rest + 1 > cap) { cap = len + rest + 1; out = realloc(out, cap); }
                                  memcpy(out + len, cur, rest); len += rest; out[len] = 0;
                                  break;
                              }
                              size_t pre = (size_t)(p - cur);
                              size_t ganti_len = strlen(ganti);
                              if (len + pre + ganti_len + 1 > cap) { cap = (len + pre + ganti_len + 1) * 2; out = realloc(out, cap); }
                              memcpy(out + len, cur, pre); len += pre;
                              memcpy(out + len, ganti, ganti_len); len += ganti_len;
                              out[len] = 0;
                              cur = p + cari_len;
                          }
                          Val *r = val_str(out); free(out); return r;
                      }

                      static Val *builtin_hurufBesar(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1 && args[0]->type==VAL_STR, "hurufBesar() butuh teks");
                          char *s = strdup(args[0]->str ? args[0]->str : "");
                          for (char *p = s; *p; p++) *p = (char)toupper((unsigned char)*p);
                          Val *r = val_str(s); free(s); return r;
                      }

                      static Val *builtin_hurufKecil(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1 && args[0]->type==VAL_STR, "hurufKecil() butuh teks");
                          char *s = strdup(args[0]->str ? args[0]->str : "");
                          for (char *p = s; *p; p++) *p = (char)tolower((unsigned char)*p);
                          Val *r = val_str(s); free(s); return r;
                      }

                      static Val *builtin_pisah(Val **args, int n, int line) {
                          LOOP_ASSERT(n==2 && args[0]->type==VAL_STR && args[1]->type==VAL_STR,
                                      "pisah() butuh (teks, pemisah)");
                          const char *s = args[0]->str ? args[0]->str : "";
                          const char *sep = args[1]->str ? args[1]->str : "";
                          size_t sep_len = strlen(sep);
                          int cap = 8, cnt = 0;
                          Val **out = malloc(sizeof(Val*) * cap);
                          if (sep_len == 0) {
                              int len = (int)strlen(s);
                              for (int i = 0; i < len; i++) {
                                  if (cnt >= cap) { cap *= 2; out = realloc(out, sizeof(Val*)*cap); }
                                  char buf[2] = { s[i], 0 };
                                  out[cnt++] = val_str(buf);
                              }
                              return val_array(out, cnt);
                          }
                          const char *cur = s;
                          for (;;) {
                              const char *p = strstr(cur, sep);
                              if (cnt >= cap) { cap *= 2; out = realloc(out, sizeof(Val*)*cap); }
                              if (!p) { out[cnt++] = val_str(cur); break; }
                              size_t plen = (size_t)(p - cur);
                              char *piece = malloc(plen + 1); memcpy(piece, cur, plen); piece[plen] = 0;
                              out[cnt++] = val_str(piece); free(piece);
                              cur = p + sep_len;
                          }
                          return val_array(out, cnt);
                      }

                      static Val *builtin_trimTeks(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1 && args[0]->type==VAL_STR, "trimTeks() butuh teks");
                          const char *s = args[0]->str ? args[0]->str : "";
                          int a = 0, b = (int)strlen(s) - 1;
                          while (a <= b && isspace((unsigned char)s[a])) a++;
                          while (b >= a && isspace((unsigned char)s[b])) b--;
                          int len = b - a + 1; if (len < 0) len = 0;
                          char *buf = malloc((size_t)len + 1);
                          memcpy(buf, s + a, (size_t)len); buf[len] = 0;
                          Val *r = val_str(buf); free(buf); return r;
                      }

                      static Val *builtin_mulaiDengan(Val **args, int n, int line) {
                          LOOP_ASSERT(n==2 && args[0]->type==VAL_STR && args[1]->type==VAL_STR,
                                      "mulaiDengan() butuh (teks, awalan)");
                          const char *s = args[0]->str ? args[0]->str : "";
                          const char *p = args[1]->str ? args[1]->str : "";
                          return val_bool(strncmp(s, p, strlen(p)) == 0);
                      }

                      static Val *builtin_akhiriDengan(Val **args, int n, int line) {
                          LOOP_ASSERT(n==2 && args[0]->type==VAL_STR && args[1]->type==VAL_STR,
                                      "akhiriDengan() butuh (teks, akhiran)");
                          const char *s = args[0]->str ? args[0]->str : "";
                          const char *p = args[1]->str ? args[1]->str : "";
                          size_t sl = strlen(s), pl = strlen(p);
                          if (pl > sl) return val_bool(0);
                          return val_bool(strcmp(s + (sl - pl), p) == 0);
                      }

                      static Val *builtin_regexCocok(Val **args, int n, int line) {
                          LOOP_ASSERT(n==2 && args[0]->type==VAL_STR && args[1]->type==VAL_STR,
                                      "regexCocok() butuh (teks, pola)");
                          regex_t re;
                          if (regcomp(&re, args[1]->str, REG_EXTENDED) != 0)
                              loop_error("Baris %d: regexCocok() pola tidak valid", line);
                          int rc = regexec(&re, args[0]->str ? args[0]->str : "", 0, NULL, 0);
                          regfree(&re);
                          return val_bool(rc == 0);
                      }

                      static Val *builtin_regexGanti(Val **args, int n, int line) {
                          LOOP_ASSERT(n==3 && args[0]->type==VAL_STR && args[1]->type==VAL_STR && args[2]->type==VAL_STR,
                                      "regexGanti() butuh (teks, pola, ganti)");
                          regex_t re;
                          if (regcomp(&re, args[1]->str, REG_EXTENDED) != 0)
                              loop_error("Baris %d: regexGanti() pola tidak valid", line);
                          const char *s = args[0]->str ? args[0]->str : "";
                          const char *ganti = args[2]->str ? args[2]->str : "";
                          size_t cap = strlen(s) + 1, len = 0;
                          char *out = malloc(cap); out[0] = 0;
                          const char *cur = s;
                          regmatch_t m;
                          while (*cur && regexec(&re, cur, 1, &m, 0) == 0) {
                              size_t pre = (size_t)m.rm_so;
                              size_t glen = strlen(ganti);
                              if (len + pre + glen + 1 > cap) { cap = (len + pre + glen + 1) * 2; out = realloc(out, cap); }
                              memcpy(out + len, cur, pre); len += pre;
                              memcpy(out + len, ganti, glen); len += glen;
                              out[len] = 0;
                              if (m.rm_eo == m.rm_so) { /* cegah infinite loop pada match kosong */
                                  if (cur[m.rm_eo] == 0) break;
                                  if (len + 1 + 1 > cap) { cap *= 2; out = realloc(out, cap); }
                                  out[len++] = cur[m.rm_eo]; out[len] = 0;
                                  cur += m.rm_eo + 1;
                              } else {
                                  cur += m.rm_eo;
                              }
                          }
                          size_t rest = strlen(cur);
                          if (len + rest + 1 > cap) { cap = len + rest + 1; out = realloc(out, cap); }
                          memcpy(out + len, cur, rest); len += rest; out[len] = 0;
                          regfree(&re);
                          Val *r = val_str(out); free(out); return r;
                      }

                      static Val *builtin_terakhirIndexDari(Val **args, int n, int line) {
                          LOOP_ASSERT(n==2 && args[0]->type==VAL_STR && args[1]->type==VAL_STR,
                                      "terakhirIndexDari() butuh (teks, cari)");
                          const char *hay = args[0]->str ? args[0]->str : "";
                          const char *needle = args[1]->str ? args[1]->str : "";
                          size_t nlen = strlen(needle);
                          if (nlen == 0) return val_num((double)strlen(hay));
                          const char *last = NULL, *cur = hay;
                          for (;;) {
                              const char *p = strstr(cur, needle);
                              if (!p) break;
                              last = p; cur = p + nlen;
                          }
                          return val_num(last ? (double)(last - hay) : -1);
                      }

                      /* ---------------------------------------------------------------
                       * Larik tambahan
                       * --------------------------------------------------------------- */

                      static Val *builtin_dorong(Val **args, int n, int line) {
                          LOOP_ASSERT(n==2 && args[0]->type==VAL_ARRAY, "dorong() butuh (larik, nilai)");
                          Val *a = args[0]; int sz = a->arr_n + 1;
                          Val **arr = malloc(sizeof(Val*) * (size_t)sz);
                          for (int i=0;i<a->arr_n;i++) arr[i] = val_ref(a->arr[i]);
                          arr[a->arr_n] = val_ref(args[1]);
                          return val_array(arr, sz);
                      }

                      static Val *builtin_hapusTerakhir(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1 && args[0]->type==VAL_ARRAY, "hapusTerakhir() butuh larik");
                          Val *a = args[0]; int sz = a->arr_n > 0 ? a->arr_n - 1 : 0;
                          Val **arr = sz > 0 ? malloc(sizeof(Val*) * (size_t)sz) : NULL;
                          for (int i=0;i<sz;i++) arr[i] = val_ref(a->arr[i]);
                          return val_array(arr, sz);
                      }

                      static Val *builtin_hapusKunci(Val **args, int n, int line) {
                          LOOP_ASSERT(n==2 && args[0]->type==VAL_ARRAY && args[1]->type==VAL_NUM,
                                      "hapusKunci() butuh (larik, indeks)");
                          Val *a = args[0]; int idx = (int)args[1]->num;
                          if (idx < 0 || idx >= a->arr_n) {
                              Val **arr = a->arr_n > 0 ? malloc(sizeof(Val*)*a->arr_n) : NULL;
                              for (int i=0;i<a->arr_n;i++) arr[i] = val_ref(a->arr[i]);
                              return val_array(arr, a->arr_n);
                          }
                          int sz = a->arr_n - 1;
                          Val **arr = sz > 0 ? malloc(sizeof(Val*) * (size_t)sz) : NULL;
                          int j = 0;
                          for (int i=0;i<a->arr_n;i++) if (i != idx) arr[j++] = val_ref(a->arr[i]);
                          return val_array(arr, sz);
                      }

                      static Val *builtin_hapusLarik(Val **args, int n, int line) {
                          LOOP_ASSERT(n==2 && args[0]->type==VAL_ARRAY, "hapusLarik() butuh (larik, nilai)");
                          Val *a = args[0], *target = args[1];
                          int found = -1;
                          for (int i=0;i<a->arr_n;i++) {
                              Val *e = a->arr[i];
                              if (e->type != target->type) continue;
                              int sama = 0;
                              if (e->type==VAL_NUM)  sama = (e->num == target->num);
                              else if (e->type==VAL_STR)  sama = (strcmp(e->str?e->str:"", target->str?target->str:"")==0);
                              else if (e->type==VAL_BOOL) sama = (e->bval == target->bval);
                              if (sama) { found = i; break; }
                          }
                          if (found < 0) {
                              Val **arr = a->arr_n > 0 ? malloc(sizeof(Val*)*a->arr_n) : NULL;
                              for (int i=0;i<a->arr_n;i++) arr[i] = val_ref(a->arr[i]);
                              return val_array(arr, a->arr_n);
                          }
                          int sz = a->arr_n - 1;
                          Val **arr = sz > 0 ? malloc(sizeof(Val*) * (size_t)sz) : NULL;
                          int j = 0;
                          for (int i=0;i<a->arr_n;i++) if (i != found) arr[j++] = val_ref(a->arr[i]);
                          return val_array(arr, sz);
                      }

                      static Val *builtin_iris(Val **args, int n, int line) {
                          LOOP_ASSERT(n==3 && args[1]->type==VAL_NUM && args[2]->type==VAL_NUM,
                                      "iris() butuh (larik|teks, awal, akhir)");
                          int awal = (int)args[1]->num, akhir = (int)args[2]->num;
                          if (args[0]->type == VAL_ARRAY) {
                              Val *arr = args[0];
                              if (awal < 0) awal = 0;
                              if (akhir >= arr->arr_n) akhir = arr->arr_n - 1;
                              int sz = akhir - awal + 1;
                              if (sz <= 0) return val_array(NULL, 0);
                              Val **out = malloc(sizeof(Val*) * (size_t)sz);
                              for (int i = 0; i < sz; i++) out[i] = val_ref(arr->arr[awal + i]);
                              return val_array(out, sz);
                          }
                          if (args[0]->type == VAL_STR) {
                              const char *s = args[0]->str ? args[0]->str : "";
                              int len = (int)strlen(s);
                              if (awal < 0) awal = 0;
                              if (akhir >= len) akhir = len - 1;
                              int sz = akhir - awal + 1;
                              if (sz <= 0) return val_str("");
                              char *buf = malloc((size_t)sz + 1);
                              memcpy(buf, s + awal, (size_t)sz); buf[sz] = 0;
                              Val *r = val_str(buf); free(buf); return r;
                          }
                          loop_error("Baris %d: iris() butuh larik atau teks", line); return NULL;
                      }

                      static Val *builtin_adaDalam(Val **args, int n, int line) {
                          LOOP_ASSERT(n==2 && args[0]->type==VAL_ARRAY, "adaDalam() butuh (larik, nilai)");
                          Val *a = args[0], *target = args[1];
                          for (int i=0;i<a->arr_n;i++) {
                              Val *e = a->arr[i];
                              if (e->type != target->type) continue;
                              if (e->type==VAL_NUM  && e->num == target->num) return val_bool(1);
                              if (e->type==VAL_STR  && strcmp(e->str?e->str:"", target->str?target->str:"")==0) return val_bool(1);
                              if (e->type==VAL_BOOL && e->bval == target->bval) return val_bool(1);
                          }
                          return val_bool(0);
                      }

                      static Val *builtin_saring(Val **args, int n, int line) {
                          LOOP_ASSERT(n==2 && args[0]->type==VAL_ARRAY && args[1]->type==VAL_FUNC,
                                      "saring() butuh (larik, fungsi)");
                          Val *a = args[0];
                          int cap = a->arr_n > 0 ? a->arr_n : 1, cnt = 0;
                          Val **out = malloc(sizeof(Val*) * (size_t)cap);
                          for (int i=0;i<a->arr_n;i++) {
                              Val *fargs[1] = { a->arr[i] };
                              Val *r = panggil_nilai_fungsi(args[1], fargs, 1, line);
                              int keep = (r->type==VAL_BOOL && r->bval) || (r->type==VAL_NUM && r->num != 0);
                              val_free(r);
                              if (keep) out[cnt++] = val_ref(a->arr[i]);
                          }
                          return val_array(out, cnt);
                      }

                      static Val *builtin_peta(Val **args, int n, int line) {
                          LOOP_ASSERT(n==2 && args[0]->type==VAL_ARRAY && args[1]->type==VAL_FUNC,
                                      "peta() butuh (larik, fungsi)");
                          Val *a = args[0];
                          Val **out = a->arr_n > 0 ? malloc(sizeof(Val*) * (size_t)a->arr_n) : NULL;
                          for (int i=0;i<a->arr_n;i++) {
                              Val *fargs[1] = { a->arr[i] };
                              out[i] = panggil_nilai_fungsi(args[1], fargs, 1, line);
                          }
                          return val_array(out, a->arr_n);
                      }

                      static Val *builtin_kurangi(Val **args, int n, int line) {
                          LOOP_ASSERT(n==3 && args[0]->type==VAL_ARRAY && args[1]->type==VAL_FUNC,
                                      "kurangi() butuh (larik, fungsi, nilaiAwal)");
                          Val *a = args[0];
                          Val *acc = val_ref(args[2]);
                          for (int i=0;i<a->arr_n;i++) {
                              Val *fargs[2] = { acc, a->arr[i] };
                              Val *r = panggil_nilai_fungsi(args[1], fargs, 2, line);
                              val_free(acc);
                              acc = r;
                          }
                          return acc;
                      }

                      static Val *builtin_kunciObjek(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1 && args[0]->type==VAL_ARRAY, "kunciObjek() butuh larik");
                          Val *a = args[0];
                          Val **out = a->arr_n > 0 ? malloc(sizeof(Val*) * (size_t)a->arr_n) : NULL;
                          for (int i=0;i<a->arr_n;i++) out[i] = val_num(i);
                          return val_array(out, a->arr_n);
                      }

                      static Val *val_deep_copy(Val *v) {
                          switch (v->type) {
                              case VAL_STR:   return val_str(v->str ? v->str : "");
                              case VAL_NUM:   return val_num(v->num);
                              case VAL_BOOL:  return val_bool(v->bval);
                              case VAL_ARRAY: {
                                  Val **out = v->arr_n > 0 ? malloc(sizeof(Val*) * (size_t)v->arr_n) : NULL;
                                  for (int i=0;i<v->arr_n;i++) out[i] = val_deep_copy(v->arr[i]);
                                  return val_array(out, v->arr_n);
                              }
                              default: return val_ref(v);
                          }
                      }

                      static Val *builtin_salin(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1, "salin() butuh 1 argumen");
                          return val_deep_copy(args[0]);
                      }

                      /* ---------------------------------------------------------------
                       * Matematika tambahan
                       * --------------------------------------------------------------- */

                      static Val *builtin_lantai(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1 && args[0]->type==VAL_NUM, "lantai() butuh angka");
                          return val_num(floor(args[0]->num));
                      }

                      static Val *builtin_langit(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1 && args[0]->type==VAL_NUM, "langit() butuh angka");
                          return val_num(ceil(args[0]->num));
                      }

                      static Val *builtin_absolut(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1 && args[0]->type==VAL_NUM, "absolut() butuh angka");
                          return val_num(fabs(args[0]->num));
                      }

                      static Val *builtin_acakAngka(Val **args, int n, int line) {
                          if (n >= 2 && args[0]->type==VAL_NUM && args[1]->type==VAL_NUM) {
                              double lo = args[0]->num, hi = args[1]->num;
                              double r = lo + ((double)rand() / ((double)RAND_MAX + 1.0)) * (hi - lo);
                              return val_num(r);
                          }
                          return val_num((double)rand() / ((double)RAND_MAX + 1.0));
                      }

                      static Val *builtin_log(Val **args, int n, int line) {
                          LOOP_ASSERT(n>=1 && args[0]->type==VAL_NUM, "log() butuh angka");
                          if (n >= 2 && args[1]->type==VAL_NUM) return val_num(log(args[0]->num) / log(args[1]->num));
                          return val_num(log(args[0]->num));
                      }

                      static Val *builtin_kosinus(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1 && args[0]->type==VAL_NUM, "kosinus() butuh angka");
                          return val_num(cos(args[0]->num));
                      }

                      static Val *builtin_sinus(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1 && args[0]->type==VAL_NUM, "sinus() butuh angka");
                          return val_num(sin(args[0]->num));
                      }

                      static Val *builtin_akarKuadrat(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1 && args[0]->type==VAL_NUM, "akarKuadrat() butuh angka");
                          LOOP_ASSERT(args[0]->num >= 0, "Baris %d: akarKuadrat() dari angka negatif", line);
                          return val_num(sqrt(args[0]->num));
                      }

                      static Val *builtin_PI(Val **args, int n, int line) {
                          return val_num(3.14159265358979323846);
                      }

                      /* ---------------------------------------------------------------
                       * Bitwise
                       * --------------------------------------------------------------- */

                      static Val *builtin_bitAND(Val **args, int n, int line) {
                          LOOP_ASSERT(n==2 && args[0]->type==VAL_NUM && args[1]->type==VAL_NUM, "bitAND() butuh 2 angka");
                          return val_num((double)((int64_t)args[0]->num & (int64_t)args[1]->num));
                      }
                      static Val *builtin_bitOR(Val **args, int n, int line) {
                          LOOP_ASSERT(n==2 && args[0]->type==VAL_NUM && args[1]->type==VAL_NUM, "bitOR() butuh 2 angka");
                          return val_num((double)((int64_t)args[0]->num | (int64_t)args[1]->num));
                      }
                      static Val *builtin_bitXOR(Val **args, int n, int line) {
                          LOOP_ASSERT(n==2 && args[0]->type==VAL_NUM && args[1]->type==VAL_NUM, "bitXOR() butuh 2 angka");
                          return val_num((double)((int64_t)args[0]->num ^ (int64_t)args[1]->num));
                      }
                      static Val *builtin_bitKiri(Val **args, int n, int line) {
                          LOOP_ASSERT(n==2 && args[0]->type==VAL_NUM && args[1]->type==VAL_NUM, "bitKiri() butuh 2 angka");
                          return val_num((double)((int64_t)args[0]->num << (int64_t)args[1]->num));
                      }
                      static Val *builtin_bitKanan(Val **args, int n, int line) {
                          LOOP_ASSERT(n==2 && args[0]->type==VAL_NUM && args[1]->type==VAL_NUM, "bitKanan() butuh 2 angka");
                          return val_num((double)((int64_t)args[0]->num >> (int64_t)args[1]->num));
                      }
                      static Val *builtin_bitNOT(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1 && args[0]->type==VAL_NUM, "bitNOT() butuh 1 angka");
                          return val_num((double)(~(int64_t)args[0]->num));
                      }

                      /* ---------------------------------------------------------------
                       * Meta / sistem
                       * --------------------------------------------------------------- */

                      static Val *builtin_sistemEnvGet(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1 && args[0]->type==VAL_STR, "_sistemEnvGet() butuh nama teks");
                          const char *v = getenv(args[0]->str);
                          return val_str(v ? v : "");
                      }
                      static Val *builtin_sistemEnvSet(Val **args, int n, int line) {
                          LOOP_ASSERT(n==2 && args[0]->type==VAL_STR && args[1]->type==VAL_STR,
                                      "_sistemEnvSet() butuh (nama, nilai)");
                          return val_bool(setenv(args[0]->str, args[1]->str, 1) == 0);
                      }
                      static Val *builtin_sistemEnvHapus(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1 && args[0]->type==VAL_STR, "_sistemEnvHapus() butuh nama teks");
                          return val_bool(unsetenv(args[0]->str) == 0);
                      }
                      static Val *builtin_sistemEnvSemua(Val **args, int n, int line) {
                          int cnt = 0; for (char **e = environ; *e; e++) cnt++;
                          Val **out = cnt > 0 ? malloc(sizeof(Val*) * (size_t)cnt) : NULL;
                          for (int i = 0; i < cnt; i++) out[i] = val_str(environ[i]);
                          return val_array(out, cnt);
                      }
                      static Val *builtin_sistemArgumen(Val **args, int n, int line) {
                          return val_ref(env_get(g_global_env, "argumen"));
                      }
                      static Val *builtin_sistemDirKerja(Val **args, int n, int line) {
                          char buf[4096];
                          if (!getcwd(buf, sizeof(buf))) return val_str("");
                          return val_str(buf);
                      }
                      static Val *builtin_sistemPindahDir(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1 && args[0]->type==VAL_STR, "_sistemPindahDir() butuh path teks");
                          return val_bool(chdir(args[0]->str) == 0);
                      }
                      static Val *builtin_sistemPathSep(Val **args, int n, int line) {
                          return val_str("/");
                      }
                      static Val *builtin_sistemNamaOS(Val **args, int n, int line) {
                          return val_str("linux");
                      }
                      static Val *builtin_sistemArsitektur(Val **args, int n, int line) {
                          struct utsname u;
                          if (uname(&u) != 0) return val_str("?");
                          return val_str(u.machine);
                      }
                      static Val *builtin_sistemJumlahCPU(Val **args, int n, int line) {
                          long c = sysconf(_SC_NPROCESSORS_ONLN);
                          return val_num(c > 0 ? (double)c : 1);
                      }
                      static Val *builtin_sistemPID(Val **args, int n, int line) {
                          return val_num((double)getpid());
                      }
                      static Val *builtin_sistemKeluar(Val **args, int n, int line) {
                          int code = (n >= 1 && args[0]->type==VAL_NUM) ? (int)args[0]->num : 0;
                          exit(code);
                          return val_null();
                      }
                      static Val *builtin_sistemJalankan(Val **args, int n, int line) {
                          LOOP_ASSERT(n==1 && args[0]->type==VAL_STR, "_sistemJalankan() butuh teks perintah");
                          int rc = system(args[0]->str);
                          if (rc == -1) return val_num(-1);
                          if (WIFEXITED(rc)) return val_num(WEXITSTATUS(rc));
                          return val_num(-1);
                      }

                      /* Tabel builtin */
                      typedef struct { const char *nama; BuiltinFn fn; } BuiltinEntry;
                      static const BuiltinEntry BUILTINS[] = {
                          {"panjang",    builtin_panjang},
                          {"jumlah",     builtin_jumlah},
                          {"maks",       builtin_maks},
                          {"min",        builtin_min},
                          {"rerata",     builtin_rerata},
                          {"urut",       builtin_urut},
                          {"balik",      builtin_balik},
                          {"angkaTeks",  builtin_angkaTeks},
                          {"teksAngka",  builtin_teksAngka},
                          {"masukan",    builtin_masukan},
                          {"tipe",       builtin_tipe},
                          {"tambah",     builtin_tambah},
                          {"gabung",     builtin_gabung},
                          {"prima",      builtin_prima},
                          {"acak",       builtin_acak},
                          {"ganda",      builtin_ganda},
                          {"kodeKarakter",     builtin_kodeKarakter},
                          {"karakter",         builtin_karakter},
                          {"bulatkan",         builtin_bulatkan},
                          {"irisLarik",        builtin_irisLarik},
                          {"bacaFileTeks",     builtin_bacaFileTeks},
                          {"tulisFileTeks",    builtin_tulisFileTeks},
                          {"gabungTeks",       builtin_gabungTeks},
                          {"cetakTanpaBaris",  builtin_cetakTanpaBaris},
                          {"eksekusi",          builtin_eksekusi},
                          {"sambung",          builtin_sambung},
                          {"tlsSambung",       builtin_tlsSambung},
                          {"kirim",            builtin_kirim},
                          {"terima",           builtin_terima},
                          {"tutupKoneksi",     builtin_tutupKoneksi},
                          {"ambil",            builtin_ambil},
                          {"telusuri",           builtin_telusuri},

                          /* IO tambahan */
                          {"tambahFileTeks",   builtin_tambahFileTeks},
                          {"appendFile",       builtin_appendFile},
                          {"tulis",            builtin_tulis},
                          {"tulisInline",      builtin_tulisInline},
                          {"tulisStderr",      builtin_tulisStderr},
                          {"tulisError",       builtin_tulisError},
                          {"barisBaru",        builtin_barisBaru},
                          {"waktumilisdetik",  builtin_waktumilisdetik},
                          {"waktuSekarang",    builtin_waktuSekarang},
                          {"waktuISO",         builtin_waktuISO},
                          {"tidur",            builtin_tidur},

                          /* String tambahan */
                          {"keAngka",          builtin_keAngka},
                          {"keDesimal",        builtin_keDesimal},
                          {"keTeks",           builtin_keTeks},
                          {"dariKode",         builtin_dariKode},
                          {"potong",           builtin_potong},
                          {"cariTeks",         builtin_cariTeks},
                          {"gantiTeks",        builtin_gantiTeks},
                          {"ganti",            builtin_gantiTeks},
                          {"hurufBesar",       builtin_hurufBesar},
                          {"hurufKecil",       builtin_hurufKecil},
                          {"naik",             builtin_hurufBesar},
                          {"turunkan",         builtin_hurufKecil},
                          {"pisah",            builtin_pisah},
                          {"trimTeks",         builtin_trimTeks},
                          {"trim",             builtin_trimTeks},
                          {"mulaiDengan",      builtin_mulaiDengan},
                          {"akhiriDengan",     builtin_akhiriDengan},
                          {"regexGanti",       builtin_regexGanti},
                          {"regexCocok",       builtin_regexCocok},
                          {"terakhirIndexDari",builtin_terakhirIndexDari},

                          /* Larik tambahan */
                          {"dorong",           builtin_dorong},
                          {"hapusTerakhir",    builtin_hapusTerakhir},
                          {"hapusKunci",       builtin_hapusKunci},
                          {"hapusLarik",       builtin_hapusLarik},
                          {"iris",             builtin_iris},
                          {"adaDalam",         builtin_adaDalam},
                          {"saring",           builtin_saring},
                          {"peta",             builtin_peta},
                          {"kurangi",          builtin_kurangi},
                          {"jenisNilai",       builtin_tipe},
                          {"kunciObjek",       builtin_kunciObjek},
                          {"salin",            builtin_salin},

                          /* Matematika tambahan */
                          {"lantai",           builtin_lantai},
                          {"langit",           builtin_langit},
                          {"absolut",          builtin_absolut},
                          {"acakAngka",        builtin_acakAngka},
                          {"log",              builtin_log},
                          {"kosinus",          builtin_kosinus},
                          {"sinus",            builtin_sinus},
                          {"akarKuadrat",      builtin_akarKuadrat},
                          {"PI",               builtin_PI},

                          /* Bitwise */
                          {"bitAND",           builtin_bitAND},
                          {"bitOR",            builtin_bitOR},
                          {"bitXOR",           builtin_bitXOR},
                          {"bitKiri",          builtin_bitKiri},
                          {"bitKanan",         builtin_bitKanan},
                          {"bitNOT",           builtin_bitNOT},

                          /* Meta / sistem */
                          {"_sistemEnvGet",    builtin_sistemEnvGet},
                          {"_sistemEnvSet",    builtin_sistemEnvSet},
                          {"_sistemEnvHapus",  builtin_sistemEnvHapus},
                          {"_sistemEnvSemua",  builtin_sistemEnvSemua},
                          {"_sistemArgumen",   builtin_sistemArgumen},
                          {"_sistemDirKerja",  builtin_sistemDirKerja},
                          {"_sistemPindahDir", builtin_sistemPindahDir},
                          {"_sistemPathSep",   builtin_sistemPathSep},
                          {"_sistemNamaOS",    builtin_sistemNamaOS},
                          {"_sistemArsitektur",builtin_sistemArsitektur},
                          {"_sistemJumlahCPU", builtin_sistemJumlahCPU},
                          {"_sistemPID",       builtin_sistemPID},
                          {"_sistemKeluar",    builtin_sistemKeluar},
                          {"_sistemJalankan",  builtin_sistemJalankan},

                          {NULL, NULL}
                      };

                      static BuiltinFn find_builtin(const char *name) {
                          for (int i = 0; BUILTINS[i].nama; i++)
                              if (strcmp(BUILTINS[i].nama, name) == 0) return BUILTINS[i].fn;
                              return NULL;
                      }

                      /* Validasi tipe eksplisit */
                      static void validasi_tipe(const char *tipe, Val *v, const char *nama, int line) {
                          if (!tipe[0]) return;
                          if (strcmp(tipe,"angka")==0 && v->type != VAL_NUM)
                              loop_error("Baris %d: Variabel '%s' harus angka", line, nama);
                          if (strcmp(tipe,"teks")==0 && v->type != VAL_STR)
                              loop_error("Baris %d: Variabel '%s' harus teks", line, nama);
                          if (strcmp(tipe,"bool")==0 && v->type != VAL_BOOL)
                              loop_error("Baris %d: Variabel '%s' harus bool", line, nama);
                          if (strcmp(tipe,"larik")==0 && v->type != VAL_ARRAY)
                              loop_error("Baris %d: Variabel '%s' harus larik", line, nama);
                      }

                      /* eval utama */

                      static Val *eval_panggil(const char *fname, Node **arg_nodes, int n_args, Env *env, int line);

                      static Val *eval(Node *n, Env *env) {
                          if (!n) return val_null();

                          switch (n->type) {

                              case N_LIT_NUM:  return val_num(n->num);
                              case N_LIT_BOOL: return val_bool(n->bval);
                              case N_LIT_NULL: return val_null();

                              case N_LIT_STR:  return val_str(n->str);

                              case N_STRING_INTERP: {
                                  /* Build string dari bagian-bagian */
                                  char result[MAX_STRING*2]; result[0]=0; int off=0;
                                  for (int i=0;i<n->n_interp;i++) {
                                      InterpPart *p = &n->interp[i];
                                      if (!p->is_expr) {
                                          off += snprintf(result+off, sizeof(result)-off, "%s", p->text);
                                      } else {
                                          /* Parse & eval ekspresi dalam #{} */
                                          Lexer *L2 = calloc(1, sizeof(Lexer));
                                          if (!L2) loop_error("Out of memory (string interp lexer)");
                                          L2->src = p->text;
                                          tokenize(L2);
                                          Parser P2; P2.tokens = L2->tokens; P2.pos=0; P2.n=L2->n_tokens;
                                          Node *expr = parse_expr(&P2);
                                          free(L2);
                                          Val *v = eval(expr, env);
                                          char tmp[MAX_STRING]; val_format(v, tmp, MAX_STRING);
                                          off += snprintf(result+off, sizeof(result)-off, "%s", tmp);
                                          val_free(v);
                                      }
                                  }
                                  return val_str(result);
                              }

                              case N_VAR: return val_ref(env_get(env, n->str));

                              case N_ARRAY: {
                                  Val **arr = malloc(sizeof(Val*)*n->n_elems);
                                  for (int i=0;i<n->n_elems;i++) arr[i] = eval(n->elems[i], env);
                                  return val_array(arr, n->n_elems);
                              }

                              case N_INDEX: {
                                  Val *obj = eval(n->obj, env);
                                  Val *idx = eval(n->idx, env);
                                  LOOP_ASSERT(obj->type==VAL_ARRAY, "Baris %d: Indeks hanya untuk larik", n->line);
                                  int i = (int)idx->num;
                                  LOOP_ASSERT(i>=0 && i<obj->arr_n, "Baris %d: Indeks %d di luar batas (panjang %d)", n->line, i, obj->arr_n);
                                  Val *r = val_ref(obj->arr[i]);
                                  val_free(obj); val_free(idx); return r;
                              }

                              case N_BINOP: {
                                  Val *L, *R;
                                  /* Short-circuit untuk dan/atau */
                                  if (strcmp(n->op,"dan")==0) {
                                      L = eval(n->left, env);
                                      int lb = (L->type==VAL_BOOL ? L->bval : L->type!=VAL_NULL && !(L->type==VAL_NUM && L->num==0));
                                      val_free(L);
                                      if (!lb) return val_bool(0);
                                      R = eval(n->right, env);
                                      int rb = (R->type==VAL_BOOL ? R->bval : R->type!=VAL_NULL && !(R->type==VAL_NUM && R->num==0));
                                      val_free(R); return val_bool(rb);
                                  }
                                  if (strcmp(n->op,"atau")==0) {
                                      L = eval(n->left, env);
                                      int lb = (L->type==VAL_BOOL ? L->bval : L->type!=VAL_NULL && !(L->type==VAL_NUM && L->num==0));
                                      if (lb) { val_free(L); return val_bool(1); }
                                      val_free(L);
                                      R = eval(n->right, env);
                                      int rb = (R->type==VAL_BOOL ? R->bval : R->type!=VAL_NULL && !(R->type==VAL_NUM && R->num==0));
                                      val_free(R); return val_bool(rb);
                                  }

                                  L = eval(n->left, env);
                                  R = eval(n->right, env);
                                  Val *res = NULL;

                                  if (strcmp(n->op,"+")==0) {
                                      if (L->type==VAL_STR || R->type==VAL_STR) {
                                          char a[MAX_STRING], b[MAX_STRING];
                                          val_format(L, a, MAX_STRING); val_format(R, b, MAX_STRING);
                                          char *s = malloc(strlen(a)+strlen(b)+1);
                                          strcpy(s,a); strcat(s,b); res = val_str(s); free(s);
                                      } else res = val_num(L->num + R->num);
                                  }
                                  else if (strcmp(n->op,"-")==0) res = val_num(L->num - R->num);
                                  else if (strcmp(n->op,"*")==0) res = val_num(L->num * R->num);
                                  else if (strcmp(n->op,"/")==0) {
                                      if (R->num == 0) { snprintf(g_error_msg, MAX_STRING, "Pembagian dengan nol"); g_error=1; res=val_null(); }
                                      else res = val_num(L->num / R->num);
                                  }
                                  else if (strcmp(n->op,"%")==0) res = val_num(fmod(L->num, R->num));
                                  else if (strcmp(n->op,"==")==0) {
                                      if (L->type==VAL_STR && R->type==VAL_STR) res = val_bool(strcmp(L->str,R->str)==0);
                                      else res = val_bool(L->num == R->num);
                                  }
                                  else if (strcmp(n->op,"!=")==0) {
                                      if (L->type==VAL_STR && R->type==VAL_STR) res = val_bool(strcmp(L->str,R->str)!=0);
                                      else res = val_bool(L->num != R->num);
                                  }
                                  else if (strcmp(n->op,"<")==0)  res = val_bool(L->num <  R->num);
                                  else if (strcmp(n->op,">")==0)  res = val_bool(L->num >  R->num);
                                  else if (strcmp(n->op,"<=")==0) res = val_bool(L->num <= R->num);
                                  else if (strcmp(n->op,">=")==0) res = val_bool(L->num >= R->num);
                                  else loop_error("Baris %d: Operator tak dikenal: '%s'", n->line, n->op);

                                  val_free(L); val_free(R); return res;
                              }

                              case N_UNIOP: {
                                  Val *v = eval(n->left, env);
                                  if (strcmp(n->op,"-")==0)     { Val *r = val_num(-v->num); val_free(v); return r; }
                                  if (strcmp(n->op,"tidak")==0) {
                                      int b = (v->type==VAL_BOOL) ? !v->bval : (v->type==VAL_NULL || (v->type==VAL_NUM&&v->num==0));
                                      val_free(v); return val_bool(b);
                                  }
                                  val_free(v); return val_null();
                              }

                              case N_ASSIGN: {
                                  Val *v = eval(n->right, env);
                                  if (n->var_tipe && n->var_tipe[0]) validasi_tipe(n->var_tipe, v, n->varname, n->line);
                                  env_set(env, n->varname, v);
                                  Val *r = val_ref(v); val_free(v); return r;
                              }

                              case N_ASSIGN_COMPOUND: {
                                  Val *cur = val_ref(env_get(env, n->varname));
                                  Val *rhs = eval(n->right, env);
                                  Val *res;
                                  char op = n->op[0];
                                  if (op=='+') {
                                      if (cur->type==VAL_STR) {
                                          char a[MAX_STRING], b[MAX_STRING];
                                          val_format(cur, a, MAX_STRING); val_format(rhs, b, MAX_STRING);
                                          char *s = malloc(strlen(a)+strlen(b)+1);
                                          strcpy(s,a); strcat(s,b); res = val_str(s); free(s);
                                      } else res = val_num(cur->num + rhs->num);
                                  }
                                  else if (op=='-') res = val_num(cur->num - rhs->num);
                                  else if (op=='*') res = val_num(cur->num * rhs->num);
                                  else if (op=='/') res = rhs->num!=0 ? val_num(cur->num/rhs->num) : val_num(0);
                                  else if (op=='%') res = val_num(fmod(cur->num, rhs->num));
                                  else res = val_null();
                                  env_set(env, n->varname, res);
                                  val_free(cur); val_free(rhs);
                                  Val *r = val_ref(res); val_free(res); return r;
                              }

                              case N_CETAK: {
                                  Val *v = eval(n->left, env);
                                  char buf[MAX_STRING*2]; val_format(v, buf, sizeof(buf));
                                  printf("%s\n", buf);
                                  Val *r = val_ref(v); val_free(v); return r;
                              }

                              case N_JALANKAN: {
                                  Val *v = eval(n->left, env);
                                  if (v->type != VAL_STR) {
                                      snprintf(g_error_msg, MAX_STRING, "jalankan() butuh teks perintah");
                                      g_error=1; val_free(v); return val_null();
                                  }
                                  int rc = system(v->str);
                                  Val *r;
                                  if (rc == -1) r = val_num(-1);
                                  else if (WIFEXITED(rc)) r = val_num(WEXITSTATUS(rc));
                                  else r = val_num(-1);
                                  val_free(v); return r;
                              }

                              case N_BACA_FILE: {
                                  /* PENTING: bacaFile() EKSEKUSI file sebagai kode Loop (seperti import/exec Python),
                                   * bukan membaca file sebagai teks. Untuk baca teks, gunakan bacaFileTeks() */
                                  FILE *f = fopen(n->str, "r");
                                  if (!f) { snprintf(g_error_msg, MAX_STRING, "File tidak ditemukan: %s", n->str); g_error=1; return val_null(); }
                                  fseek(f, 0, SEEK_END); long sz = ftell(f); rewind(f);
                                  char *kode = malloc(sz+1); fread(kode, 1, sz, f); kode[sz]=0; fclose(f);
                                  Val *r = run_kode(kode, env);
                                  free(kode); return r;
                              }

                              case N_TULIS_FILE: {
                                  Val *isi = eval(n->left, env);
                                  char buf[MAX_STRING]; val_format(isi, buf, sizeof(buf));
                                  FILE *f = fopen(n->str, "w");
                                  if (!f) { snprintf(g_error_msg, MAX_STRING, "Tidak bisa buka file: %s", n->str); g_error=1; val_free(isi); return val_null(); }
                                  fprintf(f, "%s", buf); fclose(f);
                                  val_free(isi); return val_null();
                              }

                              case N_PAKAI: {
                                  /* Sudah pernah di-'pakai' di sesi ini? Skip diam-diam,
                                   * persis kayak Python gak nge-reimport modul yang sama. */
                                  for (int i = 0; i < g_pakai_n_loaded; i++) {
                                      if (strcmp(g_pakai_loaded[i], n->str) == 0) return val_null();
                                  }
                                  char path[PATH_MAX];
                                  snprintf(path, sizeof(path), "%s/%s.lp", g_stdlib_dir, n->str);
                                  FILE *f = fopen(path, "r");
                                  if (!f) {
                                      snprintf(g_error_msg, MAX_STRING,
                                               "pakai: modul '%s' tidak ditemukan (dicari di %s)",
                                               n->str, path);
                                      g_error = 1; return val_null();
                                  }
                                  fseek(f, 0, SEEK_END); long sz = ftell(f); rewind(f);
                                  char *kode = malloc(sz + 1);
                                  fread(kode, 1, sz, f); kode[sz] = 0; fclose(f);
                                  Val *r = run_kode(kode, env);
                                  free(kode);
                                  if (g_pakai_n_loaded < 64) {
                                      g_pakai_loaded[g_pakai_n_loaded++] = strdup(n->str);
                                  }
                                  return r;
                              }

                              case N_FUNGSI:
                                  env_set(env, n->fname, val_func(n));
                                  return val_null();

                              case N_PANGGIL: {
                                  return eval_panggil(n->fname, n->args, n->n_args, env, n->line);
                              }

                              case N_KEMBALI: {
                                  g_kembali_v = eval(n->left, env);
                                  g_kembali = 1; return val_null();
                              }

                              case N_HENTIKAN: g_hentikan = 1; return val_null();
                              case N_LEWATI:   g_lewati   = 1; return val_null();

                              case N_JIKA: {
                                  Val *cond = eval(n->cond, env);
                                  int ok = (cond->type==VAL_BOOL) ? cond->bval : (cond->type!=VAL_NULL && !(cond->type==VAL_NUM&&cond->num==0));
                                  val_free(cond);
                                  Env *sub = env_new(env);
                                  if (ok) {
                                      for (int i=0;i<n->n_body&&!g_kembali&&!g_hentikan&&!g_lewati&&!g_error;i++)
                                          val_free(eval(n->body[i], sub));
                                  } else if (n->selain) {
                                      for (int i=0;i<n->n_selain&&!g_kembali&&!g_hentikan&&!g_lewati&&!g_error;i++)
                                          val_free(eval(n->selain[i], sub));
                                  }
                                  env_free(sub); return val_null();
                              }

                              case N_SELAMA: {
                                  for (;;) {
                                      Val *cond = eval(n->cond, env);
                                      int ok = (cond->type==VAL_BOOL) ? cond->bval : (cond->type!=VAL_NULL && !(cond->type==VAL_NUM&&cond->num==0));
                                      val_free(cond);
                                      if (!ok) break;
                                      Env *sub = env_new(env);
                                      for (int i=0;i<n->n_body&&!g_kembali&&!g_hentikan&&!g_lewati&&!g_error;i++)
                                          val_free(eval(n->body[i], sub));
                                      env_free(sub);
                                      if (g_hentikan) { g_hentikan=0; break; }
                                      if (g_lewati)   { g_lewati=0; continue; }
                                      if (g_kembali||g_error) break;
                                  }
                                  return val_null();
                              }

                              case N_ULANG: {
                                  Val *hv = eval(n->left, env);
                                  int count = (int)hv->num; val_free(hv);
                                  for (int ci=0;ci<count;ci++) {
                                      Env *sub = env_new(env);
                                      for (int i=0;i<n->n_body&&!g_kembali&&!g_hentikan&&!g_lewati&&!g_error;i++)
                                          val_free(eval(n->body[i], sub));
                                      env_free(sub);
                                      if (g_hentikan) { g_hentikan=0; break; }
                                      if (g_lewati)   { g_lewati=0; continue; }
                                      if (g_kembali||g_error) break;
                                  }
                                  return val_null();
                              }

                              case N_PILIH: {
                                  Val *eksp = eval(n->left, env);
                                  int matched = 0;
                                  for (int i=0;i<n->n_kasus;i++) {
                                      Val *kval = eval(n->kasus_val[i], env);
                                      int eq = 0;
                                      if (eksp->type==VAL_STR && kval->type==VAL_STR) eq = strcmp(eksp->str,kval->str)==0;
                                      else eq = (eksp->num == kval->num);
                                      val_free(kval);
                                      if (eq) {
                                          Env *sub = env_new(env);
                                          val_free(eval(n->kasus_body[i], sub));
                                          env_free(sub); matched=1; break;
                                      }
                                  }
                                  if (!matched && n->lainnya) {
                                      Env *sub = env_new(env);
                                      val_free(eval(n->lainnya, sub));
                                      env_free(sub);
                                  }
                                  val_free(eksp); return val_null();
                              }

                              case N_COBA: {
                                  int saved_error = g_error;
                                  g_error = 0;
                                  Env *sub = env_new(env);
                                  for (int i=0;i<n->n_body&&!g_kembali&&!g_hentikan&&!g_lewati&&!g_error;i++)
                                      val_free(eval(n->body[i], sub));
                                  env_free(sub);
                                  if (g_error) {
                                      g_error = 0;
                                      Env *catch_env = env_new(env);
                                      for (int i=0;i<n->n_tangkap&&!g_kembali&&!g_hentikan&&!g_lewati;i++)
                                          val_free(eval(n->tangkap[i], catch_env));
                                      env_free(catch_env);
                                  } else {
                                      g_error = saved_error;
                                  }
                                  return val_null();
                              }

                              case N_PIPA: {
                                  Val *kiri = eval(n->pipa_kiri, env);
                                  /* Kumpulkan args: kiri + extra */
                                  Val **pargs = malloc(sizeof(Val*)*(n->n_pipa_extra+1));
                                  pargs[0] = kiri;
                                  for (int i=0;i<n->n_pipa_extra;i++) pargs[i+1] = eval(n->pipa_extra[i], env);
                                  int total = n->n_pipa_extra + 1;
                                  /* Cek builtin */
                                  BuiltinFn bf = find_builtin(n->pipa_fn);
                                  Val *result;
                                  if (bf) {
                                      result = bf(pargs, total, n->line);
                                  } else {
                                      /* Fungsi user-defined */
                                      Val *fv = env_get(env, n->pipa_fn);
                                      LOOP_ASSERT(fv && fv->type==VAL_FUNC, "Baris %d: '%s' bukan fungsi", n->line, n->pipa_fn);
                                      Node *fn = fv->func_node;
                                      Env *fe = env_new(g_global_env);
                                      for (int i=0;i<fn->n_params&&i<total;i++)
                                          env_local_set(fe, fn->params[i], pargs[i]);
                                      for (int i=0;i<fn->n_body&&!g_kembali&&!g_error;i++)
                                          val_free(eval(fn->body[i], fe));
                                      result = g_kembali ? g_kembali_v : val_null();
                                      g_kembali=0; g_kembali_v=NULL;
                                      env_free(fe);
                                  }
                                  for (int i=0;i<total;i++) val_free(pargs[i]);
                                  free(pargs); return result;
                              }

                              case N_PROGRAM: {
                                  Val *last = val_null();
                                  for (int i=0;i<n->n_body&&!g_kembali&&!g_error;i++) {
                                      val_free(last); last = eval(n->body[i], env);
                                  }
                                  return last;
                              }

                              default:
                                  loop_error("Baris %d: Node tak dikenal: %d", n->line, n->type);
                          }
                          return val_null();
                      }

                      static Val *eval_panggil(const char *fname, Node **arg_nodes, int n_args, Env *env, int line) {
                          BuiltinFn bf = find_builtin(fname);
                          Val **args = malloc(sizeof(Val*)*n_args);
                          for (int i=0;i<n_args;i++) args[i] = eval(arg_nodes[i], env);

                          Val *result;
                          if (bf) {
                              result = bf(args, n_args, line);
                          } else {
                              Val *fv = env_get(env, fname);
                              LOOP_ASSERT(fv && fv->type==VAL_FUNC, "Baris %d: '%s' bukan fungsi", line, fname);
                              Node *fn = fv->func_node;
                              LOOP_ASSERT(n_args == fn->n_params,
                                          "Baris %d: Fungsi '%s' butuh %d argumen, dapat %d", line, fname, fn->n_params, n_args);
                              Env *fe = env_new(g_global_env);
                              for (int i=0;i<fn->n_params;i++) env_local_set(fe, fn->params[i], args[i]);
                              for (int i=0;i<fn->n_body&&!g_kembali&&!g_error;i++)
                                  val_free(eval(fn->body[i], fe));
                              result = g_kembali ? g_kembali_v : val_null();
                              g_kembali=0; g_kembali_v=NULL;
                              env_free(fe);
                          }
                          for (int i=0;i<n_args;i++) val_free(args[i]);
                          free(args); return result;
                      }

                      /* Panggil sebuah Val bertipe VAL_FUNC dengan argumen yang SUDAH dievaluasi
                       *  (dipakai builtin higher-order: peta, saring, kurangi). Tidak meng-free
                       *  args (pemanggil yang punya kepemilikan). */
                      static Val *panggil_nilai_fungsi(Val *fv, Val **args, int n_args, int line) {
                          LOOP_ASSERT(fv && fv->type==VAL_FUNC, "Baris %d: nilai bukan fungsi", line);
                          Node *fn = fv->func_node;
                          LOOP_ASSERT(n_args == fn->n_params,
                                      "Baris %d: Fungsi '%s' butuh %d argumen, dapat %d",
                                      line, fn->fname ? fn->fname : "?", fn->n_params, n_args);
                          Env *fe = env_new(g_global_env);
                          for (int i=0;i<fn->n_params;i++) env_local_set(fe, fn->params[i], args[i]);
                          for (int i=0;i<fn->n_body&&!g_kembali&&!g_error;i++)
                              val_free(eval(fn->body[i], fe));
                          Val *result = g_kembali ? g_kembali_v : val_null();
                          g_kembali=0; g_kembali_v=NULL;
                          env_free(fe);
                          return result;
                      }

                      static Val *run_kode(const char *kode, Env *env) {
                          Lexer *L = calloc(1, sizeof(Lexer));
                          if (!L) loop_error("Out of memory (run_kode lexer)");
                          L->src = kode; tokenize(L);
                          Node *prog = parse_program(L->tokens, L->n_tokens);
                          free(L);
                          return eval(prog, env);
                      }

                      typedef struct {
                          char   data[1<<20];   /* .data section */
                          int    dlen;
                          char   text[1<<22];   /* .text section */
                          int    tlen;
                          char   bss[4096];
                          int    bsslen;
                          int    label_cnt;
                          int    str_cnt;
                          /* Variable offsets (stack) */
                          char   varnames[MAX_ENV_VARS][MAX_IDENT];
                          int    varoffs[MAX_ENV_VARS];
                          int    n_vars;
                          int    stack_sz;
                          int    in_func;
                          char   func_end[MAX_IDENT];
                      } Codegen;

                      #define CG_EMIT(cg, ...) do { \
                      char _buf[512]; snprintf(_buf, 512, __VA_ARGS__); \
                      int _l = strlen(_buf); memcpy((cg)->text+(cg)->tlen, _buf, _l); \
                      (cg)->tlen += _l; (cg)->text[(cg)->tlen] = 0; \
                      } while(0)

                      #define CG_DATA(cg, ...) do { \
                      char _buf[512]; snprintf(_buf, 512, __VA_ARGS__); \
                      int _l = strlen(_buf); memcpy((cg)->data+(cg)->dlen, _buf, _l); \
                      (cg)->dlen += _l; (cg)->data[(cg)->dlen] = 0; \
                      } while(0)

                      static char *cg_new_label(Codegen *cg, const char *pfx) {
                          static char lbl[MAX_IDENT];
                          snprintf(lbl, MAX_IDENT, "_%s%d", pfx, ++cg->label_cnt);
                          return lbl;
                      }

                      static char *cg_str_label(Codegen *cg, const char *s) {
                          static char lbl[MAX_IDENT];
                          snprintf(lbl, MAX_IDENT, "__str%d", ++cg->str_cnt);
                          /* Escape string untuk NASM */
                          char escaped[MAX_STRING*4]; int ei = 0;
                          for (int i = 0; s[i]; i++) {
                              unsigned char c = (unsigned char)s[i];
                              if (c == '\n') { memcpy(escaped+ei, "', 0xA, '", 9); ei+=9; }
                              else if (c == '\t') { memcpy(escaped+ei, "', 0x9, '", 9); ei+=9; }
                              else if (c == '\'') { memcpy(escaped+ei, "', 0x27, '", 10); ei+=10; }
                              else { escaped[ei++] = c; }
                          }
                          escaped[ei] = 0;
                          int bytes = strlen(s) + 2; /* +newline +null */
                          CG_DATA(cg, "    %s db '%s', 0xA, 0    ; len=%d\n", lbl, escaped, bytes);
                          return lbl;
                      }

                      static int cg_alloc_var(Codegen *cg, const char *name) {
                          for (int i=0;i<cg->n_vars;i++)
                              if (strcmp(cg->varnames[i], name)==0) return cg->varoffs[i];
                              cg->stack_sz += 8;
                          int off = -cg->stack_sz;
                          strncpy(cg->varnames[cg->n_vars], name, MAX_IDENT);
                          cg->varoffs[cg->n_vars++] = off;
                          return off;
                      }

                      static int cg_var_off(Codegen *cg, const char *name) {
                          for (int i=0;i<cg->n_vars;i++)
                              if (strcmp(cg->varnames[i], name)==0) return cg->varoffs[i];
                              return cg_alloc_var(cg, name);
                      }

                      static void cg_expr(Codegen *cg, Node *n);
                      static void cg_stmt(Codegen *cg, Node *n);

                      static void cg_expr(Codegen *cg, Node *n) {
                          if (!n) { CG_EMIT(cg, "    xor  rax, rax\n"); return; }
                          switch (n->type) {
                              case N_LIT_NUM:
                                  if (n->num == (int64_t)n->num)
                                      CG_EMIT(cg, "    mov  rax, %lld\n", (long long)(int64_t)n->num);
                              else
                                  CG_EMIT(cg, "    mov  rax, 0  ; float %.6g (integer truncated)\n", n->num);
                              break;
                              case N_LIT_BOOL:
                                  CG_EMIT(cg, "    mov  rax, %d\n", n->bval); break;
                              case N_LIT_NULL:
                                  CG_EMIT(cg, "    xor  rax, rax\n"); break;
                              case N_LIT_STR: {
                                  char *lbl = cg_str_label(cg, n->str);
                                  CG_EMIT(cg, "    mov  rax, %s\n", lbl); break;
                              }
                              case N_VAR: {
                                  int off = cg_var_off(cg, n->str);
                                  CG_EMIT(cg, "    mov  rax, qword [rbp%+d]\n", off); break;
                              }
                              case N_UNIOP:
                                  cg_expr(cg, n->left);
                                  if (strcmp(n->op,"-")==0)     CG_EMIT(cg, "    neg  rax\n");
                                  else if (strcmp(n->op,"tidak")==0) {
                                      CG_EMIT(cg, "    test rax, rax\n    setz al\n    movzx rax, al\n");
                                  }
                                  break;
                              case N_BINOP: {
                                  const char *op = n->op;
                                  if (strcmp(op,"dan")==0) {
                                      char lf[MAX_IDENT], le[MAX_IDENT];
                                      snprintf(lf, MAX_IDENT, "_%d_df", ++cg->label_cnt);
                                      snprintf(le, MAX_IDENT, "_%d_de", cg->label_cnt);
                                      cg_expr(cg, n->left);
                                      CG_EMIT(cg, "    test rax, rax\n    jz %s\n", lf);
                                      cg_expr(cg, n->right);
                                      CG_EMIT(cg, "    test rax, rax\n    jz %s\n    mov rax, 1\n    jmp %s\n%s:\n    xor rax, rax\n%s:\n", lf, le, lf, le);
                                      break;
                                  }
                                  if (strcmp(op,"atau")==0) {
                                      char lt[MAX_IDENT], le[MAX_IDENT];
                                      snprintf(lt, MAX_IDENT, "_%d_ot", ++cg->label_cnt);
                                      snprintf(le, MAX_IDENT, "_%d_oe", cg->label_cnt);
                                      cg_expr(cg, n->left);
                                      CG_EMIT(cg, "    test rax, rax\n    jnz %s\n", lt);
                                      cg_expr(cg, n->right);
                                      CG_EMIT(cg, "    test rax, rax\n    jnz %s\n    xor rax, rax\n    jmp %s\n%s:\n    mov rax, 1\n%s:\n", lt, le, lt, le);
                                      break;
                                  }
                                  cg_expr(cg, n->left); CG_EMIT(cg, "    push rax\n");
                                  cg_expr(cg, n->right); CG_EMIT(cg, "    mov  rcx, rax\n    pop  rax\n");

                                  if (strcmp(op,"+")==0) CG_EMIT(cg, "    add  rax, rcx\n");
                                  else if (strcmp(op,"-")==0) CG_EMIT(cg, "    sub  rax, rcx\n");
                                  else if (strcmp(op,"*")==0) CG_EMIT(cg, "    imul rax, rcx\n");
                                  else if (strcmp(op,"/")==0) CG_EMIT(cg, "    cqo\n    idiv rcx\n");
                                  else if (strcmp(op,"%")==0) CG_EMIT(cg, "    cqo\n    idiv rcx\n    mov  rax, rdx\n");
                                  else {
                                      char ct[MAX_IDENT], ce[MAX_IDENT];
                                      snprintf(ct, MAX_IDENT, "_%d_ct", ++cg->label_cnt);
                                      snprintf(ce, MAX_IDENT, "_%d_ce", cg->label_cnt);
                                      CG_EMIT(cg, "    cmp  rax, rcx\n");
                                      const char *jmp = "je";
                                      if (strcmp(op,"!=")==0) jmp="jne";
                                      else if (strcmp(op,"<")==0) jmp="jl";
                                      else if (strcmp(op,">")==0) jmp="jg";
                                      else if (strcmp(op,"<=")==0) jmp="jle";
                                      else if (strcmp(op,">=")==0) jmp="jge";
                                      CG_EMIT(cg, "    %s %s\n    xor  rax, rax\n    jmp  %s\n%s:\n    mov  rax, 1\n%s:\n", jmp, ct, ce, ct, ce);
                                  }
                                  break;
                              }
                              case N_PANGGIL: {
                                  const char *regs[] = {"rdi","rsi","rdx","rcx","r8","r9"};
                                  if (strcmp(n->fname,"tambah")==0 && n->n_args==2) {
                                      cg_expr(cg, n->args[0]); CG_EMIT(cg, "    push rax\n");
                                      cg_expr(cg, n->args[1]);
                                      CG_EMIT(cg, "    mov rcx, rax\n    pop rax\n    add rax, rcx\n");
                                      break;
                                  }
                                  for (int i=0;i<n->n_args&&i<6;i++) {
                                      cg_expr(cg, n->args[i]); CG_EMIT(cg, "    push rax\n");
                                  }
                                  for (int i=n->n_args-1;i>=0&&i<6;i--)
                                      CG_EMIT(cg, "    pop  %s\n", regs[i]);
                                  CG_EMIT(cg, "    call %s\n", n->fname);
                                  break;
                              }
                              case N_ASSIGN: {
                                  cg_expr(cg, n->right);
                                  int off = cg_alloc_var(cg, n->varname);
                                  CG_EMIT(cg, "    mov  qword [rbp%+d], rax\n", off);
                                  break;
                              }
                              default:
                                  CG_EMIT(cg, "    xor  rax, rax  ; unsupported expr node %d\n", n->type);
                          }
                      }

                      static const char *REGS[] = {"rdi","rsi","rdx","rcx","r8","r9"};

                      static void cg_func(Codegen *cg, Node *fn) {
                          /* Save state */
                          char sv_names[MAX_ENV_VARS][MAX_IDENT]; int sv_offs[MAX_ENV_VARS];
                          int sv_n = cg->n_vars, sv_sz = cg->stack_sz, sv_in = cg->in_func;
                          char sv_end[MAX_IDENT];
                          memcpy(sv_names, cg->varnames, sizeof(sv_names));
                          memcpy(sv_offs,  cg->varoffs,  sizeof(sv_offs));
                          strncpy(sv_end, cg->func_end, MAX_IDENT);

                          cg->n_vars=0; cg->stack_sz=0; cg->in_func=1;
                          snprintf(cg->func_end, MAX_IDENT, "_%s_end", fn->fname);

                          CG_EMIT(cg, "\n; fungsi %s\n%s:\n    push rbp\n    mov  rbp, rsp\n", fn->fname, fn->fname);
                          int ph = cg->tlen;
                          CG_EMIT(cg, "    sub  rsp, %-6d         ; placeholder\n", 0);

                          /* Params */
                          for (int i=0;i<fn->n_params&&i<6;i++) {
                              int off = cg_alloc_var(cg, fn->params[i]);
                              CG_EMIT(cg, "    mov  qword [rbp%+d], %s\n", off, REGS[i]);
                          }
                          /* Body */
                          for (int i=0;i<fn->n_body;i++) cg_stmt(cg, fn->body[i]);
                          CG_EMIT(cg, "    xor  rax, rax\n%s:\n", cg->func_end);
                          int aligned = ((cg->stack_sz+15)/16)*16;
                          if (aligned==0) aligned=16;
                          /* Patch placeholder */
                          char patch[32]; snprintf(patch, 32, "    sub  rsp, %d", aligned);
                          memcpy(cg->text+ph, patch, strlen(patch));
                          CG_EMIT(cg, "    mov  rsp, rbp\n    pop  rbp\n    ret\n");

                          /* Restore */
                          cg->n_vars = sv_n; cg->stack_sz = sv_sz; cg->in_func = sv_in;
                          memcpy(cg->varnames, sv_names, sizeof(sv_names));
                          memcpy(cg->varoffs,  sv_offs,  sizeof(sv_offs));
                          strncpy(cg->func_end, sv_end, MAX_IDENT);
                      }

                      static void cg_stmt(Codegen *cg, Node *n) {
                          if (!n) return;
                          switch (n->type) {
                              case N_FUNGSI: cg_func(cg, n); break;
                              case N_ASSIGN:
                              case N_ASSIGN_COMPOUND: {
                                  cg_expr(cg, n->right);
                                  int off = cg_alloc_var(cg, n->varname);
                                  /* Compound: load, op, store */
                                  if (n->type == N_ASSIGN_COMPOUND) {
                                      CG_EMIT(cg, "    mov  rcx, rax\n    mov  rax, qword [rbp%+d]\n", off);
                                      char op = n->op[0];
                                      if (op=='+') CG_EMIT(cg, "    add  rax, rcx\n");
                                      else if (op=='-') CG_EMIT(cg, "    sub  rax, rcx\n");
                                      else if (op=='*') CG_EMIT(cg, "    imul rax, rcx\n");
                                      else if (op=='/') CG_EMIT(cg, "    cqo\n    idiv rcx\n");
                                      else if (op=='%') CG_EMIT(cg, "    cqo\n    idiv rcx\n    mov rax, rdx\n");
                                  }
                                  CG_EMIT(cg, "    mov  qword [rbp%+d], rax\n", off);
                                  break;
                              }
                              case N_CETAK: {
                                  Node *e = n->left;
                                  if (e->type == N_LIT_STR) {
                                      char *lbl = cg_str_label(cg, e->str);
                                      int bytes = strlen(e->str) + 2;
                                      CG_EMIT(cg, "    mov  rax, 1\n    mov  rdi, 1\n    mov  rsi, %s\n    mov  rdx, %d\n    syscall\n", lbl, bytes);
                                  } else {
                                      cg_expr(cg, e);
                                      CG_EMIT(cg, "    mov  rdi, rax\n    call __print_int\n");
                                  }
                                  break;
                              }
                              case N_JIKA: {
                                  char lsel[MAX_IDENT], lend[MAX_IDENT];
                                  snprintf(lsel, MAX_IDENT, "_%d_jsel", ++cg->label_cnt);
                                  snprintf(lend, MAX_IDENT, "_%d_jend", cg->label_cnt);
                                  cg_expr(cg, n->cond);
                                  CG_EMIT(cg, "    test rax, rax\n    jz   %s\n", lsel);
                                  for (int i=0;i<n->n_body;i++) cg_stmt(cg, n->body[i]);
                                  CG_EMIT(cg, "    jmp  %s\n%s:\n", lend, lsel);
                                  for (int i=0;i<n->n_selain;i++) cg_stmt(cg, n->selain[i]);
                                  CG_EMIT(cg, "%s:\n", lend);
                                  break;
                              }
                              case N_SELAMA: {
                                  char lcek[MAX_IDENT], lend[MAX_IDENT];
                                  snprintf(lcek, MAX_IDENT, "_%d_wc", ++cg->label_cnt);
                                  snprintf(lend, MAX_IDENT, "_%d_we", cg->label_cnt);
                                  CG_EMIT(cg, "%s:\n", lcek);
                                  cg_expr(cg, n->cond);
                                  CG_EMIT(cg, "    test rax, rax\n    jz   %s\n", lend);
                                  for (int i=0;i<n->n_body;i++) cg_stmt(cg, n->body[i]);
                                  CG_EMIT(cg, "    jmp  %s\n%s:\n", lcek, lend);
                                  break;
                              }
                              case N_ULANG: {
                                  char lctr[MAX_IDENT], lcek[MAX_IDENT], lend[MAX_IDENT];
                                  snprintf(lctr, MAX_IDENT, "__uctr%d", ++cg->label_cnt);
                                  snprintf(lcek, MAX_IDENT, "_%d_uc", cg->label_cnt);
                                  snprintf(lend, MAX_IDENT, "_%d_ue", cg->label_cnt);
                                  cg_expr(cg, n->left);
                                  int off = cg_alloc_var(cg, lctr);
                                  CG_EMIT(cg, "    mov  qword [rbp%+d], rax\n%s:\n    cmp  qword [rbp%+d], 0\n    jle  %s\n",
                                          off, lcek, off, lend);
                                  for (int i=0;i<n->n_body;i++) cg_stmt(cg, n->body[i]);
                                  CG_EMIT(cg, "    dec  qword [rbp%+d]\n    jmp  %s\n%s:\n", off, lcek, lend);
                                  break;
                              }
                              case N_KEMBALI:
                                  cg_expr(cg, n->left);
                                  if (cg->func_end[0]) CG_EMIT(cg, "    jmp  %s\n", cg->func_end);
                                  break;
                              case N_PANGGIL:
                                  cg_expr(cg, n); break;
                              default:
                                  cg_expr(cg, n); break;
                          }
                      }

                      static void cg_helpers(Codegen *cg) {
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
                                  "    pop  rbp\n    ret\n"
                          );
                      }

                      static char *nasm_generate(Node *prog) {
                          Codegen *cg = calloc(1, sizeof(Codegen));
                          cg->bsslen = snprintf(cg->bss, sizeof(cg->bss),
                                                "section .bss\n    __intbuf resb 32\n");

                          /* Pre-scan fungsi */
                          CG_EMIT(cg, "\nglobal _start\n_start:\n    push rbp\n    mov  rbp, rsp\n");
                          int ph = cg->tlen;
                          CG_EMIT(cg, "    sub  rsp, %-6d         ; placeholder\n", 0);

                          /* Emit top-level statements (skip fungsi - will be emitted at end) */
                          for (int i=0;i<prog->n_body;i++) {
                              if (prog->body[i]->type != N_FUNGSI)
                                  cg_stmt(cg, prog->body[i]);
                          }
                          CG_EMIT(cg, "\n    xor  rdi, rdi\n    mov  rax, 60\n    syscall\n");

                          /* Patch stack size */
                          int aligned = ((cg->stack_sz+15)/16)*16;
                          if (aligned==0) aligned=16;
                          char patch[32]; snprintf(patch, 32, "    sub  rsp, %d", aligned);
                          memcpy(cg->text+ph, patch, strlen(patch));

                          /* Fungsi user-defined */
                          for (int i=0;i<prog->n_body;i++)
                              if (prog->body[i]->type == N_FUNGSI) cg_func(cg, prog->body[i]);

                              cg_helpers(cg);

                          /* Assemble output */
                          int outsz = 64 + cg->dlen + cg->bsslen + cg->tlen + 64;
                          char *out = malloc(outsz);
                          snprintf(out, outsz,
                                   "; Loop Compiler v%s - NASM x86-64 Linux\n"
                                   "section .data\n%s\n%s\nsection .text\n%s\n",
                                   LOOP_VERSI, cg->data, cg->bss, cg->text);

                          free(cg);
                          return out;
                      }

                      /* ---------------------------------------------------------------
                       * MAIN
                       * --------------------------------------------------------------- */

                      static char *baca_file(const char *path) {
                          FILE *f = fopen(path, "r");
                          if (!f) { fprintf(stderr, "[LOOP] File tidak ditemukan: %s\n", path); exit(1); }
                          fseek(f, 0, SEEK_END); long sz = ftell(f); rewind(f);
                          char *buf = malloc(sz+1); fread(buf, 1, sz, f); buf[sz]=0; fclose(f);
                          return buf;
                      }

                      /* Mode interaktif (REPL) */

                      /* Cek apakah token yang udah ke-scan masih "menggantung" (butuh baris lagi):
                       * '{' nunggu '}', 'jika' nunggu 'akhir'. Gak perlu presisi ketat - cukup
                       * buat mutusin lanjut baca baris atau udah waktunya coba parse. */
                      static int repl_masih_gantung(Token *toks, int n) {
                          int depth = 0;
                          for (int i = 0; i < n; i++) {
                              TokenType t = toks[i].type;
                              if (t == TK_LBRACE || t == TK_JIKA) depth++;
                              else if (t == TK_RBRACE || t == TK_AKHIR) depth--;
                          }
                          return depth > 0;
                      }

                      static int repl_node_adalah_stmt(NodeType t) {
                          return t == N_ASSIGN || t == N_ASSIGN_COMPOUND || t == N_CETAK ||
                          t == N_BACA_FILE || t == N_TULIS_FILE || t == N_PAKAI ||
                          t == N_JIKA || t == N_SELAMA || t == N_ULANG || t == N_PILIH ||
                          t == N_COBA || t == N_FUNGSI ||
                          t == N_KEMBALI || t == N_HENTIKAN || t == N_LEWATI || t == N_PROGRAM;
                      }

                      static void run_repl(void) {
                          printf("Loop %s (main, %s %s) [GCC %s] on linux\n",
                                 LOOP_VERSI, __DATE__, __TIME__, __VERSION__);
                          printf("Ketik pernyataan Loop. Ctrl-D buat keluar.\n");

                          g_global_env = env_new(NULL);
                          Val *argumen_val = val_array(NULL, 0);
                          env_set(g_global_env, "argumen", argumen_val);
                          val_free(argumen_val);

                          g_in_repl = 1;

                          static char accum[65536];
                          char linebuf[4096];
                          accum[0] = 0;

                          for (;;) {
                              if (setjmp(g_repl_jmp)) {
                                  g_error = 0;
                                  accum[0] = 0;
                              }

                              printf(accum[0] ? "... " : ">>> ");
                              fflush(stdout);

                              if (!fgets(linebuf, sizeof(linebuf), stdin)) {
                                  printf("\n");
                                  break; /* Ctrl-D */
                              }

                              if (strlen(accum) + strlen(linebuf) + 1 >= sizeof(accum)) {
                                  fprintf(stderr, "[LOOP] Baris terlalu panjang, direset.\n");
                                  accum[0] = 0;
                                  continue;
                              }
                              strcat(accum, linebuf);

                              int kosong = 1;
                              for (char *p = accum; *p; p++) {
                                  if (!isspace((unsigned char)*p)) { kosong = 0; break; }
                              }
                              if (kosong) { accum[0] = 0; continue; }

                              Lexer *L = calloc(1, sizeof(Lexer));
                              if (!L) { fprintf(stderr, "[LOOP] Fatal: out of memory\n"); break; }
                              L->src = accum;
                              tokenize(L);

                              if (repl_masih_gantung(L->tokens, L->n_tokens)) {
                                  free(L);
                                  continue;
                              }

                              Node *prog = parse_program(L->tokens, L->n_tokens);
                              free(L);

                              Val *last = NULL;
                              NodeType last_type = N_PROGRAM;
                              for (int i = 0; i < prog->n_body; i++) {
                                  if (last) val_free(last);
                                  last_type = prog->body[i]->type;
                                  last = eval(prog->body[i], g_global_env);
                                  if (g_error) {
                                      fprintf(stderr, "[LOOP ERROR] %s\n", g_error_msg);
                                      g_error = 0;
                                      last = NULL;
                                      break;
                                  }
                              }

                              if (last && last->type != VAL_NULL && !repl_node_adalah_stmt(last_type)) {
                                  char buf[MAX_STRING * 4];
                                  val_format(last, buf, sizeof(buf));
                                  printf("%s\n", buf);
                              }
                              if (last) val_free(last);

                              accum[0] = 0;
                          }

                          env_free(g_global_env);
                      }

                      int main(int argc, char **argv) {
                          init_stdlib_dir();

                          if (argc < 2) {
                              run_repl();
                              return 0;
                          }

                          const char *filepath = NULL;
                          int do_nasm  = 0;
                          int do_debug = 0;
                          const char *outfile = NULL;

                          for (int i = 1; i < argc; i++) {
                              if (strcmp(argv[i], "--nasm") == 0 ||
                                  strcmp(argv[i], "--kompilasi") == 0) {
                                  do_nasm = 1;
                                  } else if (strcmp(argv[i], "--target") == 0 && i + 1 < argc) {
                                      /* --target nasm (backward compat dengan extension lama) */
                                      if (strcmp(argv[i+1], "nasm") == 0) do_nasm = 1;
                                      i++; /* lewati argumen berikutnya */
                                  } else if (strcmp(argv[i], "--debug") == 0) {
                                      do_debug = 1;
                                  } else if ((strcmp(argv[i], "-o") == 0 ||
                                      strcmp(argv[i], "--output") == 0) && i + 1 < argc) {
                                      outfile = argv[++i];
                                      } else if (argv[i][0] != '-') {
                                          /* Cuma argumen non-flag PERTAMA yang jadi file dieksekusi.
                                           * Sisanya (mis. compiler.lp <target.lp>) dibiarkan utuh di
                                           * argv/argumen[] biar script yang jalan bisa baca sendiri. */
                                          if (!filepath) filepath = argv[i];
                                      } else {
                                          fprintf(stderr, "[LOOP] Peringatan: flag tidak dikenal '%s' - diabaikan\n", argv[i]);
                                      }
                          }

                          if (!filepath) { fprintf(stderr, "Error: tidak ada file .lp\n"); return 1; }

                          char *kode = baca_file(filepath);

                          if (do_debug) {
                              fprintf(stderr, "[LOOP DEBUG] File    : %s\n", filepath);
                              fprintf(stderr, "[LOOP DEBUG] Ukuran  : %d byte\n", (int)strlen(kode));
                              fprintf(stderr, "[LOOP DEBUG] Mode    : %s\n", do_nasm ? "NASM codegen" : "interpreter");
                          }

                          /* Tokenize - Lexer dialokasikan di heap (struct ~257MB, terlalu besar untuk stack) */
                          Lexer *L = calloc(1, sizeof(Lexer));
                          if (!L) { fprintf(stderr, "[LOOP] Fatal: out of memory\n"); free(kode); return 1; }
                          L->src = kode; tokenize(L);

                          if (do_debug) {
                              fprintf(stderr, "[LOOP DEBUG] Token   : %d\n", L->n_tokens);
                          }

                          /* Parse */
                          Node *prog = parse_program(L->tokens, L->n_tokens);
                          free(L);

                          if (do_debug) {
                              fprintf(stderr, "[LOOP DEBUG] Node top-level: %d\n", prog->n_body);
                          }

                          if (do_nasm) {
                              char *nasm = nasm_generate(prog);
                              if (outfile) {
                                  FILE *f = fopen(outfile, "w");
                                  if (!f) { fprintf(stderr, "Tidak bisa buka output: %s\n", outfile); free(kode); return 1; }
                                  fputs(nasm, f); fclose(f);
                                  fprintf(stderr, "[LOOP] NASM → %s\n", outfile);
                                  fprintf(stderr, "       Untuk menjalankan:\n");
                                  fprintf(stderr, "       nasm -f elf64 %s -o out.o && ld out.o -o out && ./out\n", outfile);
                              } else {
                                  puts(nasm);
                              }
                              free(nasm);
                          } else {
                              g_global_env = env_new(NULL);

                              /* Sediakan 'argumen' - argv TANPA nama binary interpreter (argv[0]),
                               * biar konsisten sama argv standar pas program ini udah jadi
                               * binary native berdiri sendiri: argumen[0]=nama skrip,
                               * argumen[1]=argumen pertama, dst. */
                              int aan = argc - 1;
                              Val **av = malloc(sizeof(Val*) * (size_t)(aan > 0 ? aan : 1));
                              for (int i = 1; i < argc; i++) av[i-1] = val_str(argv[i]);
                              Val *argumen_val = val_array(av, aan);
                              env_set(g_global_env, "argumen", argumen_val);
                              val_free(argumen_val);

                              Val *result = eval(prog, g_global_env);
                              if (g_error) {
                                  fprintf(stderr, "[LOOP ERROR] %s\n", g_error_msg);
                                  val_free(result); env_free(g_global_env); free(kode); return 1;
                              }
                              if (do_debug) {
                                  fprintf(stderr, "[LOOP DEBUG] Eksekusi selesai.\n");
                              }
                              val_free(result);
                              env_free(g_global_env);
                          }

                          free(kode);
                          return 0;
                      }

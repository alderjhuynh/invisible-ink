#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>

static const char *g_src_path = "input";
static int g_cur_line = 1;

static void die(int line, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "%s:%d: error: ", g_src_path, line ? line : g_cur_line);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);
    exit(1);
}

static void *xmalloc(size_t n) {
    void *p = malloc(n);
    if (!p) { fprintf(stderr, "scc: out of memory\n"); exit(1); }
    return p;
}
static void *xrealloc(void *p, size_t n) {
    void *q = realloc(p, n);
    if (!q) { fprintf(stderr, "scc: out of memory\n"); exit(1); }
    return q;
}
static char *xstrdup_n(const char *s, size_t l) {
    char *p = xmalloc(l + 1);
    memcpy(p, s, l);
    p[l] = 0;
    return p;
}
static char *xstrdup(const char *s) { return xstrdup_n(s, strlen(s)); }

enum {
    T_EOF = 0, T_IDENT, T_NUMBER, T_CHAR,
    T_LP, T_RP, T_LBRACE, T_RBRACE, T_LBRACK, T_RBRACK,
    T_SEMI, T_COMMA,
    T_ASSIGN, T_PLUS, T_MINUS, T_STAR, T_SLASH, T_PERCENT,
    T_AMPER, T_PIPE, T_CARET, T_NOT,
    T_EQ, T_NE, T_LT, T_GT, T_LE, T_GE,
    T_LSHIFT, T_RSHIFT, T_ANDAND, T_OROR
};

static const char *TOKNAMES[] = {
    "end of file", "identifier", "number", "char literal", "(", ")", "{", "}",
    "[", "]", ";", ",", "=", "+", "-", "*", "/", "%", "&", "|", "^", "!",
    "==", "!=", "<", ">", "<=", ">=", "<<", ">>", "&&", "||"
};

typedef struct { int kind; int64_t num; char *s; int line; } Tok;

typedef struct { Tok *toks; size_t n, cap, pos; } TokStream;
static TokStream T;

static void tpush(int kind, int64_t num, const char *s, int line) {
    if (T.n == T.cap) {
        T.cap = T.cap ? T.cap * 2 : 256;
        T.toks = xrealloc(T.toks, T.cap * sizeof(Tok));
    }
    T.toks[T.n].kind = kind;
    T.toks[T.n].num = num;
    T.toks[T.n].s = s ? xstrdup(s) : NULL;
    T.toks[T.n].line = line;
    T.n++;
}

static void lex(const char *src) {
    size_t i = 0;
    int line = 1;
    memset(&T, 0, sizeof(T));
    while (src[i]) {
        char c = src[i];
        if (c == '\n') { line++; i++; continue; }
        if (c == ' ' || c == '\t' || c == '\r' || c == '\v' || c == '\f') { i++; continue; }
        if (c == '/' && src[i+1] == '/') { while (src[i] && src[i] != '\n') i++; continue; }
        if (c == '/' && src[i+1] == '*') {
            size_t start_line = line;
            i += 2;
            while (src[i] && !(src[i] == '*' && src[i+1] == '/')) {
                if (src[i] == '\n') line++;
                i++;
            }
            if (!src[i]) die(start_line, "unterminated /* comment");
            i += 2;
            continue;
        }
        if (isalpha((unsigned char)c) || c == '_') {
            size_t j = i;
            while (isalnum((unsigned char)src[j]) || src[j] == '_') j++;
            tpush(T_IDENT, 0, NULL, line);
            Tok *t = &T.toks[T.n - 1];
            free(t->s);
            t->s = xstrdup_n(src + i, j - i);
            i = j;
            continue;
        }
        if (isdigit((unsigned char)c)) {
            size_t j = i;
            while (isdigit((unsigned char)src[j])) j++;
            char *s = xstrdup_n(src + i, j - i);
            tpush(T_NUMBER, strtoll(s, NULL, 10), NULL, line);
            free(s);
            i = j;
            continue;
        }
        if (c == '\'') {
            i++;
            int64_t v;
            if (src[i] == '\\') {
                i++;
                char e = src[i];
                switch (e) {
                    case 'n': v = 10; break;
                    case 't': v = 9;  break;
                    case 'r': v = 13; break;
                    case '0': v = 0;  break;
                    case '\\': v = 92; break;
                    case '\'': v = 39; break;
                    case '"':  v = 34; break;
                    default: die(line, "bad escape '\\%c'", e);
                }
                i++;
            } else if ((unsigned char)src[i] < 0x80) {
                v = (int64_t)(unsigned char)src[i];
                i++;
            } else {
                die(line, "non-ASCII char literal (v0 is byte-only)");
                v = 0;
            }
            if (src[i] != '\'') die(line, "unterminated char literal");
            i++;
            tpush(T_CHAR, v, NULL, line);
            continue;
        }
        int k = -1;
        switch (c) {
            case '(': k = T_LP; break; case ')': k = T_RP; break;
            case '{': k = T_LBRACE; break; case '}': k = T_RBRACE; break;
            case '[': k = T_LBRACK; break; case ']': k = T_RBRACK; break;
            case ';': k = T_SEMI; break; case ',': k = T_COMMA; break;
            case '+': k = T_PLUS; break; case '-': k = T_MINUS; break;
            case '*': k = T_STAR; break; case '/': k = T_SLASH; break;
            case '%': k = T_PERCENT; break;
            case '&': k = T_AMPER; break; case '|': k = T_PIPE; break;
            case '^': k = T_CARET; break;
            case '=': k = T_ASSIGN; break;
            case '!': k = T_NOT; break;
            case '<': k = T_LT; break;
            case '>': k = T_GT; break;
            default: die(line, "unexpected character '%c' (v0 has no strings, '~', '++', '--')", c);
        }
        if (c == '<' && src[i+1] == '<') { tpush(T_LSHIFT, 0, NULL, line); i += 2; continue; }
        if (c == '>' && src[i+1] == '>') { tpush(T_RSHIFT, 0, NULL, line); i += 2; continue; }
        if (c == '=' && src[i+1] == '=') { tpush(T_EQ, 0, NULL, line); i += 2; continue; }
        if (c == '!' && src[i+1] == '=') { tpush(T_NE, 0, NULL, line); i += 2; continue; }
        if (c == '<' && src[i+1] == '=') { tpush(T_LE, 0, NULL, line); i += 2; continue; }
        if (c == '>' && src[i+1] == '=') { tpush(T_GE, 0, NULL, line); i += 2; continue; }
        if (c == '&' && src[i+1] == '&') { tpush(T_ANDAND, 0, NULL, line); i += 2; continue; }
        if (c == '|' && src[i+1] == '|') { tpush(T_OROR, 0, NULL, line); i += 2; continue; }
        tpush(k, 0, NULL, line);
        i++;
    }
    tpush(T_EOF, 0, NULL, line);
}

static const char *peek_tok_at(int ahead);

static int peek_kind(int ahead) {
    size_t i = T.pos + (size_t)ahead;
    if (i >= T.n) return T_EOF;
    return T.toks[i].kind;
}
static int cur_line(void) {
    return (T.pos < T.n) ? T.toks[T.pos].line : g_cur_line;
}
static void advance(void) {
    if (T.toks[T.pos].kind != T_EOF) T.pos++;
}
static const char *peek_tok_at(int ahead) {
    size_t i = T.pos + (size_t)ahead;
    if (i >= T.n) return NULL;
    Tok *t = &T.toks[i];
    return t->kind == T_IDENT ? t->s : NULL;
}
static const char *peek_tok(void) { return peek_tok_at(0); }
static bool is_ident_kw(const char *kw, int ahead) {
    return peek_kind(ahead) == T_IDENT && peek_tok_at(ahead) &&
           strcmp(peek_tok_at(ahead), kw) == 0;
}
static void expect(int kind, const char *what) {
    if (peek_kind(0) != kind) die(cur_line(), "expected %s", what);
    advance();
}
static bool eat_kw(const char *kw) {
    if (is_ident_kw(kw, 0)) { advance(); return true; }
    return false;
}

typedef struct { char *data; size_t len, cap; } SB;
static void sb_init(SB *sb) {
    sb->cap = 1024; sb->len = 0; sb->data = xmalloc(sb->cap); sb->data[0] = 0;
}
static void sb_put(SB *sb, const char *s) {
    size_t l = strlen(s);
    if (sb->len + l + 2 > sb->cap) {
        while (sb->len + l + 2 > sb->cap) sb->cap *= 2;
        sb->data = xrealloc(sb->data, sb->cap);
    }
    memcpy(sb->data + sb->len, s, l);
    sb->len += l;
    sb->data[sb->len] = 0;
}
static void sb_linef(SB *sb, const char *fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    sb_put(sb, "    ");
    sb_put(sb, buf);
    sb_put(sb, "\n");
}
static void sb_wordline(SB *sb, const char *name) {
    size_t need = strlen(name) + 10;
    if (sb->len + need + 2 > sb->cap) {
        while (sb->len + need + 2 > sb->cap) sb->cap *= 2;
        sb->data = xrealloc(sb->data, sb->cap);
    }
    int w = snprintf(sb->data + sb->len, sb->cap - sb->len, "word %s\n", name);
    if (w > 0) sb->len += (size_t)w;
}


typedef struct { char *name; int64_t addr; int is_array; int64_t len; int has_init; } Global;

typedef struct {
    char *name;
    int off;
    int is_param;
    int is_array;
    int64_t len;
} Local;

typedef struct Fn Fn;
typedef struct St St;
typedef struct { St **items; size_t count, cap; } StList;
struct Fn {
    char *name;        /* Scribe name */
    char *qn;          /* Quill word name */
    Local *locals;     /* params (off 1..N) then declared locals */
    size_t nlocals, caplocals;
    int next_offset;   /* next free cell offset within the frame */
    int nargs;
    int has_early_return;
    size_t helper_count;
    int64_t frame_size;   /* N + L + 3, finalized at codegen */
    StList body;          /* parsed function body */
    size_t idx;
};

static Global *g_globals = NULL;
static size_t g_nglobals = 0, g_capg = 0;
static Fn *g_fns = NULL;
static size_t g_nfns = 0, g_capfn = 0;

#define SP_SLOT 0
#define FP_SLOT 1
#define FIRST_GLOBAL_ADDR 2
static int64_t g_next_global_addr = FIRST_GLOBAL_ADDR;

static const char *BUILTINS[] = {
    "putc", "putn", "getc", "peek", "poke", "sleep", "rnd", "exit", NULL
};
static const char *KEYWORDS[] = {
    "fn", "let", "if", "else", "while", "for", "return", NULL
};
static const char *RESERVED[] = { "SP", "FP", "_program", NULL };
static bool g_builtin_void[] = {
    true, true, false, false, true, true, false, true,
    /* putc putn getc peek poke sleep rnd exit */
};

static bool is_builtin(const char *s) {
    for (int i = 0; BUILTINS[i]; i++) if (strcmp(s, BUILTINS[i]) == 0) return true;
    return false;
}
static bool is_void_builtin(const char *s) {
    for (int i = 0; BUILTINS[i]; i++)
        if (strcmp(s, BUILTINS[i]) == 0) return g_builtin_void[i];
    return false;
}
static bool is_keyword_name(const char *s) {
    for (int i = 0; KEYWORDS[i]; i++) if (strcmp(s, KEYWORDS[i]) == 0) return true;
    return false;
}
static void check_user_name(const char *s, int line) {
    if (is_builtin(s)) die(line, "'%s' is a builtin name", s);
    if (is_keyword_name(s)) die(line, "'%s' is a keyword", s);
    for (int i = 0; RESERVED[i]; i++)
        if (strcmp(s, RESERVED[i]) == 0) die(line, "'%s' is reserved by scc", s);
}
static char *mangle(const char *name) {
    char *s = xmalloc(strlen(name) + 4);
    sprintf(s, "fn_%s", name);
    return s;
}

static Fn *find_fn(const char *name) {
    for (size_t i = 0; i < g_nfns; i++)
        if (strcmp(g_fns[i].name, name) == 0) return &g_fns[i];
    return NULL;
}
static Global *find_global(const char *name) {
    for (size_t i = 0; i < g_nglobals; i++)
        if (strcmp(g_globals[i].name, name) == 0) return &g_globals[i];
    return NULL;
}
static Local *find_local(Fn *f, const char *name) {
    for (size_t i = 0; i < f->nlocals; i++)
        if (strcmp(f->locals[i].name, name) == 0) return &f->locals[i];
    return NULL;
}

enum { N_INT, N_VAR, N_ARR, N_CALL, N_BUILTIN, N_BIN, N_UNARY };
enum {
    B_ADD, B_SUB, B_MUL, B_DIV, B_MOD,
    B_AND, B_ORR, B_XOR, B_SHL, B_SHR,
    B_EQ, B_NE, B_LT, B_LE, B_GT, B_GE,
    B_LAND, B_LOR
};
enum { U_NEG, U_NOT };

typedef struct Node Node;

typedef struct { Node **items; size_t count, cap; } NodeList;
static void nl_push(NodeList *l, Node *n) {
    if (l->count == l->cap) {
        l->cap = l->cap ? l->cap * 2 : 4;
        l->items = xrealloc(l->items, l->cap * sizeof(Node *));
    }
    l->items[l->count++] = n;
}

struct Node {
    int kind, line;
    int64_t num;          /* N_INT */
    char *s;              /* N_VAR / N_ARR / N_CALL / N_BUILTIN name */
    Node *a, *b;          /* N_BIN: operands. N_UNARY: a. N_ARR: b=index */
    int op;               /* B_* / U_* */
    NodeList args;
};

enum { S_LET, S_ASSIGN, S_IF, S_WHILE, S_FOR, S_RETURN, S_EXPR };

static void sl_push(StList *l, St *s) {
    if (l->count == l->cap) {
        l->cap = l->cap ? l->cap * 2 : 4;
        l->items = xrealloc(l->items, l->cap * sizeof(St *));
    }
    l->items[l->count++] = s;
}

struct St {
    int kind, line;
    Node *expr;           /* return value / conditions / let-init */
    Node *lval;           /* S_ASSIGN target */
    Node *call;           /* S_EXPR expression */
    StList then_b, else_b;
    StList body, tail;    /* S_WHILE body; S_FOR body + tail(step) */
    St *init;             /* S_FOR init */
};

static Node *node_new(int kind, int line) {
    Node *n = xmalloc(sizeof(Node));
    memset(n, 0, sizeof(Node));
    n->kind = kind;
    n->line = line;
    return n;
}

typedef struct { char *name; int line; size_t argc; } DCall;
static DCall *g_dcalls = NULL;
static size_t g_ndcalls = 0, g_capdc = 0;
static void record_call(const char *name, int line, size_t argc) {
    if (g_ndcalls == g_capdc) {
        g_capdc = g_capdc ? g_capdc * 2 : 32;
        g_dcalls = xrealloc(g_dcalls, g_capdc * sizeof(DCall));
    }
    g_dcalls[g_ndcalls].name = xstrdup(name);
    g_dcalls[g_ndcalls].line = line;
    g_dcalls[g_ndcalls].argc = argc;
    g_ndcalls++;
}

static Node *parse_expr(void);

static Node *parse_primary(void) {
    int line = cur_line();
    int k = peek_kind(0);
    if (k == T_NUMBER || k == T_CHAR) {
        Node *n = node_new(N_INT, line);
        n->num = T.toks[T.pos].num;
        advance();
        return n;
    }
    if (k == T_LP) {
        advance();
        Node *e = parse_expr();
        expect(T_RP, "')'");
        return e;
    }
    if (k == T_IDENT) {
        const char *name = peek_tok();
        if (is_keyword_name(name)) die(line, "keyword '%s' used as a value", name);
        if (is_builtin(name)) {
            advance();
            expect(T_LP, "'(' after builtin name");
            Node *n = node_new(N_BUILTIN, line);
            n->s = xstrdup(name);
            if (peek_kind(0) != T_RP) {
                for (;;) {
                    nl_push(&n->args, parse_expr());
                    if (peek_kind(0) == T_COMMA) { advance(); continue; }
                    break;
                }
            }
            expect(T_RP, "')' to close the call");
            static const int ARITY[] = { 1, 1, 0, 1, 2, 1, 1, 0 };
            for (int i = 0; BUILTINS[i]; i++) {
                if (strcmp(name, BUILTINS[i]) == 0) {
                    if ((int)n->args.count != ARITY[i])
                        die(line, "builtin %s() takes %d argument(s), got %zu",
                                name, ARITY[i], n->args.count);
                    break;
                }
            }
            return n;
        }
        advance();
        if (peek_kind(0) == T_LP) {
            advance();
            Node *n = node_new(N_CALL, line);
            n->s = xstrdup(name);
            if (peek_kind(0) != T_RP) {
                for (;;) {
                    nl_push(&n->args, parse_expr());
                    if (peek_kind(0) == T_COMMA) { advance(); continue; }
                    break;
                }
            }
            expect(T_RP, "')' to close the call");
            record_call(name, line, n->args.count);
            return n;
        }
        if (peek_kind(0) == T_LBRACK) {
            advance();
            Node *idx = parse_expr();
            expect(T_RBRACK, "']' after index");
            Node *n = node_new(N_ARR, line);
            n->s = xstrdup(name);
            n->b = idx;
            return n;
        }
        Node *n = node_new(N_VAR, line);
        n->s = xstrdup(name);
        return n;
    }
    die(line, "expected an expression, got '%s'",
            TOKNAMES[k >= 0 && k < (int)(sizeof(TOKNAMES)/sizeof(TOKNAMES[0])) ? k : 0]);
    return NULL;
}

static Node *parse_unary(void) {
    int line = cur_line();
    if (peek_kind(0) == T_MINUS) {
        if (peek_kind(1) == T_NUMBER || peek_kind(1) == T_CHAR) {
            advance();
            Node *n = node_new(N_INT, line);
            n->num = -T.toks[T.pos].num;
            advance();
            return n;
        }
        advance();
        Node *n = node_new(N_UNARY, line);
        n->op = U_NEG;
        n->a = parse_unary();
        return n;
    }
    if (peek_kind(0) == T_NOT) {
        advance();
        Node *n = node_new(N_UNARY, line);
        n->op = U_NOT;
        n->a = parse_unary();
        return n;
    }
    return parse_primary();
}

static Node *bin(Node *a, Node *b, int op, int line) {
    Node *n = node_new(N_BIN, line);
    n->op = op;
    n->a = a;
    n->b = b;
    return n;
}

static Node *parse_factor(void) {
    Node *a = parse_unary();
    for (;;) {
        int k = peek_kind(0);
        if (k == T_STAR || k == T_SLASH || k == T_PERCENT) {
            int ln = cur_line(); advance();
            a = bin(a, parse_unary(), (k == T_STAR) ? B_MUL : (k == T_SLASH) ? B_DIV : B_MOD, ln);
        } else return a;
    }
}
static Node *parse_term(void) {
    Node *a = parse_factor();
    for (;;) {
        int k = peek_kind(0);
        if (k == T_PLUS || k == T_MINUS) {
            int ln = cur_line(); advance();
            a = bin(a, parse_factor(), (k == T_PLUS) ? B_ADD : B_SUB, ln);
        } else return a;
    }
}
static Node *parse_shift(void) {
    Node *a = parse_term();
    for (;;) {
        int k = peek_kind(0);
        if (k == T_LSHIFT || k == T_RSHIFT) {
            int ln = cur_line(); advance();
            a = bin(a, parse_term(), (k == T_LSHIFT) ? B_SHL : B_SHR, ln);
        } else return a;
    }
}
static Node *parse_relational(void) {
    Node *a = parse_shift();
    for (;;) {
        int k = peek_kind(0); int op = -1;
        if (k == T_LT) op = B_LT;
        else if (k == T_GT) op = B_GT;
        else if (k == T_LE) op = B_LE;
        else if (k == T_GE) op = B_GE;
        if (op >= 0) { int ln = cur_line(); advance(); a = bin(a, parse_shift(), op, ln); }
        else return a;
    }
}
static Node *parse_equality(void) {
    Node *a = parse_relational();
    for (;;) {
        int k = peek_kind(0); int op = -1;
        if (k == T_EQ) op = B_EQ;
        else if (k == T_NE) op = B_NE;
        if (op >= 0) { int ln = cur_line(); advance(); a = bin(a, parse_relational(), op, ln); }
        else return a;
    }
}
static Node *parse_band(void) {
    Node *a = parse_equality();
    while (peek_kind(0) == T_AMPER) { int ln = cur_line(); advance(); a = bin(a, parse_equality(), B_AND, ln); }
    return a;
}
static Node *parse_bxor(void) {
    Node *a = parse_band();
    while (peek_kind(0) == T_CARET) { int ln = cur_line(); advance(); a = bin(a, parse_band(), B_XOR, ln); }
    return a;
}
static Node *parse_bor(void) {
    Node *a = parse_bxor();
    while (peek_kind(0) == T_PIPE) { int ln = cur_line(); advance(); a = bin(a, parse_bxor(), B_ORR, ln); }
    return a;
}
static Node *parse_land(void) {
    Node *a = parse_bor();
    while (peek_kind(0) == T_ANDAND) { int ln = cur_line(); advance(); a = bin(a, parse_bor(), B_LAND, ln); }
    return a;
}
static Node *parse_expr(void) {
    Node *a = parse_land();
    while (peek_kind(0) == T_OROR) { int ln = cur_line(); advance(); a = bin(a, parse_land(), B_LOR, ln); }
    return a;
}

static St *parse_stmt(Fn *fn);
static void parse_block(Fn *fn, StList *out);

static int64_t g_program_sp_init = FIRST_GLOBAL_ADDR;

static void parse_global_decl(void) {
    int line = cur_line();
    advance(); /* let */
    if (peek_kind(0) != T_IDENT) die(line, "expected a name after 'let'");
    const char *nm = peek_tok();
    if (find_global(nm)) die(line, "global '%s' already declared", nm);
    if (is_builtin(nm) || is_keyword_name(nm)) die(line, "'%s' may not be a global name", nm);
    for (int i = 0; RESERVED[i]; i++)
        if (strcmp(nm, RESERVED[i]) == 0) die(line, "'%s' is reserved by scc", nm);
    advance();

    if (g_nglobals == g_capg) {
        g_capg = g_capg ? g_capg * 2 : 8;
        g_globals = xrealloc(g_globals, g_capg * sizeof(Global));
    }
    Global *g = &g_globals[g_nglobals++];
    memset(g, 0, sizeof(Global));
    g->name = xstrdup(nm);
    g->addr = g_next_global_addr;
    g->len = 1;

    if (peek_kind(0) == T_LBRACK) {
        advance();
        if (peek_kind(0) != T_NUMBER) die(cur_line(), "array length must be a literal number");
        int64_t len = T.toks[T.pos].num;
        advance();
        expect(T_RBRACK, "']' after array length");
        if (len <= 0) die(line, "array '%s' must have a positive length", nm);
        g->is_array = true;
        g->len = len;
        g_next_global_addr += len;
    } else {
        g_next_global_addr += 1;
    }
    if (g->is_array && peek_kind(0) == T_ASSIGN)
        die(line, "array '%s' can't have an initializer (v0)", nm);
    if (!g->is_array && peek_kind(0) == T_ASSIGN) {
        advance();
        int64_t v = 0;
        if (peek_kind(0) == T_MINUS) {
            advance();
            if (peek_kind(0) != T_NUMBER && peek_kind(0) != T_CHAR)
                die(line, "global initializers must be literal constants");
            v = -T.toks[T.pos].num;
            advance();
        } else if (peek_kind(0) == T_NUMBER || peek_kind(0) == T_CHAR) {
            v = T.toks[T.pos].num;
            advance();
        } else {
            die(line, "global initializers must be literal constants");
        }
        g->len = v;
        g->has_init = 1;
    }
    expect(T_SEMI, "';' after global declaration");
}

static Fn *parse_fn_decl(void) {
    int line = cur_line();
    advance(); /* fn */
    if (peek_kind(0) != T_IDENT) die(line, "expected a fn name");
    const char *nm = peek_tok();
    check_user_name(nm, line);
    if (find_fn(nm)) die(line, "function '%s' already declared", nm);
    if (find_global(nm)) die(line, "'%s' is both a global and a function", nm);
    advance();

    if (g_nfns == g_capfn) {
        g_capfn = g_capfn ? g_capfn * 2 : 8;
        g_fns = xrealloc(g_fns, g_capfn * sizeof(Fn));
    }
    Fn *f = &g_fns[g_nfns++];
    memset(f, 0, sizeof(Fn));
    f->name = xstrdup(nm);
    f->qn = mangle(nm);
    f->idx = g_nfns - 1;
    f->caplocals = 8;
    f->locals = xmalloc(f->caplocals * sizeof(Local));

    expect(T_LP, "'(' after fn name");
    while (peek_kind(0) != T_RP) {
        if (peek_kind(0) != T_IDENT) die(cur_line(), "expected a parameter name");
        const char *pn = peek_tok();
        if (find_local(f, pn)) die(cur_line(), "parameter '%s' named twice", pn);
        if (is_builtin(pn) || is_keyword_name(pn)) die(cur_line(), "'%s' may not be a parameter name", pn);
        if (f->nlocals == f->caplocals) {
            f->caplocals *= 2;
            f->locals = xrealloc(f->locals, f->caplocals * sizeof(Local));
        }
        Local *l = &f->locals[f->nlocals++];
        memset(l, 0, sizeof(Local));
        l->name = xstrdup(pn);
        l->is_param = true;
        l->off = ++f->nargs;
        advance();
        if (peek_kind(0) == T_COMMA) { advance(); continue; }
        break;
    }
    expect(T_RP, "')' after parameters");
    f->next_offset = f->nargs + 1;
    parse_block(f, &f->body);
    return f;
}

static void parse_program(void) {
    while (peek_kind(0) != T_EOF) {
        if (peek_kind(0) != T_IDENT)
            die(cur_line(), "top level expects 'fn' or 'let', got '%s'",
                    TOKNAMES[peek_kind(0)]);
        const char *nm = peek_tok();
        if (strcmp(nm, "fn") == 0) parse_fn_decl();
        else if (strcmp(nm, "let") == 0) parse_global_decl();
        else die(cur_line(), "top level expects 'fn' or 'let', got '%s'", nm);
    }
    for (size_t i = 0; i < g_ndcalls; i++) {
        Fn *f = find_fn(g_dcalls[i].name);
        if (!f) die(g_dcalls[i].line, "call to undefined function '%s'", g_dcalls[i].name);
        if ((int)g_dcalls[i].argc != f->nargs)
            die(g_dcalls[i].line, "function '%s' takes %d argument(s), got %zu",
                    f->name, f->nargs, g_dcalls[i].argc);
    }
    Fn *main_fn = find_fn("main");
    if (!main_fn) die(0, "no 'fn main' defined");
    if (main_fn->nargs != 0) die(0, "main must take zero arguments");
    g_program_sp_init = g_next_global_addr;
}

static Local *register_local(Fn *f, const char *nm, int is_array, int64_t len, int line) {
    if (find_local(f, nm)) die(line, "local '%s' already declared", nm);
    if (find_global(nm)) die(line, "local '%s' shadows global (no shadowing in v0)", nm);
    if (is_builtin(nm) || is_keyword_name(nm)) die(line, "'%s' may not be a variable name", nm);
    if (f->nlocals == f->caplocals) {
        f->caplocals = f->caplocals ? f->caplocals * 2 : 8;
        f->locals = xrealloc(f->locals, f->caplocals * sizeof(Local));
    }
    Local *l = &f->locals[f->nlocals++];
    memset(l, 0, sizeof(Local));
    l->name = xstrdup(nm);
    l->is_param = false;
    l->off = f->next_offset;
    l->is_array = is_array;
    l->len = len;
    f->next_offset += (int)len;
    return l;
}

static St *new_st(int kind, int line) {
    St *s = xmalloc(sizeof(St));
    memset(s, 0, sizeof(St));
    s->kind = kind;
    s->line = line;
    return s;
}

static St *parse_assign_core(Fn *fn) {
    int line = cur_line();
    const char *nm = peek_tok();
    St *s = new_st(S_ASSIGN, line);
    (void)fn;
    advance(); /* ident */
    if (peek_kind(0) == T_LBRACK) {
        advance();
        Node *lv = node_new(N_ARR, line);
        lv->s = xstrdup(nm);
        lv->b = parse_expr();
        expect(T_RBRACK, "']' after index");
        s->lval = lv;
    } else {
        Node *lv = node_new(N_VAR, line);
        lv->s = xstrdup(nm);
        s->lval = lv;
    }
    expect(T_ASSIGN, "'=' in assignment");
    s->expr = parse_expr();
    return s;
}

static St *parse_local_decl(Fn *fn, bool need_semi) {
    int line = cur_line();
    advance(); /* 'let' */
    if (peek_kind(0) != T_IDENT) die(line, "expected a name after 'let'");
    const char *nm = peek_tok();
    advance();
    int is_array = 0; int64_t alen = 1;
    if (peek_kind(0) == T_LBRACK) {
        advance();
        if (peek_kind(0) != T_NUMBER) die(cur_line(), "array length must be a literal number");
        alen = T.toks[T.pos].num;
        advance();
        expect(T_RBRACK, "']' after array length");
        if (alen <= 0) die(line, "array length must be positive");
        is_array = 1;
    }
    register_local(fn, nm, is_array, alen, line);
    St *s = new_st(S_LET, line);
    Node *lv = node_new(N_VAR, line);
    lv->s = xstrdup(nm);
    s->lval = lv;
    if (peek_kind(0) == T_ASSIGN) {
        if (is_array) die(line, "array '%s' can't have an initializer (v0)", nm);
        advance();
        s->expr = parse_expr();
    }
    if (need_semi) expect(T_SEMI, "';' after declaration");
    return s;
}

static void parse_block(Fn *fn, StList *out) {
    expect(T_LBRACE, "'{'");
    while (peek_kind(0) != T_RBRACE) {
        if (peek_kind(0) == T_EOF) die(cur_line(), "unexpected end of file inside a block");
        sl_push(out, parse_stmt(fn));
    }
    expect(T_RBRACE, "'}'");
}

static St *parse_stmt(Fn *fn) {
    int line = cur_line();
    int k = peek_kind(0);

    if (k == T_IDENT) {
        const char *nm = peek_tok();
        if (strcmp(nm, "let") == 0) return parse_local_decl(fn, true);
        if (strcmp(nm, "fn") == 0) die(line, "functions must be declared at top level");
        if (strcmp(nm, "if") == 0) {
            advance();
            expect(T_LP, "'(' after 'if'");
            Node *cond = parse_expr();
            expect(T_RP, "')' after if condition");
            St *s = new_st(S_IF, line);
            s->expr = cond;
            parse_block(fn, &s->then_b);
            if (eat_kw("else")) parse_block(fn, &s->else_b);
            return s;
        }
        if (strcmp(nm, "while") == 0) {
            advance();
            expect(T_LP, "'(' after 'while'");
            Node *cond = parse_expr();
            expect(T_RP, "')' after while condition");
            St *s = new_st(S_WHILE, line);
            s->expr = cond;
            parse_block(fn, &s->body);
            return s;
        }
        if (strcmp(nm, "for") == 0) {
            advance();
            expect(T_LP, "'(' after 'for'");
            St *s = new_st(S_FOR, line);
            if (peek_kind(0) != T_SEMI) {
                if (strcmp(peek_tok(), "let") == 0) s->init = parse_local_decl(fn, false);
                else s->init = parse_assign_core(fn);
            }
            expect(T_SEMI, "';' after for-init");
            if (peek_kind(0) != T_SEMI) s->expr = parse_expr();
            expect(T_SEMI, "';' after for-condition");
            if (peek_kind(0) == T_IDENT && !is_keyword_name(peek_tok()) && !is_builtin(peek_tok())) {
                sl_push(&s->tail, parse_assign_core(fn));
            }
            expect(T_RP, "')' after for-step");
            parse_block(fn, &s->body);
            return s;
        }
        if (strcmp(nm, "return") == 0) {
            advance();
            St *s = new_st(S_RETURN, line);
            fn->has_early_return = 1;
            if (peek_kind(0) != T_SEMI) s->expr = parse_expr();
            expect(T_SEMI, "';' after return");
            return s;
        }
        if (peek_kind(1) == T_ASSIGN || peek_kind(1) == T_LBRACK) {
            St *s = parse_assign_core(fn);
            expect(T_SEMI, "';' after assignment");
            return s;
        }
        {
            Node *e = parse_expr();
            expect(T_SEMI, "';'");
            St *s = new_st(S_EXPR, line);
            s->call = e;
            return s;
        }
    }
    die(line, "expected a statement (let/if/while/for/return/assignment/expression)");
    return NULL;
}

typedef struct { Fn *fn; SB *out; } Gen;

static void em(Gen *g, const char *fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    sb_linef(g->out, "%s", buf);
}
static void em_int(Gen *g, int64_t v) { sb_linef(g->out, "push %lld", (long long)v); }

/* forward decls */
static void gen_expr(Gen *g, Node *n);
static void emit_range(Gen *g, St **items, size_t start, size_t end);
static void emit_stmt(Gen *g, St *s);
static void emit_done_guard(Gen *g, St **items, size_t start, size_t end);
static bool stmt_may_return(St *s);
static bool list_may_return(StList *l);

/* addr of name */
static void emit_name_addr(Gen *g, const char *name, int line) {
    Local *l = find_local(g->fn, name);
    if (l) {
        em(g, "load FP");
        em_int(g, l->off);
        em(g, "add");
        return;
    }
    Global *gl = find_global(name);
    if (!gl) die(line, "undefined variable '%s'", name);
    em_int(g, gl->addr);
}

static void emit_lval_addr(Gen *g, Node *lv) {
    emit_name_addr(g, lv->s, lv->line);
    if (lv->kind == N_ARR) {
        gen_expr(g, lv->b);   /* index */
        em(g, "add");         /* base + i, one cell per element */
    }
}

static int fn_roff(Fn *f) { return f->nargs + ((int)f->nlocals - f->nargs) + 1; }
static int fn_droff(Fn *f) { return f->nargs + ((int)f->nlocals - f->nargs) + 2; }

static void gen_binary(Gen *g, Node *n) {
    gen_expr(g, n->a);
    gen_expr(g, n->b);
    switch (n->op) {
        case B_ADD: em(g, "add"); break;
        case B_SUB: em(g, "sub"); break;
        case B_MUL: em(g, "mul"); break;
        case B_DIV: em(g, "div"); break;
        case B_MOD: em(g, "mod"); break;
        case B_AND: em(g, "and"); break;
        case B_ORR: em(g, "or"); break;
        case B_XOR: em(g, "xor"); break;
        case B_SHL: em(g, "shl"); break;
        case B_SHR: em(g, "shr"); break;
        case B_EQ:
            em(g, "sub"); em(g, "sign"); em(g, "dup"); em(g, "mul");
            em_int(g, 1); em(g, "swap"); em(g, "sub");
            break;
        case B_NE:
            em(g, "sub"); em(g, "sign"); em(g, "dup"); em(g, "mul");
            break;
        case B_LT:
            em(g, "sub"); em(g, "sign");
            em_int(g, 1); em(g, "swap"); em(g, "sub");
            em_int(g, 1); em(g, "shr");
            break;
        case B_GT:
            em(g, "sub"); em(g, "sign");
            em_int(g, 1); em(g, "add");
            em_int(g, 1); em(g, "shr");
            break;
        case B_LE:
            em(g, "sub"); em(g, "sign");
            em_int(g, 1); em(g, "add");
            em_int(g, 1); em(g, "shr");
            em_int(g, 1); em(g, "xor");
            break;
        case B_GE:
            em(g, "sub"); em(g, "sign");
            em_int(g, 1); em(g, "swap"); em(g, "sub");
            em_int(g, 1); em(g, "shr");
            em_int(g, 1); em(g, "xor");
            break;
        case B_LAND:
            em(g, "swap"); em(g, "sign"); em(g, "dup"); em(g, "mul");
            em(g, "swap"); em(g, "sign"); em(g, "dup"); em(g, "mul");
            em(g, "and");
            break;
        case B_LOR:
            em(g, "swap"); em(g, "sign"); em(g, "dup"); em(g, "mul");
            em(g, "swap"); em(g, "sign"); em(g, "dup"); em(g, "mul");
            em(g, "or");
            break;
    }
}

static void gen_expr(Gen *g, Node *n) {
    switch (n->kind) {
        case N_INT: em_int(g, n->num); break;
        case N_VAR:
            emit_name_addr(g, n->s, n->line);
            em(g, "load");
            break;
        case N_ARR:
            emit_name_addr(g, n->s, n->line);
            gen_expr(g, n->b);
            em(g, "add");
            em(g, "load");
            break;
        case N_CALL: {
            for (size_t i = 0; i < n->args.count; i++) gen_expr(g, n->args.items[i]);
            Fn *f = find_fn(n->s);
            em(g, "call %s", f->qn);
            break;
        }
        case N_BUILTIN: {
            if (is_void_builtin(n->s))
                die(n->line, "builtin %s() produces no value; use it as a statement", n->s);
            if (strcmp(n->s, "getc") == 0) {
                em(g, "char-in");
            } else if (strcmp(n->s, "peek") == 0) {
                gen_expr(g, n->args.items[0]);
                em(g, "load");
            } else if (strcmp(n->s, "rnd") == 0) {
                gen_expr(g, n->args.items[0]);
                em(g, "rand");
            } else {
                die(n->line, "unknown builtin in value position");
            }
            break;
        }
        case N_UNARY:
            gen_expr(g, n->a);
            if (n->op == U_NEG) {
                em(g, "neg");
            } else {
                em(g, "sign"); em(g, "dup"); em(g, "mul");
                em_int(g, 1); em(g, "swap"); em(g, "sub");
            }
            break;
        case N_BIN:
            gen_binary(g, n);
            break;
    }
}

static void gen_void_builtin(Gen *g, Node *n) {
    if (strcmp(n->s, "putc") == 0) {
        gen_expr(g, n->args.items[0]);
        em(g, "char-out");
    } else if (strcmp(n->s, "putn") == 0) {
        gen_expr(g, n->args.items[0]);
        em(g, "num-out");
    } else if (strcmp(n->s, "poke") == 0) {
        gen_expr(g, n->args.items[0]);   /* addr first */
        gen_expr(g, n->args.items[1]);   /* value second */
        em(g, "store");
    } else if (strcmp(n->s, "sleep") == 0) {
        gen_expr(g, n->args.items[0]);
        em(g, "sleep");
    } else if (strcmp(n->s, "exit") == 0) {
        em(g, "halt");
    } else {
        die(n->line, "builtin %s() produces a value; it belongs in an expression", n->s);
    }
}

static void emit_return(Gen *g, St *s) {
    if (s->expr) {
        em(g, "load FP");        /* addr = B + ROFF */
        em_int(g, fn_roff(g->fn));
        em(g, "add");
        gen_expr(g, s->expr);
        em(g, "store");
    }
    em(g, "load FP");
    em_int(g, fn_droff(g->fn));
    em(g, "add");
    em_int(g, 1);
    em(g, "store");
}

static void gen_while(Gen *g, Node *cond, StList *body, StList *tail) {
    em(g, "while {");
    if (g->fn->has_early_return) {
        em(g, "load FP");
        em_int(g, fn_droff(g->fn));
        em(g, "add");
        em(g, "load");
        em(g, "if {");
        em_int(g, 0);                  /* done -> falsy cond -> exit */
        em(g, "} else {");
        if (cond) gen_expr(g, cond);
        else em_int(g, 1);
        em(g, "}");
    } else {
        if (cond) gen_expr(g, cond);
        else em_int(g, 1);
    }
    em(g, "} {");
    emit_range(g, body->items, 0, body->count);
    if (tail) emit_range(g, tail->items, 0, tail->count);
    em(g, "}");
}

static void emit_stmt(Gen *g, St *s) {
    switch (s->kind) {
        case S_LET:
            if (s->expr) {
                emit_lval_addr(g, s->lval);
                gen_expr(g, s->expr);
                em(g, "store");
            }
            break;
        case S_ASSIGN:
            emit_lval_addr(g, s->lval);
            gen_expr(g, s->expr);
            em(g, "store");
            break;
        case S_EXPR:
            if (s->call->kind == N_BUILTIN && is_void_builtin(s->call->s)) {
                gen_void_builtin(g, s->call);
            } else {
                gen_expr(g, s->call);
                em(g, "drop");
            }
            break;
        case S_IF:
            gen_expr(g, s->expr);
            em(g, "if {");
            emit_range(g, s->then_b.items, 0, s->then_b.count);
            em(g, "}");
            if (s->else_b.count) {
                em(g, "else {");
                emit_range(g, s->else_b.items, 0, s->else_b.count);
                em(g, "}");
            }
            break;
        case S_WHILE:
            gen_while(g, s->expr, &s->body, NULL);
            break;
        case S_FOR:
            if (s->init) emit_stmt(g, s->init);
            gen_while(g, s->expr, &s->body, &s->tail);
            break;
        case S_RETURN:
        default:
            break;
    }
}

static bool stmt_may_return(St *s) {
    if (!s) return false;
    if (s->kind == S_RETURN) return true;
    if (s->kind == S_IF)
        return list_may_return(&s->then_b) || list_may_return(&s->else_b);
    if (s->kind == S_WHILE || s->kind == S_FOR)
        return list_may_return(&s->body) || list_may_return(&s->tail);
    return false;
}
static bool list_may_return(StList *l) {
    for (size_t i = 0; i < l->count; i++)
        if (stmt_may_return(l->items[i])) return true;
    return false;
}

static void emit_range(Gen *g, St **items, size_t start, size_t end) {
    for (size_t i = start; i < end; i++) {
        St *s = items[i];
        if (s->kind == S_RETURN) {
            emit_return(g, s);
            if (i + 1 < end) emit_done_guard(g, items, i + 1, end);
            return;
        }
        emit_stmt(g, s);
        if (stmt_may_return(s)) {
            if (i + 1 < end) emit_done_guard(g, items, i + 1, end);
            return;
        }
    }
}

static void emit_done_guard(Gen *g, St **items, size_t start, size_t end) {
    em(g, "load FP");
    em_int(g, fn_droff(g->fn));
    em(g, "add");
    em(g, "load");
    em_int(g, 1);
    em(g, "sub");
    em(g, "if {");
    emit_range(g, items, start, end);
    em(g, "}");
}

static SB g_fnchunks[256];
static size_t g_nfnchunks = 0;

static void gen_function(Fn *f) {
    SB *chunk = &g_fnchunks[g_nfnchunks++];
    sb_init(chunk);
    Gen g; g.fn = f; g.out = chunk;

    sb_wordline(chunk, f->qn);
    sb_linef(chunk, "; fn %s (%d args)", f->name, f->nargs);

    int nargs = f->nargs;
    int nlocals = (int)f->nlocals - nargs;
    int size = nargs + nlocals + 3;
    int roff = nargs + nlocals + 1;
    int droff = nargs + nlocals + 2;
    f->frame_size = size;

    em(&g, "load SP");
    em_int(&g, size);
    em(&g, "add");
    em(&g, "store SP");
    em(&g, "load FP");           /* value: caller's FP */
    em(&g, "load SP");           /* addr:  B */
    em_int(&g, size);
    em(&g, "sub");
    em(&g, "swap");
    em(&g, "store");

    em(&g, "load SP");
    em_int(&g, size);
    em(&g, "sub");
    em(&g, "store FP");

    for (size_t i = (size_t)nargs; i < f->nlocals; i++) {
        Local *l = &f->locals[i];
        for (int64_t c = 0; c < l->len; c++) {
            em(&g, "load SP");
            em_int(&g, size - l->off - (int)c);
            em(&g, "sub");
            em_int(&g, 0);
            em(&g, "store");
        }
    }
    em(&g, "load SP");
    em_int(&g, size - roff);
    em(&g, "sub");
    em_int(&g, 0);
    em(&g, "store");
    em(&g, "load SP");
    em_int(&g, size - droff);
    em(&g, "sub");
    em_int(&g, 0);
    em(&g, "store");

    for (int i = nargs; i >= 1; i--) {
        em(&g, "load SP");
        em_int(&g, size - i);
        em(&g, "sub");          /* addr = B + i */
        em(&g, "swap");         /* [a_i, addr] -> [addr, a_i] */
        em(&g, "store");
    }

    emit_range(&g, f->body.items, 0, f->body.count);

    em(&g, "load SP");
    em_int(&g, size);
    em(&g, "sub");
    em(&g, "store SP");
    em(&g, "load SP");     /* B */
    em(&g, "load");        /* mem[B] = caller FP */
    em(&g, "store FP");
    em(&g, "load SP");
    em_int(&g, roff);
    em(&g, "add");
    em(&g, "load");
}

static const char *g_program_chunk = NULL;

static void gen_program(void) {
    SB prog;
    sb_init(&prog);
    sb_wordline(&prog, "_program");
    sb_linef(&prog, "; scc program entry");

    sb_linef(&prog, "let SP = %lld;", (long long)g_program_sp_init);
    sb_linef(&prog, "let FP = 0;");

    for (size_t i = 0; i < g_nglobals; i++) {
        Global *gl = &g_globals[i];
        if (gl->has_init) {
            sb_linef(&prog, "push %lld", (long long)gl->addr);   /* addr first */
            sb_linef(&prog, "push %lld", (long long)gl->len);   /* value */
            sb_linef(&prog, "store");
        }
    }

    sb_linef(&prog, "call fn_main");
    sb_linef(&prog, "drop");
    sb_linef(&prog, "halt");

    while (prog.len > 0 && prog.data[prog.len - 1] == '\n') prog.len--;
    prog.data[prog.len] = 0;
    g_program_chunk = prog.data;
}

static void write_output(FILE *f) {
    fputs(g_program_chunk, f);
    fputc('\n', f);
    for (size_t i = 0; i < g_nfnchunks; i++) {
        fputc('\n', f);
        fputs(g_fnchunks[i].data, f);
        fputc('\n', f);
    }
}

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "scc: cannot open '%s'\n", path); exit(1); }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n < 0) { fprintf(stderr, "scc: read error on '%s'\n", path); exit(1); }
    char *buf = xmalloc((size_t)n + 1);
    size_t got = fread(buf, 1, (size_t)n, f);
    buf[got] = 0;
    fclose(f);
    return buf;
}

static void usage(void) {
    fprintf(stderr,
        "Usage: ink scc [-o out.q] file.s\n"
        "  Scribe -> Quill compiler for Invisible Ink.\n"
        "  Then: ink asm out.q -o out.txt && ink run out.txt\n");
    exit(1);
}

int scc_main(int argc, char **argv) {
    const char *out_path = NULL;
    const char *in_path = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 >= argc) usage();
            out_path = argv[++i];
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage();
        } else if (!in_path) {
            in_path = argv[i];
        } else {
            usage();
        }
    }
    if (!in_path) usage();

    char *src = read_file(in_path);
    g_src_path = in_path;
    lex(src);
    parse_program();

    for (size_t i = 0; i < g_nfns; i++) gen_function(&g_fns[i]);
    gen_program();

    FILE *out = stdout;
    if (out_path) {
        out = fopen(out_path, "wb");
        if (!out) { fprintf(stderr, "scc: cannot write '%s'\n", out_path); exit(1); }
    }
    write_output(out);
    if (out_path) fclose(out);
    return 0;
}
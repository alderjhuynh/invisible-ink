/* sdec.c - Quill -> Scribe decompiler ("unscribe").
 *
 * Goal: turn Quill produced by `ink scc` back into readable Scribe.
 * Works by structural pattern matching as planned:
 *   1. parse Quill text into per-word op trees (if/while structure)
 *   2. strip the scc frame prologue/epilogue (prologue yields nargs, frame size,
 *      roff/droff, and all local-slot zero stores)
 *   3. strip return/done-guard scaffolding (scc's emit_done_guard + while-cond
 *      done checks); emits real `return expr;` instead
 *   4. peephole-merge the comparison expansions scc emits (==, !=, <, >,
 *      <=, >=, &&, ||, !) back into single compound ops
 *   5. simulate the remaining ops with a stack of expression trees; stores to
 *      frame slots become assignments/returns, everything else becomes
 *      putc/putn/poke/sleep/peek/rnd/getc
 *   6. reconstruct global + local declarations so that recompiling the output
 *      with `ink scc` reproduces the exact same Quill -> Invisible Ink bytes.
 *
 * Layout invariant used throughout: instead of guessing array lengths, every
 * frame/global cell is declared as a scalar (or pad array for unused gaps);
 * indexed access is rendered as name[idx], which scc compiles to the identical
 * `push addr / push idx / add / load` sequence.栈 behavior is therefore
 * provably byte-identical on recompilation.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>

/* error reporting through the shared quill machinery */
extern char q_errmsg[1024];
static void sdec_err(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    vsnprintf(q_errmsg, sizeof(q_errmsg), fmt, ap);
    va_end(ap);
}

static void *sxmalloc(size_t n) {
    void *p = malloc(n);
    if (!p) { fprintf(stderr, "sdec: out of memory\n"); exit(1); }
    return p;
}
static void *sxrealloc(void *p, size_t n) {
    void *q = realloc(p, n);
    if (!q) { fprintf(stderr, "sdec: out of memory\n"); exit(1); }
    return q;
}
static char *sxstrdup(const char *s) {
    size_t l = strlen(s) + 1;
    char *p = sxmalloc(l);
    memcpy(p, s, l);
    return p;
}

/* ========== tokens ========== */
enum { QT_IDENT, QT_NUM, QT_LBRACE, QT_RBRACE, QT_EQ };

typedef struct { int kind; int64_t num; char *s; } TQ;
typedef struct { TQ *t; size_t n; } Toks;

static void toks_free(Toks *tk) {
    for (size_t i = 0; i < tk->n; i++) free(tk->t[i].s);
    free(tk->t);
    tk->t = NULL; tk->n = 0;
}

static bool tq_is_ident(const char *s) {
    if (!s || !*s) return false;
    if (!(isalpha((unsigned char)s[0]) || s[0] == '_')) return false;
    for (size_t i = 1; s[i]; i++) {
        char c = s[i];
        if (!(isalnum((unsigned char)c) || c == '_' || c == '-')) return false;
    }
    return true;
}

static const char *Q_KEYWORDS[] = {
    "word","if","else","while","let","call","halt","push","dup","swap","drop",
    "rot","neg","sub","mod","id","add","div","sign","mul","pow","and","or",
    "xor","shl","shr","num-out","char-out","char-in","rand","sleep","store",
    "load","sock-create","sock-bind","sock-listen","sock-accept","sock-recv",
    "sock-send","sock-close","close","file-open","file-read","file-stat",
    "file-close","dir-list","sys", NULL
};
static bool tq_is_keyword(const char *s) {
    for (int i = 0; Q_KEYWORDS[i]; i++) if (strcmp(s, Q_KEYWORDS[i]) == 0) return true;
    return false;
}

static bool sdec_tokenize(const char *src, size_t len, Toks *tk) {
    size_t cap = 64;
    tk->t = sxmalloc(cap * sizeof(TQ));
    tk->n = 0;
    size_t i = 0;
    while (i < len) {
        char c = src[i];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') { i++; continue; }
        if (c == ';') { while (i < len && src[i] != '\n') i++; continue; }
        if (tk->n + 1 >= cap) { cap *= 2; tk->t = sxrealloc(tk->t, cap * sizeof(TQ)); }
        TQ *out = &tk->t[tk->n];
        memset(out, 0, sizeof(TQ));
        if (c == '{') { out->kind = QT_LBRACE; out->s = NULL; i++; tk->n++; continue; }
        if (c == '}') { out->kind = QT_RBRACE; out->s = NULL; i++; tk->n++; continue; }
        if (c == '=') { out->kind = QT_EQ; out->s = NULL; i++; tk->n++; continue; }
        if (isdigit((unsigned char)c) ||
            (c == '-' && i + 1 < len && isdigit((unsigned char)src[i+1]))) {
            size_t j = i + 1;
            while (j < len && isdigit((unsigned char)src[j])) j++;
            size_t l = j - i;
            char *s = sxmalloc(l + 1);
            memcpy(s, src + i, l); s[l] = 0;
            out->kind = QT_NUM;
            out->num = strtoll(s, NULL, 10);
            free(s);
            i = j; tk->n++;
            continue;
        }
        if (isalpha((unsigned char)c) || c == '_') {
            size_t j = i;
            while (j < len && (isalnum((unsigned char)src[j]) || src[j] == '_' || src[j] == '-')) j++;
            size_t l = j - i;
            char *s = sxmalloc(l + 1);
            memcpy(s, src + i, l); s[l] = 0;
            out->kind = QT_IDENT;
            out->s = s;
            i = j; tk->n++;
            continue;
        }
        sdec_err("sdec: unexpected character '%c' in Quill input", c);
        toks_free(tk);
        return false;
    }
    return true;
}

/* ========== op trees ========== */
enum AK {
    APUSH, ADUP, ASWAP, AROT, ADROP, ANEG,
    AADD, ASUB, AMUL, ADIV, AMOD,
    AAND, AOR, AXOR, ASHL, ASHR,
    ASIGN, APOW,
    ANUMOUT, ACHAROUT, ACHARIN, ARAND, ASLEEP, ASTORE, ALOAD,
    ACALL, AHALT,
    /* merged comparison/boolean compounds */
    AEQ, ANE, ALT, AGT, ALE, AGE, ALAND, ALOR, ANOT, ANZ,
    /* statement scaffolding */
    ADONESET,
    /* structure */
    AIF, AWILE,
    /* unsupported runs (sockets/files etc.) keep their kind name */
    ASOCK_CREATE, ASOCK_BIND, ASOCK_LISTEN, ASOCK_ACCEPT, ASOCK_RECV,
    ASOCK_SEND, ASOCK_CLOSE, AFILE_OPEN, AFILE_READ, AFILE_STAT, AFILE_CLOSE,
    ADIR_LIST, AID,
};

typedef struct AOp AOp;
typedef struct { AOp **it; size_t n, cap; } Ops;

struct AOp {
    int k;
    int64_t v;      /* APUSH value / ACALL word index */
    char *name;     /* extra context (e.g. op name for errors) */
    Ops *a;         /* AIF: then; AW HILE: cond */
    Ops *b;         /* AIF: else (may be NULL); AW HILE: body */
};

static Ops *ops_new(void) { Ops *o = sxmalloc(sizeof(Ops)); o->it = NULL; o->n = o->cap = 0; return o; }
static void ops_push(Ops *o, AOp *op) {
    if (o->n == o->cap) {
        o->cap = o->cap ? o->cap * 2 : 8;
        o->it = sxrealloc(o->it, o->cap * sizeof(AOp *));
    }
    o->it[o->n++] = op;
}
static AOp *op_new(int k) {
    AOp *o = sxmalloc(sizeof(AOp));
    memset(o, 0, sizeof(AOp));
    o->k = k;
    return o;
}

/* ========== word/slot maps ========== */
typedef struct { char *name; int64_t addr; } MapE;
typedef struct { MapE *e; size_t n, cap; } SMap;

static void smap_push(SMap *m, const char *name, int64_t addr) {
    if (m->n == m->cap) {
        m->cap = m->cap ? m->cap * 2 : 16;
        m->e = sxrealloc(m->e, m->cap * sizeof(MapE));
    }
    m->e[m->n].name = sxstrdup(name);
    m->e[m->n].addr = addr;
    m->n++;
}
static bool smap_find(SMap *m, const char *name, int64_t *out) {
    for (size_t i = 0; i < m->n; i++)
        if (strcmp(m->e[i].name, name) == 0) { *out = m->e[i].addr; return true; }
    return false;
}
static int smap_count(SMap *m) { return (int)m->n; }

/* ========== Parser: tokens -> op trees ========== */
typedef struct {
    Toks *tk;
    size_t pos, end;
    SMap *slots;
    SMap *words;
    const char *curword;
} Parser;

typedef enum { PR_OK, PR_ERROR, PR_ENDWORD, PR_KEEP } PStat;

static PStat parse_ops(Parser *p, Ops *out, int depth);

static AOp *simple_op(Parser *p, const char *tok) {
    (void)p;
    /* linear mnemonics */
    static const struct { const char *nm; int k; } TAB[] = {
        {"dup", ADUP}, {"swap", ASWAP}, {"drop", ADROP}, {"rot", AROT},
        {"neg", ANEG}, {"sub", ASUB}, {"mod", AMOD}, {"id", AID},
        {"add", AADD}, {"div", ADIV}, {"sign", ASIGN}, {"mul", AMUL},
        {"pow", APOW}, {"and", AAND}, {"or", AOR}, {"xor", AXOR},
        {"shl", ASHL}, {"shr", ASHR},
        {"num-out", ANUMOUT}, {"char-out", ACHAROUT}, {"char-in", ACHARIN},
        {"rand", ARAND}, {"sleep", ASLEEP},
        {"store", ASTORE}, {"load", ALOAD}, {"halt", AHALT},
        {"sock-create", ASOCK_CREATE}, {"sock-bind", ASOCK_BIND},
        {"sock-listen", ASOCK_LISTEN}, {"sock-accept", ASOCK_ACCEPT},
        {"sock-recv", ASOCK_RECV}, {"sock-send", ASOCK_SEND},
        {"sock-close", ASOCK_CLOSE}, {"close", ASOCK_CLOSE},
        {"file-close", ASOCK_CLOSE}, {"file-open", AFILE_OPEN},
        {"file-read", AFILE_READ}, {"file-stat", AFILE_STAT},
        {"dir-list", ADIR_LIST},
        {NULL, 0}
    };
    for (int i = 0; TAB[i].nm; i++)
        if (strcmp(tok, TAB[i].nm) == 0) {
            AOp *o = op_new(TAB[i].k);
            o->name = sxstrdup(tok);
            return o;
        }
    return NULL;
}

static TQ *pk(Parser *p) { return p->pos < p->end ? &p->tk->t[p->pos] : NULL; }
static bool pk_is(Parser *p, const char *s) {
    TQ *t = pk(p);
    if (!t) return false;
    if (strcmp(s, "{") == 0) return t->kind == QT_LBRACE;
    if (strcmp(s, "}") == 0) return t->kind == QT_RBRACE;
    if (strcmp(s, "=") == 0) return t->kind == QT_EQ;
    return t->kind == QT_IDENT && strcmp(t->s, s) == 0;
}

/* number in slot sugar: `let N = V` where addr assigned in scan order */
static PStat parse_let(Parser *p, Ops *out) {
    p->pos++; /* 'let' */
    TQ *t = pk(p);
    if (!t || t->kind != QT_IDENT) {
        sdec_err("sdec: word '%s': 'let' without name", p->curword);
        return PR_ERROR;
    }
    char *nm = sxstrdup(t->s);
    p->pos++;
    if (!pk(p) || pk(p)->kind != QT_EQ) {
        sdec_err("sdec: word '%s': 'let %s' missing '='", p->curword, nm);
        free(nm);
        return PR_ERROR;
    }
    p->pos++;
    t = pk(p);
    if (!t || t->kind != QT_NUM) {
        sdec_err("sdec: word '%s': 'let %s' missing integer value", p->curword, nm);
        free(nm);
        return PR_ERROR;
    }
    int64_t val = t->num;
    p->pos++;
    int64_t addr;
    if (!smap_find(p->slots, nm, &addr)) {
        sdec_err("sdec: undefined slot '%s' (internal)", nm);
        free(nm);
        return PR_ERROR;
    }
    free(nm);
    AOp *a = op_new(APUSH); a->v = addr; ops_push(out, a);
    a = op_new(APUSH); a->v = val;        ops_push(out, a);
    ops_push(out, op_new(ASTORE));
    return PR_OK;
}

static PStat parse_iload_istore(Parser *p, Ops *out, const char *tok) {
    /* `load X` / `store X` with slot or bare address */
    p->pos++; /* keyword */
    TQ *t = pk(p);
    int64_t addr = -1;
    char *nm = NULL;
    bool have = false;
    if (t && t->kind == QT_NUM) { addr = t->num; p->pos++; have = true; }
    else if (t && t->kind == QT_IDENT && !tq_is_keyword(t->s) &&
             strcmp(t->s, "{") != 0 && strcmp(t->s, "}") != 0) {
        nm = t->s; p->pos++; have = true;
    }
    if (!have) {
        /* bare load/store */
        ops_push(out, op_new(strcmp(tok, "load") == 0 ? ALOAD : ASTORE));
        return PR_OK;
    }
    if (!nm) {
        if (addr < 0) {
            sdec_err("sdec: word '%s': negative %s address", p->curword, tok);
            return PR_ERROR;
        }
        AOp *a = op_new(APUSH); a->v = addr; ops_push(out, a);
        ops_push(out, op_new(strcmp(tok, "load") == 0 ? ALOAD : ASTORE));
        return PR_OK;
    }
    if (!smap_find(p->slots, nm, &addr)) {
        sdec_err("sdec: word '%s': undefined slot '%s'", p->curword, nm);
        return PR_ERROR;
    }
    AOp *a = op_new(APUSH); a->v = addr; ops_push(out, a);
    if (strcmp(tok, "store") == 0) ops_push(out, op_new(ASWAP));
    ops_push(out, op_new(strcmp(tok, "load") == 0 ? ALOAD : ASTORE));
    return PR_OK;
}

static PStat parse_call(Parser *p, Ops *out) {
    p->pos++; /* 'call' */
    TQ *t = pk(p);
    if (!t) {
        sdec_err("sdec: word '%s': 'call' without target", p->curword);
        return PR_ERROR;
    }
    AOp *o = op_new(ACALL);
    if (t->kind == QT_NUM) {
        o->v = t->num;
        if (o->v < 0) {
            sdec_err("sdec: word '%s': negative call index", p->curword);
            return PR_ERROR;
        }
    } else if (t->kind == QT_IDENT) {
        int64_t idx;
        if (!smap_find(p->words, t->s, &idx)) {
            sdec_err("sdec: word '%s': call to undefined word '%s'", p->curword, t->s);
            return PR_ERROR;
        }
        o->v = idx;
        o->name = NULL;
    } else {
        sdec_err("sdec: word '%s': bad call target", p->curword);
        return PR_ERROR;
    }
    p->pos++;
    ops_push(out, o);
    return PR_OK;
}

static PStat parse_block(Parser *p, Ops **out, int depth) {
    /* parse `{ ... }` */
    if (!pk_is(p, "{")) {
        TQ *t = pk(p);
        sdec_err("sdec: word '%s': expected '{' at pos %zu (got %s)",
                 p->curword, p->pos,
                 t ? (t->kind == QT_LBRACE ? "{" : t->kind == QT_RBRACE ? "}" :
                      t->kind == QT_NUM ? "<num>" :
                      t->s ? t->s : "?") : "<end>");
        return PR_ERROR;
    }
    p->pos++;
    Ops *blk = ops_new();
    PStat s = parse_ops(p, blk, depth + 1);
    if (s != PR_OK) { return s; }
    *out = blk;
    return PR_OK;
}

static PStat parse_ops(Parser *p, Ops *out, int depth) {
    if (depth > 200) {
        sdec_err("sdec: blocks nested too deep");
        return PR_ERROR;
    }
    for (;;) {
        if (p->pos >= p->end) return PR_ENDWORD;
        TQ *t = pk(p);
        if (t->kind == QT_RBRACE) {
            p->pos++;
            return PR_OK;
        }
        if (t->kind == QT_LBRACE || t->kind == QT_EQ) {
            sdec_err("sdec: word '%s': stray '%s' at token %zu", p->curword,
                     t->kind == QT_EQ ? "=" : "{", p->pos);
            return PR_ERROR;
        }
        if (t->kind == QT_NUM) {
            sdec_err("sdec: word '%s': stray integer '%lld' at token %zu",
                     p->curword, (long long)t->num, p->pos);
            return PR_ERROR;
        }
        const char *tok = t->s;
        if (strcmp(tok, "if") == 0) {
            p->pos++;
            AOp *o = op_new(AIF);
            PStat s2 = parse_block(p, &o->a, depth);
            if (s2 != PR_OK) return s2;
            if (pk_is(p, "else")) {
                p->pos++;
                PStat s3 = parse_block(p, &o->b, depth);
                if (s3 != PR_OK) return s3;
            }
            ops_push(out, o);
            continue;
        }
        if (strcmp(tok, "while") == 0) {
            p->pos++;
            AOp *o = op_new(AWILE);
            PStat s2 = parse_block(p, &o->a, depth);
            if (s2 != PR_OK) return s2;
            PStat s3 = parse_block(p, &o->b, depth);
            if (s3 != PR_OK) return s3;
            ops_push(out, o);
            continue;
        }
        if (strcmp(tok, "word") == 0) {
            sdec_err("sdec: unexpected 'word' inside word body");
            return PR_ERROR;
        }
        if (strcmp(tok, "else") == 0) {
            sdec_err("sdec: else without matching if");
            return PR_ERROR;
        }
        if (strcmp(tok, "let") == 0) {
            PStat s = parse_let(p, out);
            if (s != PR_OK) return s;
            continue;
        }
        if (strcmp(tok, "push") == 0) {
            p->pos++;
            TQ *t2 = pk(p);
            if (!t2 || t2->kind != QT_NUM) {
                sdec_err("sdec: word '%s': push requires integer literal", p->curword);
                return PR_ERROR;
            }
            int64_t val = t2->num;
            p->pos++;
            if (val < 0) {
                AOp *a = op_new(APUSH); a->v = -val; ops_push(out, a);
                ops_push(out, op_new(ANEG));
            } else {
                AOp *a = op_new(APUSH); a->v = val; ops_push(out, a);
            }
            continue;
        }
        if (strcmp(tok, "call") == 0) {
            PStat s = parse_call(p, out);
            if (s != PR_OK) return s;
            continue;
        }
        if (strcmp(tok, "sys") == 0) {
            /* The generic syscall op reconstructs fine as Quill, but there is
               no Scribe statement shape for arbitrary numbers of popped args,
               so unscribe refuses it cleanly rather than emitting lies. */
            sdec_err("sdec: word '%s': sys calls are not supported by unscribe "
                     "(use 'ink disassemble' for the Quill form)", p->curword);
            return PR_ERROR;
        }
        if (strcmp(tok, "load") == 0 || strcmp(tok, "store") == 0) {
            PStat s = parse_iload_istore(p, out, tok);
            if (s != PR_OK) return s;
            continue;
        }
        AOp *o = simple_op(p, tok);
        if (!o) {
            sdec_err("sdec: word '%s': unknown identifier '%s'", p->curword, tok);
            return PR_ERROR;
        }
        p->pos++;
        o->name = sxstrdup(tok);
        ops_push(out, o);
        continue;
    }
}

/* ========== frame shape analysis (scc prologue / epilogue) ========== */
typedef struct {
    bool ok;          /* scc-shaped word */
    int64_t size;     /* frame size S */
    int nargs;
    int64_t roff;     /* result slot offset (S-2); droff = S-1 */
} Frame;

static bool opk(Ops *o, size_t i, int k) { return i < o->n && o->it[i]->k == k; }
static bool opk_v(Ops *o, size_t i, int k, int64_t v) {
    return i < o->n && o->it[i]->k == k && o->it[i]->v == v;
}
static bool opk_any(Ops *o, size_t i, int k, int64_t *out_v) {
    if (i < o->n && o->it[i]->k == k) { *out_v = o->it[i]->v; return true; }
    return false;
}
static void ops_del_prefix(Ops *o, size_t count) {
    size_t newn = o->n - count;
    for (size_t j = 0; j < newn; j++) o->it[j] = o->it[count + j];
    o->n = newn;
}

/* Peel the scc frame prologue. On success fills fr and edits `body` in place
 * to contain just the function logic (epilogue peeled later). */
static PStat peel_prologue(Parser *p, Ops *body, Frame *fr) {
    size_t q = 0;
    int64_t S = 0, K = 0;
    /* load SP; push S; add; store SP; load FP; load SP; push S; sub; swap;
     * store; load SP; push S; sub; store FP                       (22 ops) */
    if (!opk_v(body, 0, APUSH, 0) || !opk(body, 1, ALOAD)) return PR_KEEP;
    if (!opk_any(body, 2, APUSH, &S) || !opk(body, 3, AADD)) return PR_KEEP;
    if (!opk_v(body, 4, APUSH, 0) || !opk(body, 5, ASWAP) || !opk(body, 6, ASTORE)) return PR_KEEP;
    if (!opk_v(body, 7, APUSH, 1) || !opk(body, 8, ALOAD)) return PR_KEEP;
    if (!opk_v(body, 9, APUSH, 0) || !opk(body, 10, ALOAD)) return PR_KEEP;
    if (!opk_any(body, 11, APUSH, &K) || K != S || !opk(body, 12, ASUB)) return PR_KEEP;
    if (!opk(body, 13, ASWAP) || !opk(body, 14, ASTORE)) return PR_KEEP;
    if (!opk_v(body, 15, APUSH, 0) || !opk(body, 16, ALOAD)) return PR_KEEP;
    if (!opk_any(body, 17, APUSH, &K) || K != S || !opk(body, 18, ASUB)) return PR_KEEP;
    if (!opk_v(body, 19, APUSH, 1) || !opk(body, 20, ASWAP) || !opk(body, 21, ASTORE)) return PR_KEEP;
    if (S <= 3) {
        sdec_err("sdec: word '%s': implausible frame size %lld", p->curword, (long long)S);
        return PR_ERROR;
    }
    q = 22;

    /* zero units:  PUSH(0) LOAD PUSH(K) SUB PUSH(0) STORE            (6 ops) */
    int64_t prevK = -1;
    for (;;) {
        if (!(opk_v(body, q, APUSH, 0) && opk(body, q + 1, ALOAD) &&
              opk(body, q + 2, APUSH) && opk(body, q + 3, ASUB) &&
              opk_v(body, q + 4, APUSH, 0) && opk(body, q + 5, ASTORE)))
            break;
        opk_any(body, q + 2, APUSH, &K);
        if (K <= 0 || K >= S) {
            sdec_err("sdec: word '%s': zero store outside frame (K=%lld)",
                     p->curword, (long long)K);
            return PR_ERROR;
        }
        if (prevK >= 0 && K >= prevK) {
            sdec_err("sdec: word '%s': non-descending zero stores (K=%lld)",
                     p->curword, (long long)K);
            return PR_ERROR;
        }
        prevK = K;
        q += 6;
    }

    /* arg units:   PUSH(0) LOAD PUSH(K) SUB SWAP STORE               (6 ops) */
    int nargs = 0;
    int64_t argPrevK = -1;
    for (;;) {
        if (!(opk_v(body, q, APUSH, 0) && opk(body, q + 1, ALOAD) &&
              opk(body, q + 2, APUSH) && opk(body, q + 3, ASUB) &&
              opk(body, q + 4, ASWAP) && opk(body, q + 5, ASTORE)))
            break;
        opk_any(body, q + 2, APUSH, &K);
        if (K <= 1 || K >= S || (argPrevK >= 0 && K != argPrevK + 1)) {
            sdec_err("sdec: word '%s': bad arg copy (K=%lld, prev=%lld)",
                     p->curword, (long long)K, (long long)argPrevK);
            return PR_ERROR;
        }
        argPrevK = K;
        nargs++;
        q += 6;
    }
    if (nargs > 0 && argPrevK != S - 1) {
        sdec_err("sdec: word '%s': arg copies end at K=%lld not S-1=%lld",
                 p->curword, (long long)argPrevK, (long long)(S - 1));
        return PR_ERROR;
    }

    /* epilogue:
     * load SP; push S; sub; store SP; load SP; load; store FP;
     * load SP; push roff; add; load                                 (18 ops) */
    size_t n = body->n;
    if (n > q + 18 &&
        opk_v(body, n - 18, APUSH, 0) && opk(body, n - 17, ALOAD) &&
        opk_v(body, n - 16, APUSH, S) && opk(body, n - 15, ASUB) &&
        opk_v(body, n - 14, APUSH, 0) && opk(body, n - 13, ASWAP) && opk(body, n - 12, ASTORE) &&
        opk_v(body, n - 11, APUSH, 0) && opk(body, n - 10, ALOAD) && opk(body, n - 9, ALOAD) &&
        opk_v(body, n - 8, APUSH, 1) && opk(body, n - 7, ASWAP) && opk(body, n - 6, ASTORE) &&
        opk_v(body, n - 5, APUSH, 0) && opk(body, n - 4, ALOAD) &&
        opk_v(body, n - 3, APUSH, S - 2) && opk(body, n - 2, AADD) &&
        opk(body, n - 1, ALOAD)) {
        body->n = n - 18;
    } else {
        sdec_err("sdec: word '%s': missing scc frame epilogue", p->curword);
        return PR_ERROR;
    }

    ops_del_prefix(body, q);
    fr->ok = true;
    fr->size = S;
    fr->nargs = nargs;
    fr->roff = S - 2;
    return PR_OK;
}

/* ========== scaffolding strips (needs fr: droff = S-1, roff = S-2) ========== */

static bool strip_recursive(Ops *o, Frame *fr);

/* unwrap `if { then.. }` guarded by load done; push1 sub  (emit_done_guard) */
static bool strip_guard(Ops *o, size_t i, Frame *fr) {
    int64_t k;
    if (!opk_any(o, i + 2, APUSH, &k) || k != fr->size - 1) return false;
    AOp *iff = o->it[i + 8];
    if (iff->b) return false;               /* guards have no else */
    if (i + 9 != o->n) {
        /* scc never emits code after a done guard that fired; anything
         * following would be its own guard. require end-of-list */
        return false;
    }
    if (strip_recursive(iff->a, fr)) {}
    /* remove the 8 helper ops, splice then-ops in place at i */
    size_t ins = iff->a->n;
    AOp **tmp = sxmalloc((ins ? ins : 1) * sizeof(AOp *));
    for (size_t j = 0; j < ins; j++) tmp[j] = iff->a->it[j];
    free(iff->a);
    free(iff);
    memmove(o->it + i, o->it + i + 8, (o->n - i - 9) * sizeof(AOp *)); /* (always none) */
    if (o->cap < ins) {
        o->cap = ins * 2;
        o->it = sxrealloc(o->it, o->cap * sizeof(AOp *));
    }
    for (size_t j = 0; j < ins; j++) o->it[i + j] = tmp[j];
    o->n = i + ins;
    free(tmp);
    return true;
}

/* unwrap while-cond done check:
 *   load FP; push droff; add; load; if { push 0 } else { cond } */
static bool strip_while_cond(AOp *w, Frame *fr) {
    Ops *c = w->a;
    if (c->n < 6) return false;
    if (!opk_v(c, 0, APUSH, 1) || !opk(c, 1, ALOAD)) return false;
    int64_t k;
    if (!opk_any(c, 2, APUSH, &k) || k != fr->size - 1) return false;
    if (!opk(c, 3, AADD) || !opk(c, 4, ALOAD)) return false;
    if (!opk(c, 5, AIF)) return false;
    AOp *iff = c->it[5];
    if (!iff->b) return false;
    if (!opk_v(iff->a, 0, APUSH, 0) || iff->a->n != 1) return false;
    /* else-block may itself be statements-building-cond; keep as whole */
    if (strip_recursive(iff->b, fr)) {}
    /* splice else-ops into cond */
    size_t ins = iff->b->n;
    Ops *old_cond = iff->b;
    Ops *newc = ops_new();
    for (size_t j = 0; j < ins; j++) ops_push(newc, old_cond->it[j]);
    free(old_cond);
    free(iff->a);
    free(iff);
    for (size_t j = 6; j < c->n; j++) ops_push(newc, c->it[j]);
    w->a = newc;
    return true;
}

static bool strip_recursive(Ops *o, Frame *fr) {
    bool changed_any = false;
    for (size_t i = 0; i < o->n; ) {
        AOp *op = o->it[i];
        if (op->k == AIF) {
            if (strip_recursive(op->a, fr)) {}
            if (op->b && strip_recursive(op->b, fr)) {}
            i++;
            continue;
        }
        if (op->k == AWILE) {
            if (strip_while_cond(op, fr)) changed_any = true;
            if (strip_recursive(op->b, fr)) {}
            i++;
            continue;
        }
        /* guard prefix: load FP; push droff; add; load; push 1; sub; if {...} */
        if (i + 8 < o->n &&
            opk_v(o, i, APUSH, 1) && opk(o, i + 1, ALOAD) &&
            opk(o, i + 2, APUSH) && opk(o, i + 3, AADD) && opk(o, i + 4, ALOAD) &&
            opk_v(o, i + 5, APUSH, 1) && opk(o, i + 6, ASUB) && opk(o, i + 7, AIF)) {
            size_t save = o->n;
            if (strip_guard(o, i, fr)) { changed_any = true; i = i; continue; }
            (void)save;
        }
        i++;
    }
    return changed_any;
}

/* ========== peephole: merge scc comparison expansions ========== */

static bool is(Ops *o, size_t i, int k) { return i < o->n && o->it[i]->k == k; }
static bool isv(Ops *o, size_t i, int k, int64_t v) {
    return is(o, i, k) && o->it[i]->v == v;
}

/* tries: return merged kind of run at i; scc gen_binary expansions verbatim:
 *   GE: sub sign 1 swap sub 1 shr 1 xor (9)
 *   LE: sub sign 1 add 1 shr 1 xor      (8)
 *   LT: sub sign 1 swap sub 1 shr       (7)
 *   GT: sub sign 1 add 1 shr            (7)
 *   EQ: sub sign dup mul 1 swap sub     (7)
 *   NE: sub sign dup mul                (4)
 *   NOT: sign dup mul 1 swap sub        (6)
 *   LAND/LOR: swap sign dup mul {swap sign dup mul} and|or (9)
 *   NZ: sign dup mul                    (3)
 */
static int pat_match(Ops *o, size_t i, Frame *fr) {
    int64_t k = fr->size - 1;                       /* droff */
    if (is(o, i, ASUB) && is(o, i + 1, ASIGN)) {
        if (isv(o, i + 2, APUSH, 1)) {
            if (is(o, i + 3, ASWAP) && is(o, i + 4, ASUB) &&
                isv(o, i + 5, APUSH, 1) && is(o, i + 6, ASHR) &&
                isv(o, i + 7, APUSH, 1) && is(o, i + 8, AXOR)) return AGE;
            if (is(o, i + 3, AADD) && isv(o, i + 4, APUSH, 1) && is(o, i + 5, ASHR) &&
                isv(o, i + 6, APUSH, 1) && is(o, i + 7, AXOR)) return ALE;
            if (is(o, i + 3, ASWAP) && is(o, i + 4, ASUB) &&
                isv(o, i + 5, APUSH, 1) && is(o, i + 6, ASHR)) return ALT;
            if (is(o, i + 3, AADD) && isv(o, i + 4, APUSH, 1) && is(o, i + 5, ASHR)) return AGT;
        }
        if (is(o, i + 2, ADUP) && is(o, i + 3, AMUL)) {
            if (isv(o, i + 4, APUSH, 1) && is(o, i + 5, ASWAP) && is(o, i + 6, ASUB))
                return AEQ;
            return ANE;
        }
    }
    if (is(o, i, ASWAP) && is(o, i + 1, ASIGN) && is(o, i + 2, ADUP) &&
        is(o, i + 3, AMUL) && is(o, i + 4, ASWAP) && is(o, i + 5, ASIGN) &&
        is(o, i + 6, ADUP) && is(o, i + 7, AMUL)) {
        if (is(o, i + 8, AAND)) return ALAND;
        if (is(o, i + 8, AOR)) return ALOR;
    }
    if (is(o, i, ASIGN) && is(o, i + 1, ADUP) && is(o, i + 2, AMUL)) {
        if (isv(o, i + 3, APUSH, 1) && is(o, i + 4, ASWAP) && is(o, i + 5, ASUB))
            return ANOT;
        return ANZ;
    }
    if (isv(o, i, APUSH, 1) && is(o, i + 1, ALOAD) && isv(o, i + 2, APUSH, k) &&
        is(o, i + 3, AADD) && isv(o, i + 4, APUSH, 1) && is(o, i + 5, ASTORE))
        return ADONESET;
    return 0;
}

static const char *compound_opstr(int k) {
    switch (k) {
        case AEQ: return "=="; case ANE: return "!="; case ALT: return "<";
        case AGT: return ">"; case ALE: return "<="; case AGE: return ">=";
        case ALAND: return "&&"; case ALOR: return "||"; case ANOT: return "!";
        case ANZ: return "!=";
    }
    return "?";
}

static int compound_len(int k) {
    switch (k) {
        case ANZ: return 3;
        case ANE: return 4;
        case ANOT: case ADONESET: return 6;
        case AEQ: case ALT: case AGT: return 7;
        case ALE: return 8;
        case AGE: case ALAND: case ALOR: return 9;
    }
    return 1;
}

static void merge_peephole_in(Ops *o, Frame *fr) {
    for (size_t i = 0; i < o->n; ) {
        AOp *op = o->it[i];
        if (op->k == AIF) {
            merge_peephole_in(op->a, fr);
            if (op->b) merge_peephole_in(op->b, fr);
            i++;
            continue;
        }
        if (op->k == AWILE) {
            merge_peephole_in(op->a, fr);
            merge_peephole_in(op->b, fr);
            i++;
            continue;
        }
        int m = pat_match(o, i, fr);
        if (m) {
            int len = compound_len(m);
            AOp *merged = op_new(m);
            merged->name = sxstrdup(compound_opstr(m));
            for (size_t j = 0; j < (size_t)len; j++) free(o->it[i + j]);
            if (getenv("SDEC_TRACE"))
                fprintf(stderr, "merge@%zu kind=%d len=%d\n", i, m, len);
            o->it[i] = merged;
            memmove(o->it + i + 1, o->it + i + len,
                    (o->n - i - len) * sizeof(AOp *));
            o->n -= (size_t)(len - 1);
            i++;
            continue;
        }
        i++;
    }
}

/* ========== simple string builder for Scribe output ========== */
typedef struct { char *d; size_t n, cap; int ind; } SBOut;
static SBOut *sbo_new(void) {
    SBOut *w = sxmalloc(sizeof(SBOut));
    w->cap = 4096; w->n = 0; w->ind = 0;
    w->d = sxmalloc(w->cap);
    w->d[0] = 0;
    return w;
}
static void sbo_put(SBOut *w, const char *s) {
    size_t l = strlen(s);
    while (w->n + l + 1 > w->cap) { w->cap *= 2; w->d = sxrealloc(w->d, w->cap); }
    memcpy(w->d + w->n, s, l);
    w->n += l;
    w->d[w->n] = 0;
}

/* ========== expression nodes + Scribe rendering ========== */
enum { K_NUM, K_NAME, K_BIN, K_UN, K_CALL, K_BUILTIN, K_IDX };

typedef struct EN EN;
struct EN {
    int k;
    int64_t num;
    char *name;             /* K_NAME / K_CALL / K_BUILTIN / K_IDX base */
    char *op;               /* K_BIN / K_UN */
    EN *a, *b;              /* K_BIN: a, b. K_UN: a. K_IDX: b = index */
    EN **args; size_t narg; /* K_CALL / K_BUILTIN */
    int prec;
};

static EN *en_new(int k, int prec) {
    EN *e = sxmalloc(sizeof(EN));
    memset(e, 0, sizeof(EN));
    e->k = k;
    e->prec = prec;
    return e;
}

static void en_argpush(EN *e, EN *arg) {
    e->args = sxrealloc(e->args, (e->narg + 1) * sizeof(EN *));
    e->args[e->narg++] = arg;
}

static void gpname(int64_t addr, char *buf, size_t cap) {
    if (addr == 0) snprintf(buf, cap, "SP");
    else if (addr == 1) snprintf(buf, cap, "FP");
    else snprintf(buf, cap, "g%lld", (long long)addr);
}

static void en_render(SBOut *w, EN *e) {
    char buf[64];
    if (!e) { sbo_put(w, "<err>"); return; }
    switch (e->k) {
        case K_NUM:
            snprintf(buf, sizeof(buf), "%lld", (long long)e->num);
            sbo_put(w, buf);
            return;
        case K_NAME:
            sbo_put(w, e->name);
            return;
        case K_IDX:
            sbo_put(w, e->name);
            sbo_put(w, "[");
            en_render(w, e->b);
            sbo_put(w, "]");
            return;
        case K_CALL:
        case K_BUILTIN:
            sbo_put(w, e->name);
            sbo_put(w, "(");
            for (size_t i = 0; i < e->narg; i++) {
                en_render(w, e->args[i]);
                if (i + 1 < e->narg) sbo_put(w, ", ");
            }
            sbo_put(w, ")");
            return;
        case K_UN:
            sbo_put(w, e->op);
            if (e->a->prec < e->prec) { sbo_put(w, "("); en_render(w, e->a); sbo_put(w, ")"); }
            else en_render(w, e->a);
            return;
        default:
            break;
    }
    /* binary */
    if (e->a->prec < e->prec) { sbo_put(w, "("); en_render(w, e->a); sbo_put(w, ")"); }
    else en_render(w, e->a);
    sbo_put(w, " ");
    sbo_put(w, e->op);
    sbo_put(w, " ");
    int paren = (e->b->prec < e->prec) || (e->b->prec == e->prec);
    if (paren) { sbo_put(w, "("); en_render(w, e->b); sbo_put(w, ")"); }
    else en_render(w, e->b);
}

/* ========== symbolic stack simulation ========== */
enum { LvNone = 0, LvGlob, LvFp };
typedef struct { EN *e; int lv; int64_t off; EN *idx; } Sv;

typedef struct { Sv *it; size_t n, cap; } Stack;

/* per-program state */
#include <setjmp.h>
static jmp_buf g_sdec_jb;

typedef struct SimCtx SimCtx;
struct SimCtx {
    const char *wordname;
    Frame *fr;
    int nwords;
    int64_t *nargs_by_word;    /* word index -> nargs */
    char **fname_by_word;      /* word index -> decompiled fn name */
    SBOut *w;
    /* per-fn used offset bookkeeping */
    unsigned char *usedoff;    /* indexed by offset; sized >= offset+1 */
    size_t usedoff_cap;
    int saw_return;
    /* program-level global usage */
    unsigned char *gused;      /* indexed by global addr (>=2) */
    size_t gused_cap;
    bool need_sgn, need_pow;
};

static void ctx_mark_glob(SimCtx *c, int64_t addr) {
    if (addr < 2) return;                      /* SP/FP */
    if (c->gused == NULL) {
        c->gused_cap = 64;
        c->gused = sxmalloc(c->gused_cap);
        memset(c->gused, 0, c->gused_cap);
    }
    size_t slot = (size_t)addr;
    if (slot >= c->gused_cap) {
        size_t newcap = slot * 2 + 16;
        c->gused = sxrealloc(c->gused, newcap);
        memset(c->gused + c->gused_cap, 0, newcap - c->gused_cap);
        c->gused_cap = newcap;
    }
    c->gused[slot] = 1;
}

static void ctx_mark_off(SimCtx *c, int64_t off) {
    if (off < 1) return;
    if (c->usedoff == NULL) {
        c->usedoff_cap = 64;
        c->usedoff = sxmalloc(c->usedoff_cap);
        memset(c->usedoff, 0, c->usedoff_cap);
    }
    size_t slot = (size_t)off;
    if (slot >= c->usedoff_cap) {
        size_t newcap = slot * 2 + 16;
        c->usedoff = sxrealloc(c->usedoff, newcap);
        memset(c->usedoff + c->usedoff_cap, 0, (newcap - c->usedoff_cap));
        c->usedoff_cap = newcap;
    }
    c->usedoff[slot] = 1;
}

static void ctx_init_off(SimCtx *c) {
    free(c->usedoff);
    c->usedoff = NULL; c->usedoff_cap = 0;
}

/* offset -> param/local name (params first, then locals) */
static void ctx_slotname(SimCtx *c, int64_t off, char *buf, size_t cap) {
    if (off <= c->fr->nargs) snprintf(buf, cap, "a%lld", (long long)off);
    else snprintf(buf, cap, "_s%lld", (long long)off);
}

static void stack_push(Stack *s, Sv v) {
    if (s->n == s->cap) {
        s->cap = s->cap ? s->cap * 2 : 16;
        s->it = sxrealloc(s->it, s->cap * sizeof(Sv));
    }
    s->it[s->n++] = v;
}

static Sv stack_pop(Stack *s, const char *word, const char *what) {
    if (s->n == 0) {
        sdec_err("sdec: word '%s': stack underflow at '%s' (input is not scc-shaped)",
                 word, what);
        longjmp(g_sdec_jb, 1);
    }
    return s->it[--s->n];
}

static Sv sv_none(EN *e) {
    Sv v; memset(&v, 0, sizeof(v));
    v.e = e;
    v.lv = LvNone;
    return v;
}

static bool en_is_name(EN *e, const char *nm) {
    return e && e->k == K_NAME && strcmp(e->name, nm) == 0;
}

static int binprec(const char *op) {
    static const struct { const char *o; int p; } T[] = {
        {"||",1},{"&&",2},{"|",3},{"^",4},{"&",5},
        {"==",6},{"!=",6},{"<",7},{">",7},{"<=",7},{">=",7},
        {"<<",8},{">>",8},{"+",9},{"-",9},{"*",10},{"/",10},{"%",10}
    };
    for (size_t i = 0; i < sizeof(T)/sizeof(T[0]); i++)
        if (strcmp(op, T[i].o) == 0) return T[i].p;
    return 9;
}

static EN *en_bina(const char *op, EN *a, EN *b) {
    EN *e = en_new(K_BIN, binprec(op));
    e->op = (char *)op;
    e->a = a;
    e->b = b;
    return e;
}

static EN *en_call(const char *nm, size_t narg, EN **args) {
    EN *e = en_new(K_CALL, 11);
    e->name = (char *)nm;
    for (size_t i = 0; i < narg; i++) en_argpush(e, args[i]);
    return e;
}

static EN *en_builtin(const char *nm, size_t narg, EN **args) {
    EN *e = en_new(K_BUILTIN, 11);
    e->name = (char *)nm;
    for (size_t i = 0; i < narg; i++) en_argpush(e, args[i]);
    return e;
}

static EN *en_name(const char *nm) {
    EN *e = en_new(K_NAME, 11);
    e->name = sxstrdup(nm);
    return e;
}

static EN *en_num(int64_t v) {
    EN *e = en_new(K_NUM, 11);
    e->num = v;
    return e;
}

static EN *en_idx(EN *base, EN *idxnode) {
    /* base must be K_NAME */
    EN *e = en_new(K_IDX, 11);
    e->name = base->name;
    e->b = idxnode;
    return e;
}

static EN *en_un(const char *op, EN *a, int prec) {
    EN *e = en_new(K_UN, prec);
    e->op = (char *)op;
    e->a = a;
    return e;
}

static void emit_expr_stmt(SimCtx *c, EN *e) {
    SBOut *w = c->w;
    for (int i = 0; i < w->ind; i++) sbo_put(w, "    ");
    en_render(w, e);
    sbo_put(w, ";\n");
}

static void emit_store_stmt(SimCtx *c, const char *nm, EN *idx, EN *v) {
    SBOut *w = c->w;
    for (int i = 0; i < w->ind; i++) sbo_put(w, "    ");
    sbo_put(w, nm);
    if (idx) { sbo_put(w, "["); en_render(w, idx); sbo_put(w, "]"); }
    sbo_put(w, " = ");
    en_render(w, v);
    sbo_put(w, ";\n");
}

static void emit_return_stmt(SimCtx *c, EN *v) {
    SBOut *w = c->w;
    for (int i = 0; i < w->ind; i++) sbo_put(w, "    ");
    sbo_put(w, "return ");
    en_render(w, v);
    sbo_put(w, ";\n");
    c->saw_return = true;
}

/* run ops list as a statement pipeline; final stack count is returned */
static void sim_run(SimCtx *c, Ops *o, Stack *st, int depth) {
    if (depth > 500) {
        sdec_err("sdec: word '%s': nesting too deep", c->wordname);
        longjmp(g_sdec_jb, 1);
    }
    for (size_t i = 0; i < o->n; i++) {
        AOp *op = o->it[i];
        switch (op->k) {
        case APUSH:
            stack_push(st, sv_none(en_num(op->v)));
            st->it[st->n - 1].lv = LvGlob;
            st->it[st->n - 1].off = op->v;
            break;
        case ADUP: case ASWAP: case AROT: {
            Sv a = stack_pop(st, c->wordname, op->name);
            if (op->k == ADUP) {
                stack_push(st, a);
                stack_push(st, a);
            } else if (op->k == ASWAP) {
                Sv b = stack_pop(st, c->wordname, "swap");
                stack_push(st, a);
                stack_push(st, b);
            } else {
                Sv b = stack_pop(st, c->wordname, "rot");
                Sv cc = stack_pop(st, c->wordname, "rot");
                stack_push(st, b);
                stack_push(st, a);
                stack_push(st, cc);
            }
            break;
        }
        case ANEG: {
            Sv a = stack_pop(st, c->wordname, "neg");
            EN *n;
            if (a.e->k == K_NUM) n = en_num(-a.e->num);
            else n = en_un("-", a.e, 11);
            stack_push(st, sv_none(n));
            break;
        }
        case AADD: {
            Sv b = stack_pop(st, c->wordname, "add");
            Sv a = stack_pop(st, c->wordname, "add");
            /* FP-offset lvalue chain detection */
            if (en_is_name(a.e, "FP") && b.e->k == K_NUM) {
                Sv r = a;
                r.e = en_bina("+", a.e, b.e);
                r.lv = LvFp;
                r.off = b.e->num;
                r.idx = NULL;
                stack_push(st, r);
                ctx_mark_off(c, r.off);
                break;
            }
            if (a.lv == LvFp && a.idx == NULL) {
                Sv r = a;
                r.e = en_bina("+", a.e, b.e);
                r.idx = b.e;
                stack_push(st, r);
                break;
            }
            if (a.lv == LvGlob && a.idx == NULL && a.off > 0) {
                Sv r = a;
                r.e = en_bina("+", a.e, b.e);
                r.idx = b.e;
                stack_push(st, r);
                break;
            }
            stack_push(st, sv_none(en_bina("+", a.e, b.e)));
            break;
        }
        case ASUB: case AMUL: case ADIV: case AMOD:
        case AXOR: case ASHL: case ASHR: {
            Sv b = stack_pop(st, c->wordname, op->name);
            Sv a = stack_pop(st, c->wordname, op->name);
            const char *opstr =
                op->k == ASUB ? "-" : op->k == AMUL ? "*" : op->k == ADIV ? "/"
                : op->k == AMOD ? "%" : op->k == AXOR ? "^" : op->k == ASHL ? "<<"
                : ">>";
            stack_push(st, sv_none(en_bina(opstr, a.e, b.e)));
            break;
        }
        case AAND: case AOR: {
            Sv b = stack_pop(st, c->wordname, op->name);
            Sv a = stack_pop(st, c->wordname, op->name);
            stack_push(st, sv_none(en_bina(op->k == AAND ? "&" : "|", a.e, b.e)));
            break;
        }
        case AEQ: case ANE: case ALT: case AGT: case ALE: case AGE: {
            Sv b = stack_pop(st, c->wordname, op->name);
            Sv a = stack_pop(st, c->wordname, op->name);
            stack_push(st, sv_none(en_bina(op->name, a.e, b.e)));
            break;
        }
        case ALAND: case ALOR: {
            Sv b = stack_pop(st, c->wordname, op->name);
            Sv a = stack_pop(st, c->wordname, op->name);
            stack_push(st, sv_none(en_bina(op->name, a.e, b.e)));
            break;
        }
        case ANOT: {
            Sv a = stack_pop(st, c->wordname, "!");
            stack_push(st, sv_none(en_un("!", a.e, 11)));
            break;
        }
        case ANZ: {
            Sv a = stack_pop(st, c->wordname, "sgn^2");
            stack_push(st, sv_none(en_bina("!=", a.e, en_num(0))));
            break;
        }        case ASIGN: {
            Sv a = stack_pop(st, c->wordname, "sign");
            c->need_sgn = true;
            EN *args[1] = { a.e };
            stack_push(st, sv_none(en_call("_sgn", 1, args)));
            break;
        }
        case APOW: {
            Sv b = stack_pop(st, c->wordname, "pow");
            Sv a = stack_pop(st, c->wordname, "pow");
            c->need_pow = true;
            EN *args[2] = { a.e, b.e };
            stack_push(st, sv_none(en_call("_pow", 2, args)));
            break;
        }
        case ALOAD: {
            Sv a = stack_pop(st, c->wordname, "load");
            EN *n;
            if (a.lv == LvGlob || (a.e->k == K_NUM && (a.off == 0 || a.off == 1))) {
                char nm[32];
                gpname(a.off < 0 ? 0 : a.off, nm, sizeof(nm));
                if (a.idx) n = en_idx(en_name(nm), a.idx);
                else n = en_name(nm);
                ctx_mark_glob(c, a.off);
            } else if (a.lv == LvFp) {
                if (a.off >= c->fr->roff) {
                    sdec_err("sdec: word '%s': read of frame result slot",
                             c->wordname);
                    longjmp(g_sdec_jb, 1);
                }
                char nm[32];
                ctx_slotname(c, a.off, nm, sizeof(nm));
                if (a.idx) n = en_idx(en_name(nm), a.idx);
                else n = en_name(nm);
                ctx_mark_off(c, a.off);
            } else {
                EN *args[1] = { a.e };
                n = en_builtin("peek", 1, args);
            }
            stack_push(st, sv_none(n));
            break;
        }
        case ASTORE: {
            Sv v = stack_pop(st, c->wordname, "store");
            Sv a = stack_pop(st, c->wordname, "store");
            if (a.lv == LvGlob || (a.e->k == K_NUM && (a.off == 0 || a.off == 1))) {
                char nm[32];
                gpname(a.off, nm, sizeof(nm));
                emit_store_stmt(c, nm, a.idx, v.e);
                ctx_mark_glob(c, a.off);
            } else if (a.lv == LvFp) {
                if (a.idx == NULL && a.off == c->fr->roff) {
                    emit_return_stmt(c, v.e);
                } else if (a.idx == NULL && a.off == c->fr->size - 1) {
                    sdec_err("sdec: word '%s': stray done-flag store", c->wordname);
                    longjmp(g_sdec_jb, 1);
                } else {
                    char nm[32];
                    ctx_slotname(c, a.off, nm, sizeof(nm));
                    emit_store_stmt(c, nm, a.idx, v.e);
                    ctx_mark_off(c, a.off);
                }
            } else {
                EN *args[2] = { a.e, v.e };
                emit_expr_stmt(c, en_builtin("poke", 2, args));
            }
            break;
        }
        case ADROP: {
            Sv a = stack_pop(st, c->wordname, "drop");
            emit_expr_stmt(c, a.e);
            break;
        }
        case ANUMOUT: {
            Sv a = stack_pop(st, c->wordname, "num-out");
            EN *args[1] = { a.e };
            emit_expr_stmt(c, en_builtin("putn", 1, args));
            break;
        }
        case ACHAROUT: {
            Sv a = stack_pop(st, c->wordname, "char-out");
            EN *args[1] = { a.e };
            emit_expr_stmt(c, en_builtin("putc", 1, args));
            break;
        }
        case ASLEEP: {
            Sv a = stack_pop(st, c->wordname, "sleep");
            EN *args[1] = { a.e };
            emit_expr_stmt(c, en_builtin("sleep", 1, args));
            break;
        }
        case ACHARIN: {
            EN *args[1] = { NULL };
            EN *n = en_builtin("getc", 0, args);
            (void)args;
            stack_push(st, sv_none(n));
            break;
        }
        case ARAND: {
            Sv a = stack_pop(st, c->wordname, "rand");
            EN *args[1] = { a.e };
            stack_push(st, sv_none(en_builtin("rnd", 1, args)));
            break;
        }
        case ACALL: {
            if (op->v >= (int64_t)c->nwords || op->v < 0) {
                sdec_err("sdec: word '%s': call index out of range", c->wordname);
                longjmp(g_sdec_jb, 1);
            }
            int64_t n = c->nargs_by_word[op->v];
            EN **args = sxmalloc((size_t)sizeof(EN *) * (n ? n : 1));
            Sv *pops = sxmalloc((size_t)sizeof(Sv) * (n ? n : 1));
            for (int64_t j = 0; j < n; j++)
                pops[n - 1 - j] = sv_none(stack_pop(st, c->wordname, "call args").e);
            for (int64_t j = 0; j < n; j++) args[j] = pops[j].e;
            free(pops);
            EN *nc = en_call(c->fname_by_word[op->v], (size_t)n, args);
            free(args);
            stack_push(st, sv_none(nc));
            break;
        }
        case AHALT: {
            EN *args[1] = { NULL };
            emit_expr_stmt(c, en_builtin("exit", 0, args));
            (void)args;
            break;
        }
        case ADONESET:
            break;
        case AIF: {
            Sv cond = stack_pop(st, c->wordname, "if");
            if (st->n != 0) {
                fprintf(stderr, "DBGIF word=%s ops#%lu cond_k=%d leftover_k: ", c->wordname,
                        (unsigned long)i, cond.e->k);
                for (size_t di = 0; di < st->n; di++)
                    fprintf(stderr, " %d", st->it[di].e->k);
                fprintf(stderr, "\n");
                sdec_err("sdec: word '%s': unbalanced stack before 'if'",
                         c->wordname);
                longjmp(g_sdec_jb, 1);
            }
            SBOut *w = c->w;
            for (int i = 0; i < w->ind; i++) sbo_put(w, "    ");
            sbo_put(w, "if (");
            en_render(w, cond.e);
            sbo_put(w, ") {\n");
            w->ind++;
            sim_run(c, op->a, st, depth + 1);
            w->ind--;
            for (int i = 0; i < w->ind; i++) sbo_put(w, "    ");
            if (op->b) {
                sbo_put(w, "} else {\n");
                w->ind++;
                sim_run(c, op->b, st, depth + 1);
                w->ind--;
                for (int i = 0; i < w->ind; i++) sbo_put(w, "    ");
            }
            sbo_put(w, "}\n");
            break;
        }
        case AWILE: {
            Stack *cst = sxmalloc(sizeof(Stack));
            memset(cst, 0, sizeof(Stack));
            sim_run(c, op->a, cst, depth + 1);
            if (cst->n != 1) {
                fprintf(stderr, "DEBUG cond ops:");
                for (size_t di = 0; di < op->a->n; di++)
                    fprintf(stderr, " %d", op->a->it[di]->k);
                fprintf(stderr, "\n");
                sdec_err("sdec: word '%s': while condition is not a pure "
                         "expression (%zu values)", c->wordname, cst->n);
                longjmp(g_sdec_jb, 1);
            }
            EN *cond = cst->it[0].e;
            free(cst->it);
            free(cst);
            SBOut *w = c->w;
            for (int i = 0; i < w->ind; i++) sbo_put(w, "    ");
            sbo_put(w, "while (");
            en_render(w, cond);
            sbo_put(w, ") {\n");
            w->ind++;
            sim_run(c, op->b, st, depth + 1);
            w->ind--;
            for (int i = 0; i < w->ind; i++) sbo_put(w, "    ");
            sbo_put(w, "}\n");
            break;
        }
        default: {
            static const char *UNSUP[] = {
                "sock-create","sock-bind","sock-listen","sock-accept","sock-recv",
                "sock-send","close","file-open","file-read","file-stat","dir-list",
                "id", NULL
            };
            (void)UNSUP;
            const char *nm = op->name ? op->name : "?";
            sdec_err("sdec: word '%s': op '%s' has no Scribe form yet", c->wordname, nm);
            longjmp(g_sdec_jb, 1);
        }
        }
    }
}

/* ========== program-level glue ========== */
typedef struct { int64_t addr; int64_t val; } GInit;
typedef struct { GInit *it; size_t n, cap; } GInits;
static void ginit_push(GInits *g, int64_t a, int64_t v) {
    if (g->n == g->cap) {
        g->cap = g->cap ? g->cap * 2 : 8;
        g->it = sxrealloc(g->it, g->cap * sizeof(GInit));
    }
    g->it[g->n].addr = a;
    g->it[g->n].val = v;
    g->n++;
}
static bool ginit_find(GInits *g, int64_t a, int64_t *v) {
    for (size_t i = 0; i < g->n; i++)
        if (g->it[i].addr == a) { *v = g->it[i].val; return true; }
    return false;
}

typedef struct {
    char *qname;
    char *fname;
    Ops *ops;
    Frame fr;
} Word;

static const char *SCRIBE_KEYWORDS[] = {
    "fn","let","if","else","while","for","return","SP","FP","_program",
    "putc","putn","getc","peek","poke","sleep","rnd","exit", NULL
};
static bool scribe_reserved(const char *s) {
    for (int i = 0; SCRIBE_KEYWORDS[i]; i++)
        if (strcmp(s, SCRIBE_KEYWORDS[i]) == 0) return true;
    return false;
}

/* walk the scc `_program` wrapper word: SP/FP lets, global inits, main call */
static PStat analyze_program(Word *prog, int nwords,
                             GInits *inits, int64_t *sp_init, int *main_idx) {
    Ops *o = prog->ops;
    int64_t spseen = -2, fpseen = -3;
    size_t j = 0;
    bool call_seen = false, drop_seen = false, halt_seen = false;
    *main_idx = -1;
    while (j < o->n) {
        if (j + 2 < o->n &&
            opk(o, j, APUSH) && opk(o, j + 1, APUSH) && opk(o, j + 2, ASTORE)) {
            int64_t k = o->it[j]->v, v = o->it[j + 1]->v;
            if (k == 0) {
                if (spseen != -2) {
                    sdec_err("sdec: duplicate SP initializer in _program");
                    return PR_ERROR;
                }
                spseen = v; j += 3; continue;
            }
            if (k == 1) {
                if (fpseen != -3) {
                    sdec_err("sdec: duplicate FP initializer in _program");
                    return PR_ERROR;
                }
                if (v != 0) {
                    sdec_err("sdec: FP must be initialized to 0 in _program");
                    return PR_ERROR;
                }
                fpseen = v; j += 3; continue;
            }
            if (spseen < 0 || fpseen < 0) {
                sdec_err("sdec: uninitialized side-datum (global init before SP/FP) in _program");
                return PR_ERROR;
            }
            size_t inits_n = inits->n;
            if (inits_n > 0 && inits->it[inits_n - 1].addr >= k) {
                sdec_err("sdec: global initializers must ascend in _program");
                return PR_ERROR;
            }
            ginit_push(inits, k, v);
            j += 3;
            continue;
        }
        if (!call_seen && o->it[j]->k == ACALL) {
            *main_idx = (int)o->it[j]->v;
            if (*main_idx < 1 || *main_idx >= nwords) {
                sdec_err("sdec: _program calls word %d out of range", *main_idx);
                return PR_ERROR;
            }
            call_seen = true; j++; continue;
        }
        if (call_seen && !drop_seen && o->it[j]->k == ADROP) { drop_seen = true; j++; continue; }
        if (drop_seen && !halt_seen && o->it[j]->k == AHALT) { halt_seen = true; j++; continue; }
        sdec_err("sdec: unexpected op in _program at %zu", j);
        return PR_ERROR;
    }
    if (spseen < 0 || fpseen != 0 || !call_seen || !drop_seen || !halt_seen) {
        sdec_err("sdec: _program doesn't match the scc entry shape");
        return PR_ERROR;
    }
    *sp_init = spseen;
    return PR_OK;
}

/* turn a Quill word name into a Scribe fn name */
static bool name_is_synthetic(const char *s) {
    /* local/pad/global namespace: _s123, _p123, g123 */
    static const char *PRE[] = {"_s", "_p", "g", NULL};
    for (int i = 0; PRE[i]; i++) {
        size_t pl = strlen(PRE[i]);
        if (strncmp(s, PRE[i], pl) != 0) continue;
        const char *r = s + pl;
        if (!*r) continue;
        bool alldig = true;
        for (size_t j = 0; r[j]; j++) if (!isdigit((unsigned char)r[j])) { alldig = false; break; }
        if (alldig) return true;
    }
    return false;
}

static char *make_fnname(const char *qn, char **used, size_t nused) {
    const char *base = NULL;
    /* plain words are called directly; scc prefixes with fn_ */
    if (strncmp(qn, "fn_", 3) == 0) base = qn + 3;
    else base = qn;
    char *cand = sxstrdup(base);
    bool bad = cand[0] == 0 || scribe_reserved(cand) || name_is_synthetic(cand) ||
               !tq_is_ident(cand);
    if (bad) {
        free(cand);
        cand = sxmalloc(strlen(qn) + 4);
        sprintf(cand, "fn_%s", qn);
    }
    for (size_t i = 0; i < nused; i++) {
        if (strcmp(cand, used[i]) == 0) {
            size_t l = strlen(cand);
            char *nc = sxmalloc(l + 16);
            sprintf(nc, "%s_%zu", cand, i + 2);
            free(cand);
            cand = nc;
            i = 0; /* recheck all */
        }
    }
    return cand;
}

static void put_global(SBOut *g, const char *name, const char *idx, int64_t *init) {
    sbo_put(g, "let ");
    sbo_put(g, name);
    if (idx) { sbo_put(g, "["), sbo_put(g, idx), sbo_put(g, "]"); }
    if (init) {
        char b[32];
        snprintf(b, sizeof(b), " = %lld", (long long)*init);
        sbo_put(g, b);
    }
    sbo_put(g, ";\n");
}

/* master entry: Quill text -> malloc'd Scribe text (or NULL + q_errmsg) */
char *sdec_decompile(const char *src, size_t len) {
    if (setjmp(g_sdec_jb)) return NULL;

    Toks tk;
    if (!sdec_tokenize(src, len, &tk)) return NULL;

    /* maps, mirroring q_assemble scan order */
    SMap slots = {0}, wmap = {0};
    size_t nwords = 0;
    size_t *wstart = NULL;
    size_t wstartn = 0, wstartcap = 0;
    for (size_t i = 0; i < tk.n; i++) {
        TQ *t = &tk.t[i];
        if (t->kind != QT_IDENT) continue;
        if (strcmp(t->s, "word") == 0) {
            if (i + 1 < tk.n && tk.t[i + 1].kind == QT_IDENT) {
                smap_push(&wmap, tk.t[i + 1].s, (int64_t)nwords++);
                if (wstartn == wstartcap) {
                    wstartcap = wstartcap ? wstartcap * 2 : 8;
                    wstart = sxrealloc(wstart, wstartcap * sizeof(size_t));
                }
                wstart[wstartn++] = i;
            }
        } else if (strcmp(t->s, "let") == 0) {
            if (i + 1 < tk.n && tk.t[i + 1].kind == QT_IDENT) {
                int64_t dummy;
                if (!smap_find(&slots, tk.t[i + 1].s, &dummy))
                    smap_push(&slots, tk.t[i + 1].s, (int64_t)smap_count(&slots));
            }
        }
    }
    if (nwords == 0) {
        sdec_err("sdec: no words found in input");
        return NULL;
    }

    Word *ws = sxmalloc(nwords * sizeof(Word));
    memset(ws, 0, nwords * sizeof(Word));
    Parser p;
    memset(&p, 0, sizeof(p));
    p.tk = &tk;
    p.slots = &slots;
    p.words = &wmap;
    for (size_t i = 0; i < nwords; i++) {
        size_t wpos = wstart[i];
        size_t wend = (i + 1 < nwords) ? wstart[i + 1] : tk.n;
        /* header: 'word' IDENT */
        ws[i].qname = tk.t[wpos + 1].s;
        p.pos = wpos + 2;
        p.end = wend;
        p.curword = ws[i].qname;
        Ops *ops = ops_new();
        PStat r = parse_ops(&p, ops, 0);
        if (r == PR_ERROR) return NULL;
        if (p.pos != p.end) {
            sdec_err("sdec: word '%s': unexpected trailing content", ws[i].qname);
            return NULL;
        }
        ws[i].ops = ops;
    }

    if (strcmp(ws[0].qname, "_program") != 0) {
        sdec_err("sdec: first word is '%s', not '_program' "
                 "(unscribe only supports ink scc output)", ws[0].qname);
        return NULL;
    }

    /* peel scc prologue/epilogue per fn word */
    for (size_t i = 1; i < nwords; i++) {
        p.curword = ws[i].qname;
        PStat r = peel_prologue(&p, ws[i].ops, &ws[i].fr);
        if (r == PR_ERROR) return NULL;
        if (r == PR_KEEP) {
            sdec_err("sdec: word '%s': no scc frame prologue — unscribe only "
                     "supports Quill produced by `ink scc`", ws[i].qname);
            return NULL;
        }
        /* strips + peephole */
        strip_recursive(ws[i].ops, &ws[i].fr);
        merge_peephole_in(ws[i].ops, &ws[i].fr);
        if (getenv("SDEC_DEBUG")) {
            fprintf(stderr, "word %s ops:\n", ws[i].qname);
            for (size_t di = 0; di < ws[i].ops->n; di++) {
                AOp *op = ws[i].ops->it[di];
                fprintf(stderr, "  [%zu] k=%d", di, op->k);
                if (op->k == APUSH || op->k == ACALL) fprintf(stderr, " v=%lld", (long long)op->v);
                if (op->k >= AEQ && op->k <= ANZ) fprintf(stderr, " '%s'", compound_opstr(op->k));
                if (op->k == AIF) fprintf(stderr, " f%s%zu", op->a ? "{...}" : "?", op->a ? op->a->n : 0);
                if (op->k == AWILE) fprintf(stderr, " len%s%zu", op->a ? "c=" : "?", op->a ? op->a->n : 0);
                fprintf(stderr, "\n");
                for (size_t dj = 0; op->a && dj < op->a->n; dj++)
                    fprintf(stderr, "   A[%zu] k=%d\n", dj, op->a->it[dj]->k);
                for (size_t dj = 0; op->b && dj < op->b->n; dj++)
                    fprintf(stderr, "   B[%zu] k=%d\n", dj, op->b->it[dj]->k);
            }
        }
    }

    /* program wrapper analysis */
    GInits inits = {0};
    int64_t sp_init = 0;
    int main_idx = -1;
    if (analyze_program(&ws[0], (int)nwords, &inits, &sp_init, &main_idx) != PR_OK)
        return NULL;
    if (strcmp(ws[main_idx].qname, "fn_main") != 0) {
        sdec_err("sdec: _program must call fn_main, got '%s'", ws[main_idx].qname);
        return NULL;
    }
    if (ws[main_idx].fr.nargs != 0) {
        sdec_err("sdec: fn_main takes %lld arguments; main() must be 0-ary",
                 (long long)ws[main_idx].fr.nargs);
        return NULL;
    }

    /* fn names */
    char **fnames = sxmalloc(nwords * sizeof(char *));
    fnames[0] = NULL;
    {
        char **used = sxmalloc(nwords * sizeof(char *));
        size_t nused = 0;
        for (size_t i = 1; i < nwords; i++)
            fnames[i] = make_fnname(ws[i].qname, used, nused), used[nused++] = fnames[i];
        free(used);
    }
    int64_t *nargsarr = sxmalloc(nwords * sizeof(int64_t));
    nargsarr[0] = 0;
    for (size_t i = 1; i < nwords; i++) nargsarr[i] = ws[i].fr.nargs;

    /* simulate each fn body; one shared SimCtx accumulates global usage */
    SimCtx c;
    memset(&c, 0, sizeof(c));
    c.nwords = (int)nwords;
    c.nargs_by_word = nargsarr;
    c.fname_by_word = fnames;

    SBOut **chunks = sxmalloc(nwords * sizeof(SBOut *));
    chunks[0] = NULL;
    for (size_t i = 1; i < nwords; i++) {
        c.wordname = ws[i].qname;
        c.fr = &ws[i].fr;
        ctx_init_off(&c);
        c.saw_return = false;
        SBOut *body = sbo_new();
        body->ind = 1;
        c.w = body;
        Stack st = {0};
        sim_run(&c, ws[i].ops, &st, 0);
        if (st.n != 0) {
            sdec_err("sdec: word '%s': unbalanced stack after body (%zu)",
                     ws[i].qname, st.n);
            return NULL;
        }
        free(st.it);

        /* decls: locals (scalars for used offsets, pads for gaps) */
        SBOut *declb = sbo_new();
        for (int64_t off = ws[i].fr.nargs + 1; off < ws[i].fr.size - 2; ) {
            bool u = c.usedoff && (size_t)off < c.usedoff_cap && c.usedoff[off];
            if (u) {
                char nm[32], ln[80];
                ctx_slotname(&c, off, nm, sizeof(nm));
                snprintf(ln, sizeof(ln), "    let %s;\n", nm);
                sbo_put(declb, ln);
                off++;
            } else {
                int64_t next = off + 1;
                while (next < ws[i].fr.size - 2 &&
                       !(c.usedoff && (size_t)next < c.usedoff_cap && c.usedoff[next]))
                    next++;
                char ln[96];
                snprintf(ln, sizeof(ln), "    let _p%lld[%lld];\n",
                         (long long)off, (long long)(next - off));
                sbo_put(declb, ln);
                off = next;
            }
        }

        /* chunk = "fn NAME(args) {\n" + decls + body + "}\n" */
        {
            size_t hl = (size_t)ws[i].fr.nargs * 12 + 64;
            char *h = sxmalloc(hl);
            size_t w = (size_t)snprintf(h, hl, "fn %s(", fnames[i]);
            for (int64_t a = 1; a <= ws[i].fr.nargs; a++) {
                if (a > 1) w += (size_t)snprintf(h + w, hl - w, ", ");
                w += (size_t)snprintf(h + w, hl - w, "a%lld", (long long)a);
            }
            snprintf(h + w, hl - w, ") {\n");
            size_t dl = strlen(h) + declb->n + body->n + 8;
            char *full = sxmalloc(dl);
            size_t fp = 0;
            memcpy(full, h, strlen(h)); fp += strlen(h);
            memcpy(full + fp, declb->d, declb->n); fp += declb->n;
            memcpy(full + fp, body->d, body->n); fp += body->n;
            memcpy(full + fp, "}\n", 3); fp += 3;
            free(h);
            free(declb->d); free(declb);
            free(body->d);
            body->d = full;
            body->cap = fp + 1;
            body->n = fp;
            body->ind = 0;
            chunks[i] = body;
        }
    }

    /* gather global anchors: initializer addresses + addresses used by fns */
    {
        int64_t *an = sxmalloc((inits.n ? inits.n : 1) * sizeof(int64_t) + 8);
        size_t nan = 0;
        for (size_t i = 0; i < inits.n; i++) an[nan++] = inits.it[i].addr;
        if (c.gused) {
            for (size_t a = 2; a < c.gused_cap; a++)
                if (c.gused[a]) {
                    bool dup = false;
                    for (size_t i = 0; i < nan; i++) if (an[i] == (int64_t)a) dup = true;
                    if (!dup) {
                        an = sxrealloc(an, (nan + 2) * sizeof(int64_t));
                        an[nan++] = (int64_t)a;
                    }
                }
        }
        for (size_t i = 0; i < nan; i++)
            for (size_t j = i + 1; j < nan; j++)
                if (an[j] < an[i]) { int64_t t = an[i]; an[i] = an[j]; an[j] = t; }

        SBOut *out = sbo_new();
        out->ind = 0;
        sbo_put(out, "// decompiled from scc-shaped Quill by ink unscribe\n");
        sbo_put(out, "// (recompiling with `ink scc` regenerates let SP/FP automatically)\n");

        int64_t prev = 2;
        for (size_t i = 0; i < nan; i++) {
            int64_t a = an[i];
            if (a < prev) {
                sdec_err("sdec: overlapping global at %lld", (long long)a);
                return NULL;
            }
            if (a > prev) {
                char pb[96];
                snprintf(pb, sizeof(pb), "let _p%lld[%lld];\n",
                         (long long)prev, (long long)(a - prev));
                sbo_put(out, pb);
            }
            int64_t *iv = NULL, dummy;
            if (ginit_find(&inits, a, &dummy)) iv = &dummy;
            {
                char gb[40];
                snprintf(gb, sizeof(gb), "g%lld", (long long)a);
                size_t save_n = out->n;
                put_global(out, gb, NULL, iv);
                (void)save_n;
            }
            prev = a + 1;
        }
        if (prev < sp_init) {
            char pb[96];
            snprintf(pb, sizeof(pb), "let _p%lld[%lld];\n",
                     (long long)prev, (long long)(sp_init - prev));
            sbo_put(out, pb);
            prev = sp_init;
        }
        if (prev > sp_init) {
            sdec_err("sdec: globals reach %lld but SP init is %lld",
                     (long long)prev, (long long)sp_init);
            return NULL;
        }

        sbo_put(out, "\n");
        for (size_t i = 1; i < nwords; i++) {
            sbo_put(out, chunks[i]->d);
            sbo_put(out, "\n");
        }

        if (c.need_sgn) {
            sbo_put(out,
                "fn _sgn(a) {\n"
                "    if (a < 0) {\n"
                "        return 0 - 1;\n"
                "    }\n"
                "    if (a > 0) {\n"
                "        return 1;\n"
                "    }\n"
                "    return 0;\n"
                "}\n\n");
        }
        if (c.need_pow) {
            sbo_put(out,
                "fn _pow(a, b) {\n"
                "    if (b < 0) {\n"
                "        return 0;\n"
                "    }\n"
                "    let r = 1;\n"
                "    while (b > 0) {\n"
                "        r = r * a;\n"
                "        b = b - 1;\n"
                "    }\n"
                "    return r;\n"
                "}\n\n");
        }

        char *result = sxmalloc(out->n + 1);
        memcpy(result, out->d, out->n);
        result[out->n] = 0;
        free(out->d); free(out);
        free(an);
        return result;
    }
}




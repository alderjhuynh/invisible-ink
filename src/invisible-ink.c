#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <setjmp.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>

typedef uint32_t cp_t;

enum {
    CP_ITIMES  = 0x2062, /* dup -- INVISIBLE TIMES */
    CP_NBSP    = 0x00A0, /* swap */
    CP_FIGURE  = 0x2007, /* drop */
    CP_THIN    = 0x2009, /* rot */
    CP_HAIR    = 0x200A, /* begin literal */
    CP_DIG0    = 0x2005, /* literal digit 0 */
    CP_DIG1    = 0x2006, /* literal digit 1 */
    CP_IDEO    = 0x3000, /* num-out */
    CP_PUNCT   = 0x2008, /* char-out */
    CP_MEDMATH = 0x205F, /* char-in */
    CP_EN      = 0x2002, /* arity: neg / sub / mod */
    CP_EM      = 0x2003, /* arity: id / add / div */
    CP_THREE   = 0x2004, /* arity: sign / mul / pow */
    CP_ZWNJ    = 0x200C, /* block open */
    CP_ZWJ     = 0x200D, /* block close */
    CP_WJ      = 0x2060, /* if */
    CP_FA      = 0x2061, /* call */
    CP_LS      = 0x2000, /* statement boundary / literal terminator */
    CP_PS      = 0x2001, /* word boundary */
    CP_ZWSP    = 0x200B, /* terminator */
    CP_BOM     = 0xFEFF, /* begin embedded program */
    CP_INV_SEP = 0x2063, /* rand */
    CP_INV_PLUS= 0x2064, /* store */
    CP_LRM     = 0x200E, /* load */
    CP_RLM     = 0x200F, /* sleep */
    CP_SOFT    = 0x00AD  /* soft hyphen: alternate literal terminator (close+char-out) */
};

static bool is_spec_cp(cp_t c) {
    switch (c) {
        case CP_ITIMES: case CP_NBSP: case CP_FIGURE: case CP_THIN:
        case CP_HAIR: case CP_DIG0: case CP_DIG1:
        case CP_IDEO: case CP_PUNCT: case CP_MEDMATH:
        case CP_EN: case CP_EM: case CP_THREE:
        case CP_ZWNJ: case CP_ZWJ: case CP_WJ: case CP_FA:
        case CP_LS: case CP_PS: case CP_ZWSP: case CP_BOM:
        case CP_INV_SEP: case CP_INV_PLUS: case CP_LRM: case CP_RLM: case CP_SOFT:
            return true;
        default:
            return false;
    }
}

static cp_t *utf8_decode(const unsigned char *buf, size_t len, size_t *out_count) {
    cp_t *out = malloc(len * sizeof(cp_t)); /* upper bound: len codepoints */
    if (!out) { perror("malloc"); exit(1); }
    size_t n = 0, i = 0;
    while (i < len) {
        unsigned char b0 = buf[i];
        uint32_t cp; int extra;
        if (b0 < 0x80)              { cp = b0; extra = 0; }
        else if ((b0 & 0xE0) == 0xC0){ cp = b0 & 0x1F; extra = 1; }
        else if ((b0 & 0xF0) == 0xE0){ cp = b0 & 0x0F; extra = 2; }
        else if ((b0 & 0xF8) == 0xF0){ cp = b0 & 0x07; extra = 3; }
        else { out[n++] = 0xFFFD; i++; continue; }

        if (i + (size_t)extra >= len) { out[n++] = 0xFFFD; i++; continue; }
        bool ok = true;
        uint32_t val = cp;
        for (int k = 1; k <= extra; k++) {
            unsigned char bk = buf[i + k];
            if ((bk & 0xC0) != 0x80) { ok = false; break; }
            val = (val << 6) | (bk & 0x3F);
        }
        if (!ok) { out[n++] = 0xFFFD; i++; continue; }
        out[n++] = val;
        i += (size_t)extra + 1;
    }
    *out_count = n;
    return out;
}

static void utf8_put(int64_t code) {
    uint32_t c;
    if (code < 0 || code > 0x10FFFF || (code >= 0xD800 && code <= 0xDFFF)) {
        c = 0xFFFD;
    } else {
        c = (uint32_t)code;
    }
    if (c < 0x80) {
        putchar((int)c);
    } else if (c < 0x800) {
        putchar((int)(0xC0 | (c >> 6)));
        putchar((int)(0x80 | (c & 0x3F)));
    } else if (c < 0x10000) {
        putchar((int)(0xE0 | (c >> 12)));
        putchar((int)(0x80 | ((c >> 6) & 0x3F)));
        putchar((int)(0x80 | (c & 0x3F)));
    } else {
        putchar((int)(0xF0 | (c >> 18)));
        putchar((int)(0x80 | ((c >> 12) & 0x3F)));
        putchar((int)(0x80 | ((c >> 6) & 0x3F)));
        putchar((int)(0x80 | (c & 0x3F)));
    }
}

static int64_t utf8_get_stdin(void) {
    int b0 = getchar();
    if (b0 == EOF) return -1;
    uint32_t cp; int extra;
    if (b0 < 0x80)               { cp = (uint32_t)b0; extra = 0; }
    else if ((b0 & 0xE0) == 0xC0){ cp = (uint32_t)(b0 & 0x1F); extra = 1; }
    else if ((b0 & 0xF0) == 0xE0){ cp = (uint32_t)(b0 & 0x0F); extra = 2; }
    else if ((b0 & 0xF8) == 0xF0){ cp = (uint32_t)(b0 & 0x07); extra = 3; }
    else return 0xFFFD;
    for (int k = 0; k < extra; k++) {
        int bk = getchar();
        if (bk == EOF) return 0xFFFD;
        if ((bk & 0xC0) != 0x80) return 0xFFFD;
        cp = (cp << 6) | (uint32_t)(bk & 0x3F);
    }
    return (int64_t)cp;
}

typedef enum {
    OP_PUSH, OP_DUP, OP_SWAP, OP_DROP, OP_ROT,
    OP_NEG, OP_SUB, OP_MOD, OP_ID, OP_ADD, OP_DIV, OP_SIGN, OP_MUL, OP_POW,
    OP_NUMOUT, OP_CHAROUT, OP_CHARIN,
    OP_CALL, OP_IF, OP_HALT,
    OP_RAND, OP_STORE, OP_LOAD, OP_SLEEP
} OpCode;

typedef struct Block Block;

typedef struct {
    OpCode op;
    int64_t value;    /* OP_PUSH */
    Block *then_blk;  /* OP_IF */
    Block *else_blk;  /* OP_IF, nullable */
} Instr;

struct Block {
    Instr *items;
    size_t count;
    size_t cap;
};

static Block *block_new(void) {
    Block *b = malloc(sizeof(Block));
    b->items = NULL; b->count = 0; b->cap = 0;
    return b;
}

static Instr *block_push(Block *b, OpCode op) {
    if (b->count == b->cap) {
        b->cap = b->cap ? b->cap * 2 : 8;
        b->items = realloc(b->items, b->cap * sizeof(Instr));
    }
    Instr *ins = &b->items[b->count++];
    memset(ins, 0, sizeof(Instr));
    ins->op = op;
    return ins;
}

static const char *g_src_name = "program";

static void parse_error(const char *msg, size_t pos) {
    fprintf(stderr, "%s: parse error: %s (at codepoint %zu)\n", g_src_name, msg, pos);
    exit(1);
}

static OpCode arity_op(cp_t ch, int count) {
    if (ch == CP_EN)    return count == 1 ? OP_NEG  : count == 2 ? OP_SUB : OP_MOD;
    if (ch == CP_EM)    return count == 1 ? OP_ID   : count == 2 ? OP_ADD : OP_DIV;
    /* CP_THREE */       return count == 1 ? OP_SIGN : count == 2 ? OP_MUL : OP_POW;
}

static Block *parse_block(const cp_t *cps, size_t len, size_t *pos, bool stop_at_zwj) {
    Block *blk = block_new();
    size_t i = *pos;

    while (i < len) {
        cp_t ch = cps[i];

        if (stop_at_zwj && ch == CP_ZWJ) { i++; *pos = i; return blk; }

        if (ch == CP_LS) { i++; continue; }

        if (ch == CP_ZWSP) { block_push(blk, OP_HALT); i++; continue; }

        if (ch == CP_HAIR) {
            i++;
            int64_t bits = 0;
            size_t nbits = 0;
            while (i < len && (cps[i] == CP_DIG0 || cps[i] == CP_DIG1)) {
                bits = bits * 2 + (cps[i] == CP_DIG1 ? 1 : 0);
                nbits++;
                i++;
            }
            if (nbits == 0) parse_error("empty literal", i);
            if (i >= len || (cps[i] != CP_LS && cps[i] != CP_SOFT))
                parse_error("unterminated literal (expected LINE SEPARATOR or SOFT HYPHEN)", i);
            bool soft = (cps[i] == CP_SOFT);
            i++;
            Instr *ins = block_push(blk, OP_PUSH);
            ins->value = bits;
            if (soft) {
                block_push(blk, OP_CHAROUT);
            }
            continue;
        }

        if (ch == CP_WJ) {
            i++;
            if (i >= len || cps[i] != CP_ZWNJ) parse_error("expected block open after IF", i);
            i++;
            size_t p = i;
            Block *then_b = parse_block(cps, len, &p, true);
            i = p;
            Block *else_b = NULL;
            if (i < len && cps[i] == CP_ZWNJ) {
                i++;
                p = i;
                else_b = parse_block(cps, len, &p, true);
                i = p;
            }
            Instr *ins = block_push(blk, OP_IF);
            ins->then_blk = then_b;
            ins->else_blk = else_b;
            continue;
        }

        if (ch == CP_EN || ch == CP_EM || ch == CP_THREE) {
            int count = 1;
            while (i + (size_t)count < len && cps[i + (size_t)count] == ch && count < 3) count++;
            block_push(blk, arity_op(ch, count));
            i += (size_t)count;
            continue;
        }

        switch (ch) {
            case CP_ITIMES: block_push(blk, OP_DUP);     i++; break;
            case CP_NBSP:   block_push(blk, OP_SWAP);    i++; break;
            case CP_FIGURE: block_push(blk, OP_DROP);    i++; break;
            case CP_THIN:   block_push(blk, OP_ROT);     i++; break;
            case CP_IDEO:   block_push(blk, OP_NUMOUT);  i++; break;
            case CP_PUNCT:  block_push(blk, OP_CHAROUT); i++; break;
            case CP_MEDMATH:block_push(blk, OP_CHARIN);  i++; break;
            case CP_FA:     block_push(blk, OP_CALL);    i++; break;
            case CP_INV_SEP:block_push(blk, OP_RAND);    i++; break;
            case CP_INV_PLUS:block_push(blk, OP_STORE);  i++; break;
            case CP_LRM:    block_push(blk, OP_LOAD);    i++; break;
            case CP_RLM:    block_push(blk, OP_SLEEP);   i++; break;
            case CP_SOFT:   /* ignored outside literal */ i++; break;
            case CP_ZWJ:    i++; break; 
            case CP_ZWNJ:   i++; break; 
            case CP_BOM:    i++; break; 
            default:        i++; break;
        }
    }
    *pos = i;
    return blk;
}

static cp_t *extract_program(const cp_t *full, size_t full_len, size_t *out_len, bool *embedded) {
    size_t bom_idx = (size_t)-1;
    for (size_t i = 0; i < full_len; i++) {
        if (full[i] == CP_BOM) { bom_idx = i; break; }
    }

    size_t start, end;
    if (bom_idx == (size_t)-1) {
        start = 0; end = full_len;
        *embedded = false;
    } else {
        start = bom_idx + 1;
        end = full_len;
        for (size_t i = start; i < full_len; i++) {
            if (full[i] == CP_ZWSP) { end = i; break; }
        }
        *embedded = true;
    }

    cp_t *out = malloc((end - start) * sizeof(cp_t));
    size_t n = 0;
    for (size_t i = start; i < end; i++) {
        if (is_spec_cp(full[i])) out[n++] = full[i];
    }
    *out_len = n;
    return out;
}

static Block **split_and_parse_words(const cp_t *cps, size_t len, size_t *out_word_count) {
    size_t word_count = 1;
    for (size_t i = 0; i < len; i++) if (cps[i] == CP_PS) word_count++;

    Block **words = malloc(word_count * sizeof(Block *));
    size_t w = 0, seg_start = 0;
    for (size_t i = 0; i <= len; i++) {
        if (i == len || cps[i] == CP_PS) {
            size_t seg_len = i - seg_start;
            size_t p = 0;
            words[w++] = parse_block(cps + seg_start, seg_len, &p, false);
            seg_start = i + 1;
        }
    }
    *out_word_count = w;
    return words;
}

#define MAX_STEPS 10000000UL
#define MAX_DEPTH 20000

static Block **g_words;
static size_t g_word_count;

static int64_t *g_stack;
static size_t g_sp, g_cap;

static int64_t *g_mem;
static size_t g_mem_cap;

static uint64_t g_prng_state;
static bool g_prng_seeded = false;

static unsigned long g_steps;

static jmp_buf g_jmp;
typedef enum { OUT_NONE, OUT_HALT, OUT_ERROR } Outcome;
static Outcome g_outcome;
static char g_errmsg[256];

static void vm_error(const char *msg) {
    snprintf(g_errmsg, sizeof(g_errmsg), "%s", msg);
    g_outcome = OUT_ERROR;
    longjmp(g_jmp, 1);
}

static void push(int64_t v) {
    if (g_sp == g_cap) {
        g_cap = g_cap ? g_cap * 2 : 256;
        g_stack = realloc(g_stack, g_cap * sizeof(int64_t));
    }
    g_stack[g_sp++] = v;
}

static int64_t pop(void) {
    if (g_sp == 0) vm_error("stack underflow");
    return g_stack[--g_sp];
}

static void mem_ensure(size_t addr) {
    if (addr < g_mem_cap) return;
    size_t new_cap = g_mem_cap ? g_mem_cap * 2 : 64;
    while (new_cap <= addr) new_cap *= 2;
    g_mem = realloc(g_mem, new_cap * sizeof(int64_t));
    if (!g_mem) { perror("realloc"); exit(1); }
    for (size_t i = g_mem_cap; i < new_cap; i++) g_mem[i] = 0;
    g_mem_cap = new_cap;
}

static int64_t mem_load(int64_t addr) {
    if (addr < 0) vm_error("invalid memory address");
    if ((size_t)addr >= g_mem_cap) return 0;
    return g_mem[(size_t)addr];
}

static void mem_store(int64_t addr, int64_t value) {
    if (addr < 0) vm_error("invalid memory address");
    mem_ensure((size_t)addr);
    g_mem[(size_t)addr] = value;
}

static void prng_seed(void) {
    if (g_prng_seeded) return;
    uint64_t seed = 0;
    bool got = false;
    FILE *f = fopen("/dev/urandom", "rb");
    if (f) {
        uint64_t tmp = 0;
        if (fread(&tmp, 1, sizeof(tmp), f) == sizeof(tmp)) {
            seed = tmp;
            got = true;
        }
        fclose(f);
    }
    if (!got) {
        struct timespec ts;
        if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
            seed ^= (uint64_t)ts.tv_sec * 1000000007ULL ^ (uint64_t)ts.tv_nsec * 1009ULL;
            got = true;
        } else {
            seed ^= (uint64_t)time(NULL) * 1000000007ULL;
        }
        seed ^= (uint64_t)getpid() * 0x9e3779b97f4a7c15ULL;
        seed ^= (uint64_t)(uintptr_t)&seed;
    }
    if (seed == 0) seed = 0x9e3779b97f4a7c15ULL;
    /* mix once to avoid zero-lock */
    seed ^= seed >> 12;
    seed ^= seed << 25;
    seed ^= seed >> 27;
    g_prng_state = seed;
    g_prng_seeded = true;
}

static uint64_t prng_next(void) {
    prng_seed();
    /* xorshift64* */
    uint64_t x = g_prng_state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    g_prng_state = x;
    return x * 0x2545F4914F6CDD1DULL;
}

static void exec_block(Block *blk, int depth);

static void exec_instr(Instr *ins, int depth) {
    g_steps++;
    if (g_steps > MAX_STEPS) vm_error("step limit exceeded (possible infinite loop)");

    switch (ins->op) {
        case OP_PUSH: push(ins->value); break;
        case OP_DUP:  { int64_t a = pop(); push(a); push(a); break; }
        case OP_SWAP: { int64_t b = pop(), a = pop(); push(b); push(a); break; }
        case OP_DROP: pop(); break;
        case OP_ROT:  { int64_t c = pop(), b = pop(), a = pop(); push(b); push(c); push(a); break; }
        case OP_NEG:  { int64_t a = pop(); push(-a); break; }
        case OP_SUB:  { int64_t b = pop(), a = pop(); push(a - b); break; }
        case OP_MOD:  { int64_t b = pop(), a = pop(); if (b == 0) vm_error("modulo by zero"); push(a % b); break; }
        case OP_ID:   break;
        case OP_ADD:  { int64_t b = pop(), a = pop(); push(a + b); break; }
        case OP_DIV:  { int64_t b = pop(), a = pop(); if (b == 0) vm_error("divide by zero"); push(a / b); break; }
        case OP_SIGN: { int64_t a = pop(); push(a > 0 ? 1 : a < 0 ? -1 : 0); break; }
        case OP_MUL:  { int64_t b = pop(), a = pop(); push(a * b); break; }
        case OP_POW:  {
            int64_t b = pop(), a = pop();
            if (b < 0) { push(0); break; }
            int64_t r = 1;
            for (int64_t k = 0; k < b; k++) r *= a;
            push(r);
            break;
        }
        case OP_NUMOUT: { int64_t a = pop(); printf("%lld", (long long)a); break; }
        case OP_CHAROUT:{ int64_t a = pop(); utf8_put(a); break; }
        case OP_CHARIN: fflush(stdout); push(utf8_get_stdin()); break;
        case OP_RAND: {
            int64_t n = pop();
            if (n <= 0) vm_error("invalid random range");
            uint64_t un = (uint64_t)n;
            uint64_t r = prng_next();
            /* unbiased via rejection */
            uint64_t limit = UINT64_MAX - (UINT64_MAX % un);
            while (r >= limit) r = prng_next();
            push((int64_t)(r % un));
            break;
        }
        case OP_STORE: {
            int64_t value = pop();
            int64_t addr = pop();
            mem_store(addr, value);
            break;
        }
        case OP_LOAD: {
            int64_t addr = pop();
            push(mem_load(addr));
            break;
        }
        case OP_SLEEP: {
            int64_t ms = pop();
            if (ms > 0) {
                fflush(stdout);
                struct timespec ts;
                ts.tv_sec = ms / 1000;
                ts.tv_nsec = (ms % 1000) * 1000000L;
                nanosleep(&ts, NULL);
            }
            break;
        }
        case OP_CALL: {
            int64_t n = pop();
            if (depth > MAX_DEPTH) vm_error("call depth exceeded (possible infinite recursion)");
            if (n < 0 || (size_t)n >= g_word_count) vm_error("call to undefined word");
            exec_block(g_words[(size_t)n], depth + 1);
            break;
        }
        case OP_IF: {
            int64_t c = pop();
            if (c) exec_block(ins->then_blk, depth);
            else if (ins->else_blk) exec_block(ins->else_blk, depth);
            break;
        }
        case OP_HALT:
            g_outcome = OUT_HALT;
            longjmp(g_jmp, 1);
    }
}

static void exec_block(Block *blk, int depth) {
    for (size_t i = 0; i < blk->count; i++) {
        exec_instr(&blk->items[i], depth);
    }
}

static const char *op_name(OpCode op) {
    switch (op) {
        case OP_PUSH: return "push";
        case OP_DUP: return "dup"; case OP_SWAP: return "swap";
        case OP_DROP: return "drop"; case OP_ROT: return "rot";
        case OP_NEG: return "neg"; case OP_SUB: return "sub"; case OP_MOD: return "mod";
        case OP_ID: return "id"; case OP_ADD: return "add"; case OP_DIV: return "div";
        case OP_SIGN: return "sign"; case OP_MUL: return "mul"; case OP_POW: return "pow";
        case OP_NUMOUT: return "num-out"; case OP_CHAROUT: return "char-out"; case OP_CHARIN: return "char-in";
        case OP_CALL: return "call"; case OP_IF: return "if"; case OP_HALT: return "halt";
        case OP_RAND: return "rand"; case OP_STORE: return "store"; case OP_LOAD: return "load"; case OP_SLEEP: return "sleep";
    }
    return "?";
}

static void disasm_block(Block *blk, int indent) {
    for (size_t i = 0; i < blk->count; i++) {
        Instr *ins = &blk->items[i];
        printf("%*s%s", indent, "", op_name(ins->op));
        if (ins->op == OP_PUSH) printf(" %lld", (long long)ins->value);
        printf("\n");
        if (ins->op == OP_IF) {
            printf("%*sthen:\n", indent + 2, "");
            disasm_block(ins->then_blk, indent + 4);
            if (ins->else_blk) {
                printf("%*selse:\n", indent + 2, "");
                disasm_block(ins->else_blk, indent + 4);
            }
        }
    }
}

static unsigned char *read_file(const char *path, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "%s: %s\n", path, strerror(errno)); exit(1); }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *buf = malloc((size_t)sz);
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) { fprintf(stderr, "%s: read error\n", path); exit(1); }
    fclose(f);
    *out_len = (size_t)sz;
    return buf;
}

int main(int argc, char **argv) {
    bool disassemble = false;
    const char *path = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0) disassemble = true;
        else path = argv[i];
    }
    if (!path) {
        fprintf(stderr, "usage: %s [-d] program.txt\n", argv[0]);
        return 2;
    }
    g_src_name = path;

    size_t raw_len;
    unsigned char *raw = read_file(path, &raw_len);

    size_t full_len;
    cp_t *full = utf8_decode(raw, raw_len, &full_len);
    free(raw);

    size_t prog_len;
    bool embedded;
    cp_t *prog = extract_program(full, full_len, &prog_len, &embedded);
    free(full);

    g_words = split_and_parse_words(prog, prog_len, &g_word_count);
    free(prog);

    if (g_word_count == 0 || prog_len == 0) {
        fprintf(stderr, "%s: this document does not contain Invisible Ink\n", path);
        return 1;
    }

    if (disassemble) {
        for (size_t w = 0; w < g_word_count; w++) {
            printf("word %zu:\n", w);
            disasm_block(g_words[w], 2);
        }
        return 0;
    }

    g_outcome = OUT_NONE;
    if (setjmp(g_jmp) == 0) {
        exec_block(g_words[0], 0);
        fflush(stdout);
        return 0;
    }

    fflush(stdout);
    if (g_outcome == OUT_HALT) {
        return 0;
    }
    fprintf(stderr, "%s: runtime error: %s\n", path, g_errmsg);
    return 1;
}

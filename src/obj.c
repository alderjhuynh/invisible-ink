/*
 * obj.c — the .inkb compiled Ink object format.
 *
 * Little-endian fixed-width encoding chosen on purpose: no varints, no
 * length prefixes to validate twice, every field readable with exactly two
 * helpers and a running cursor. Loaders validate bounds at every step.
 */
#include "obj.h"
#include "parse.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* ---------- little-endian helpers ---------- */

static void put_u64(uint8_t *p, uint64_t v) {
    for (int i = 0; i < 8; i++) p[i] = (uint8_t)(v >> (8 * i));
}
static void put_u32(uint8_t *p, uint32_t v) {
    for (int i = 0; i < 4; i++) p[i] = (uint8_t)(v >> (8 * i));
}
static void put_u16(uint8_t *p, uint16_t v) {
    for (int i = 0; i < 2; i++) p[i] = (uint8_t)(v >> (8 * i));
}
static uint64_t get_u64(const uint8_t *p) {
    uint64_t v = 0;
    for (int i = 7; i >= 0; i--) v = (v << 8) | p[i];
    return v;
}
static uint32_t get_u32(const uint8_t *p) {
    uint32_t v = 0;
    for (int i = 3; i >= 0; i--) v = (v << 8) | p[i];
    return v;
}
static uint16_t get_u16(const uint8_t *p) {
    return (uint16_t)(p[0] | (p[1] << 8));
}

/* ---------- dynamic byte buffer ---------- */

typedef struct { uint8_t *data; size_t len, cap; } OutBuf;

static bool ob_push(OutBuf *b, const void *src, size_t n) {
    if (b->len + n > b->cap) {
        size_t ncap = b->cap ? b->cap : 256;
        while (b->len + n > ncap) {
            if (ncap > (SIZE_MAX / 2)) return false;
            ncap *= 2;
        }
        uint8_t *nd = realloc(b->data, ncap);
        if (!nd) return false;
        b->data = nd;
        b->cap = ncap;
    }
    if (n) memcpy(b->data + b->len, src, n);
    b->len += n;
    return true;
}
static bool ob_u8(OutBuf *b, uint8_t v)  { return ob_push(b, &v, 1); }
static bool ob_u16(OutBuf *b, uint16_t v){ uint8_t t[2]; put_u16(t, v); return ob_push(b, t, 2); }
static bool ob_u32(OutBuf *b, uint32_t v){ uint8_t t[4]; put_u32(t, v); return ob_push(b, t, 4); }
static bool ob_u64(OutBuf *b, uint64_t v){ uint8_t t[8]; put_u64(t, v); return ob_push(b, t, 8); }
static bool ob_i64(OutBuf *b, int64_t v) { return ob_u64(b, (uint64_t)v); }

/* ---------- serialization ---------- */

static bool save_block(OutBuf *o, const Block *b) {
    if (b->count > 0xFFFFFFFFu) return false;
    if (!ob_u32(o, (uint32_t)b->count)) return false;
    for (size_t i = 0; i < b->count; i++) {
        const Instr *ins = &b->items[i];
        if (ins->op > OP_SYSCALL) return false;
        if (!ob_u8(o, (uint8_t)ins->op)) return false;
        if (!ob_i64(o, ins->value)) return false;
        if (ins->op == OP_IF || ins->op == OP_LOOP) {
            if (!ob_u8(o, ins->else_blk ? 1 : 0)) return false;
            if (!save_block(o, ins->then_blk)) return false;
            if (ins->else_blk && !save_block(o, ins->else_blk)) return false;
        }
    }
    return true;
}

bool inkobj_save(const Block *const *words, size_t word_count,
                 const ObjMemEntry *mem, size_t mem_count,
                 uint8_t **out_data, size_t *out_len,
                 char *err, size_t errsz) {
    OutBuf o = {0};
    if (word_count == 0) { snprintf(err, errsz, "refusing to save 0 words"); return false; }
    if (mem_count > 0 && !mem) { snprintf(err, errsz, "mem_count without entries"); return false; }
    if (!ob_push(&o, "INKB", 4) || !ob_u16(&o, INKOBJ_VERSION) ||
        !ob_u16(&o, INKOBJ_ISA) || !ob_u32(&o, 0) ||
        !ob_u64(&o, (uint64_t)word_count) || !ob_u64(&o, (uint64_t)mem_count)) {
        free(o.data); snprintf(err, errsz, "out of memory"); return false;
    }
    for (size_t i = 0; i < mem_count; i++) {
        if (!ob_i64(&o, mem[i].addr) || !ob_i64(&o, mem[i].value)) {
            free(o.data); snprintf(err, errsz, "out of memory"); return false;
        }
    }
    for (size_t w = 0; w < word_count; w++) {
        if (!save_block(&o, words[w])) {
            free(o.data); snprintf(err, errsz, "out of memory (word %zu)", w);
            return false;
        }
    }
    *out_data = o.data;
    *out_len = o.len;
    return true;
}

/* ---------- loading ---------- */

typedef struct { const uint8_t *p; size_t len, pos; } ObjR;

static bool r_bytes(ObjR *r, size_t n, const uint8_t **out, char *err, size_t errsz) {
    if (r->len - r->pos < n) { snprintf(err, errsz, "truncated object (need %zu bytes at offset %zu)", n, r->pos); return false; }
    *out = r->p + r->pos;
    r->pos += n;
    return true;
}
static bool r_u8(ObjR *r, uint8_t *v, char *err, size_t errsz) {
    const uint8_t *p; if (!r_bytes(r, 1, &p, err, errsz)) return false; *v = p[0]; return true;
}
static bool r_u32(ObjR *r, uint32_t *v, char *err, size_t errsz) {
    const uint8_t *p; if (!r_bytes(r, 4, &p, err, errsz)) return false; *v = get_u32(p); return true;
}
static bool r_u64(ObjR *r, uint64_t *v, char *err, size_t errsz) {
    const uint8_t *p; if (!r_bytes(r, 8, &p, err, errsz)) return false; *v = get_u64(p); return true;
}
static bool r_i64(ObjR *r, int64_t *v, char *err, size_t errsz) {
    uint64_t u; if (!r_u64(r, &u, err, errsz)) return false; *v = (int64_t)u; return true;
}

static Block *load_block(ObjR *r, int nest, char *err, size_t errsz) {
    if (nest > INK_MAX_BLOCK_NEST) { snprintf(err, errsz, "object block nesting too deep"); return NULL; }
    uint32_t count;
    if (!r_u32(r, &count, err, errsz)) return NULL;
    Block *blk = block_new();
    if (!blk) { snprintf(err, errsz, "out of memory"); return NULL; }
    for (uint32_t k = 0; k < count; k++) {
        uint8_t op;
        int64_t value;
        if (!r_u8(r, &op, err, errsz)) { block_free(blk); return NULL; }
        if (op > OP_SYSCALL) {
            snprintf(err, errsz, "invalid opcode %u in object", (unsigned)op);
            block_free(blk);
            return NULL;
        }
        if (!r_i64(r, &value, err, errsz)) { block_free(blk); return NULL; }
        if (op == OP_IF || op == OP_LOOP) {
            uint8_t has_else;
            if (!r_u8(r, &has_else, err, errsz)) { block_free(blk); return NULL; }
            Block *then_b = load_block(r, nest + 1, err, errsz);
            if (!then_b) { block_free(blk); return NULL; }
            Block *else_b = NULL;
            if (has_else) {
                else_b = load_block(r, nest + 1, err, errsz);
                if (!else_b) { block_free(then_b); block_free(blk); return NULL; }
            } else if (op == OP_LOOP) {
                snprintf(err, errsz, "while block missing loop body");
                block_free(then_b); block_free(blk);
                return NULL;
            }
            Instr *ins = block_push(blk, (OpCode)op);
            ins->value = value;
            ins->then_blk = then_b;
            ins->else_blk = else_b;
        } else {
            Instr *ins = block_push(blk, (OpCode)op);
            ins->value = value;
        }
    }
    return blk;
}

bool inkobj_sniff(const uint8_t *data, size_t len) {
    return len >= 28 &&
           data[0] == INKOBJ_MAGIC_0 && data[1] == INKOBJ_MAGIC_1 &&
           data[2] == INKOBJ_MAGIC_2 && data[3] == INKOBJ_MAGIC_3;
}

bool inkobj_load(const uint8_t *data, size_t len,
                 Block ***out_words, size_t *out_word_count,
                 ObjMemEntry **out_mem, size_t *out_mem_count,
                 char *err, size_t errsz) {
    ObjR r = { data, len, 0 };
    if (!inkobj_sniff(data, len)) { snprintf(err, errsz, "not an Ink object (bad magic)"); return false; }
    uint16_t ver, isa;
    uint32_t flags;
    const uint8_t *p;
    if (!r_bytes(&r, 12, &p, err, errsz)) return false;  /* magic+ver+isa+flags */
    ver = get_u16(p + 4); isa = get_u16(p + 6);
    flags = get_u32(p + 8);
    if (ver != INKOBJ_VERSION) { snprintf(err, errsz, "unsupported object version %u (want %u)", ver, INKOBJ_VERSION); return false; }
    if (isa != INKOBJ_ISA)     { snprintf(err, errsz, "object targets ISA v%u; this runtime is ISA v%u", isa, INKOBJ_ISA); return false; }
    if (flags != 0)            { snprintf(err, errsz, "object has unknown flag bits set (0x%x)", flags); return false; }
    uint64_t wc, mc;
    if (!r_u64(&r, &wc, err, errsz)) return false;
    if (!r_u64(&r, &mc, err, errsz)) return false;
    if (wc == 0) { snprintf(err, errsz, "object contains 0 words"); return false; }
    if (wc > SIZE_MAX / sizeof(Block *) || mc > SIZE_MAX / sizeof(ObjMemEntry)) { snprintf(err, errsz, "absurd object word/memory count"); return false; }
    if (mc > len) { snprintf(err, errsz, "absurd object memory count"); return false; }

    ObjMemEntry *mem = NULL;
    if (mc > 0) {
        mem = malloc((size_t)mc * sizeof(ObjMemEntry));
        if (!mem) { snprintf(err, errsz, "out of memory"); return false; }
        for (uint64_t i = 0; i < mc; i++) {
            if (!r_i64(&r, &mem[i].addr, err, errsz) || !r_i64(&r, &mem[i].value, err, errsz)) {
                free(mem);
                if (err[0] == 0) snprintf(err, errsz, "truncated memory image");
                return false;
            }
        }
    }

    Block **words = malloc((size_t)wc * sizeof(Block *));
    if (!words) { free(mem); snprintf(err, errsz, "out of memory"); return false; }
    for (uint64_t w = 0; w < wc; w++) {
        words[w] = load_block(&r, 0, err, errsz);
        if (!words[w]) {
            for (uint64_t k = 0; k < w; k++) block_free(words[k]);
            free(words);
            free(mem);
            return false;
        }
    }
    if (r.pos != r.len) {
        /* trailing junk is tolerated? No: be strict, the loader is in-kernel. */
        for (uint64_t k = 0; k < wc; k++) block_free(words[k]);
        free(words);
        free(mem);
        snprintf(err, errsz, "%zu trailing bytes after last word", r.len - r.pos);
        return false;
    }

    *out_words = words;
    *out_word_count = (size_t)wc;
    *out_mem = mem;
    *out_mem_count = (size_t)mc;
    return true;
}

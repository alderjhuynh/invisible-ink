#include "parse.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *g_src_name = "program";

char g_parse_err[256] = {0};
size_t g_parse_pos = 0;

/* Record the error and unwind: parse_block returns NULL up the chain.
   Historical CLI message shape ("name: parse error: msg (at codepoint N)")
   is reproduced by the caller. */
static bool parse_error(const char *msg, size_t pos) {
    size_t i = 0;
    while (msg[i] && i + 1 < sizeof(g_parse_err)) { g_parse_err[i] = msg[i]; i++; }
    g_parse_err[i] = 0;
    g_parse_pos = pos;
    return false;
}

Block *block_new(void) {
    Block *b = malloc(sizeof(Block));
    b->items = NULL; b->count = 0; b->cap = 0;
    return b;
}
void block_free(Block *b){
    if(!b) return;
    for(size_t i=0;i<b->count;i++) if(b->items[i].op==OP_IF || b->items[i].op==OP_LOOP){ block_free(b->items[i].then_blk); block_free(b->items[i].else_blk); }
    free(b->items); free(b);
}

Instr *block_push(Block *b, OpCode op) {
    if (b->count == b->cap) {
        b->cap = b->cap ? b->cap * 2 : 8;
        b->items = realloc(b->items, b->cap * sizeof(Instr));
    }
    Instr *ins = &b->items[b->count++];
    memset(ins, 0, sizeof(Instr));
    ins->op = op;
    return ins;
}

static OpCode arity_op(cp_t ch, int count) {
    if (ch == CP_EN)    return count == 1 ? OP_NEG  : count == 2 ? OP_SUB : OP_MOD;
    if (ch == CP_EM)    return count == 1 ? OP_ID   : count == 2 ? OP_ADD : OP_DIV;
           return count == 1 ? OP_SIGN : count == 2 ? OP_MUL : OP_POW;
}

Block *parse_block(const cp_t *cps, size_t len, size_t *pos, bool stop_at_zwj) {
    Block *blk = block_new();
    size_t i = *pos;

    while (i < len) {
        cp_t ch = cps[i];

        if (stop_at_zwj && ch == CP_ZWJ) { i++; *pos = i; return blk; }

        if (ch == CP_LS) { i++; continue; }

        if (ch == CP_ZWSP) { block_push(blk, OP_HALT); i++; continue; }

        if (ch == CP_HAIR) {
            i++;
            /* two's-complement 64-bit: literal wraps like every other ISA
               value; a 64+ bit literal keeps its low 64 bits */
            uint64_t bits = 0;
            size_t nbits = 0;
            while (i < len && (cps[i] == CP_DIG0 || cps[i] == CP_DIG1)) {
                bits = bits * 2 + (cps[i] == CP_DIG1 ? 1u : 0u);
                nbits++;
                i++;
            }
            if (nbits == 0) { parse_error("empty literal", i); block_free(blk); return NULL; }
            if (i >= len || (cps[i] != CP_LS && cps[i] != CP_SOFT)) {
                parse_error("unterminated literal (expected LINE SEPARATOR or SOFT HYPHEN)", i);
                block_free(blk);
                return NULL;
            }
            bool soft = (cps[i] == CP_SOFT);
            i++;
            Instr *ins = block_push(blk, OP_PUSH);
            ins->value = (int64_t)bits;
            if (soft) {
                block_push(blk, OP_CHAROUT);
            }
            continue;
        }

        if (ch == CP_WJ) {
            i++;
            if (i >= len || cps[i] != CP_ZWNJ) { parse_error("expected block open after IF", i); block_free(blk); return NULL; }
            i++;
            size_t p = i;
            Block *then_b = parse_block(cps, len, &p, true);
            if (!then_b) { block_free(blk); return NULL; }
            i = p;
            Block *else_b = NULL;
            if (i < len && cps[i] == CP_ZWNJ) {
                i++;
                p = i;
                else_b = parse_block(cps, len, &p, true);
                if (!else_b) { block_free(then_b); block_free(blk); return NULL; }
                i = p;
            }
            Instr *ins = block_push(blk, OP_IF);
            ins->then_blk = then_b;
            ins->else_blk = else_b;
            continue;
        }

        if (ch == CP_ALM) {
            i++;
            if (i >= len || cps[i] != CP_ZWNJ) { parse_error("expected condition block after WHILE", i); block_free(blk); return NULL; }
            i++;
            size_t p = i;
            Block *cond_b = parse_block(cps, len, &p, true);
            if (!cond_b) { block_free(blk); return NULL; }
            i = p;
            if (i >= len || cps[i] != CP_ZWNJ) {
                parse_error("expected loop body block after WHILE condition", i);
                block_free(cond_b); block_free(blk); return NULL;
            }
            i++;
            p = i;
            Block *body_b = parse_block(cps, len, &p, true);
            if (!body_b) { block_free(cond_b); block_free(blk); return NULL; }
            i = p;
            Instr *ins = block_push(blk, OP_LOOP);
            ins->then_blk = cond_b;   /* condition, re-evaluated every iteration */
            ins->else_blk = body_b;   /* body, runs while condition holds */
            continue;
        }

        if (ch == CP_EN || ch == CP_EM || ch == CP_THREE) {
            int count = 1;
            while (i + (size_t)count < len && cps[i + (size_t)count] == ch && count < 3) count++;
            OpCode op = arity_op(ch, count);
            if (op != OP_ID) block_push(blk, op);
            i += (size_t)count;
            continue;
        }

        if (ch == CP_SYS) { block_push(blk, OP_SYSCALL); i++; continue; }

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
            case CP_SOCK_CREATE: block_push(blk, OP_SOCK_CREATE); i++; break;
            case CP_LRE:    block_push(blk, OP_AND);     i++; break;
            case CP_RLE:    block_push(blk, OP_OR);      i++; break;
            case CP_PDF:    block_push(blk, OP_XOR);     i++; break;
            case CP_LRO:    block_push(blk, OP_SHL);     i++; break;
            case CP_RLO:    block_push(blk, OP_SHR);     i++; break;
            case CP_SOCK_BIND:   block_push(blk, OP_SOCK_BIND);   i++; break;
            case CP_SOCK_LISTEN: block_push(blk, OP_SOCK_LISTEN); i++; break;
            case CP_SOCK_ACCEPT: block_push(blk, OP_SOCK_ACCEPT); i++; break;
            case CP_SOCK_RECV:   block_push(blk, OP_SOCK_RECV);   i++; break;
            case CP_SOCK_SEND:   block_push(blk, OP_SOCK_SEND);   i++; break;
            case CP_SOCK_CLOSE:  block_push(blk, OP_SOCK_CLOSE);  i++; break;
            case CP_FILE_OPEN:   block_push(blk, OP_FILE_OPEN);   i++; break;
            case CP_FILE_READ:   block_push(blk, OP_FILE_READ);   i++; break;
            case CP_FILE_STAT:   block_push(blk, OP_FILE_STAT);   i++; break;
            case CP_DIR_LIST:    block_push(blk, OP_DIR_LIST);    i++; break;
            case CP_FILE_CREATE: block_push(blk, OP_FILE_CREATE); i++; break;
            case CP_FILE_WRITE:  block_push(blk, OP_FILE_WRITE);  i++; break;
            case CP_FILE_APPEND: block_push(blk, OP_FILE_APPEND); i++; break;
            case CP_SOFT:    i++; break;
            case CP_ZWJ:    i++; break;
            case CP_ZWNJ:   i++; break;
            case CP_BOM:    i++; break;
            default:        i++; break;
        }
    }
    *pos = i;
    return blk;
}

cp_t *extract_program(const cp_t *full, size_t full_len, size_t *out_len, bool *embedded) {
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

/* NULL on parse error; g_parse_err / g_parse_pos set. Caller frees nothing
   (partial words are freed here). */
Block **split_and_parse_words(const cp_t *cps, size_t len, size_t *out_word_count) {
    g_parse_err[0] = 0;
    g_parse_pos = 0;
    if (len == 0) { *out_word_count = 0; return malloc(sizeof(Block *)); }
    size_t word_count = 1;
    for (size_t i = 0; i < len; i++) if (cps[i] == CP_PS) word_count++;

    Block **words = malloc(word_count * sizeof(Block *));
    size_t w = 0, seg_start = 0;
    for (size_t i = 0; i <= len; i++) {
        if (i == len || cps[i] == CP_PS) {
            size_t seg_len = i - seg_start;
            size_t p = 0;
            Block *blk = parse_block(cps + seg_start, seg_len, &p, false);
            if (!blk) {
                for (size_t k = 0; k < w; k++) block_free(words[k]);
                free(words);
                *out_word_count = 0;
                return NULL;
            }
            words[w++] = blk;
            seg_start = i + 1;
        }
    }
    *out_word_count = w;
    return words;
}

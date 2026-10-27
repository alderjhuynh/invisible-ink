#include "progload.h"
#include "parse.h"
#include "vm.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static void set_err(InkProg *p, const char *msg) {
    size_t i = 0;
    while (msg[i] && i + 1 < sizeof(p->err)) { p->err[i] = msg[i]; i++; }
    p->err[i] = 0;
}

bool ink_prog_load_path(const char *path, InkProg *p) {
    memset(p, 0, sizeof(*p));
    size_t raw_len;
    unsigned char *raw = read_file_raw(path, &raw_len);
    if (!raw) { set_err(p, "cannot read file"); return false; }

    if (inkobj_sniff(raw, raw_len)) {
        p->kind = PROG_OBJ;
        bool ok = inkobj_load(raw, raw_len, &p->words, &p->word_count,
                              &p->mem, &p->mem_count, p->err, sizeof(p->err));
        free(raw);
        if (!ok) {
            set_err(p, p->err[0] ? p->err : "object load failed");
            return false;
        }
        return true;
    }

    size_t full_len;
    cp_t *full = utf8_decode(raw, raw_len, &full_len);
    free(raw);
    size_t prog_len;
    cp_t *prog = extract_program(full, full_len, &prog_len, &p->embedded);
    free(full);
    if (prog_len == 0) {
        free(prog);
        set_err(p, "this document does not contain Invisible Ink");
        return false;
    }
    p->kind = PROG_TEXT;
    p->cps = prog;
    p->cps_len = prog_len;
    return true;
}

bool ink_prog_into_vm(InkProg *p, InkVM *vm) {
    if (p->kind == PROG_TEXT) {
        vm_load_program(vm, p->cps, p->cps_len);
        p->cps = NULL;          /* vm_load_program owns (frees) the vector */
        p->cps_len = 0;
        if (vm->outcome == OUT_ERROR) {
            set_err(p, g_parse_err[0] ? g_parse_err : vm->errmsg);
            p->err_pos = g_parse_err[0] ? g_parse_pos : 0;
            return false;
        }
        return true;
    }
    /* PROG_OBJ */
    vm->words = p->words;
    vm->word_count = p->word_count;
    for (size_t i = 0; i < p->mem_count; i++)
        vm_mem_write(vm, p->mem[i].addr, p->mem[i].value);
    if (vm->outcome == OUT_ERROR) {
        /* a bad memory image (negative addr, past the limit) is a fault */
        set_err(p, vm->errmsg);
        vm->words = NULL;
        vm->word_count = 0;
        return false;
    }
    vm->outcome = OUT_NONE;
    p->words = NULL;            /* the VM owns the trees now */
    p->word_count = 0;
    return true;
}

void ink_prog_free(InkProg *p) {
    if (p->kind == PROG_TEXT) {
        free(p->cps);
        p->cps = NULL; p->cps_len = 0;
        return;
    }
    if (p->kind == PROG_OBJ) {
        if (p->words) {
            for (size_t i = 0; i < p->word_count; i++) block_free(p->words[i]);
            free(p->words);
            p->words = NULL; p->word_count = 0;
        }
        free(p->mem);
        p->mem = NULL; p->mem_count = 0;
    }
}

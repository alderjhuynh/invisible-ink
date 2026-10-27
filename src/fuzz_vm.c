/*
 * fuzz_vm.c — full execute fuzzer: parse + run on a sandboxed VM.
 *
 * The host-side sandbox limits make a runaway program merely useless instead
 * of OOM-long: step cap, memory cap, stack cap, depth cap, bounded I/O.
 * Note the ISA guarantee under test: any terminating behavior must be a
 * clean OUT_HALT / OUT_ERROR outcome, never a wedged interpreter.
 */
#include "ink.h"
#include "vm.h"
#include "parse.h"
#include "util.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define FUZZ_MAX_STEPS   200000UL
#define FUZZ_MEM_CELLS   (256 * 1024)   /* 2 MiB of tape */
#define FUZZ_STACK_CELLS (64 * 1024)
#define FUZZ_MAX_DEPTH   1000
#define FUZZ_MAX_OUT     (1 << 20)

static unsigned long f_out_count;

static void fz_char_out(InkVM *vm, int64_t cp) {
    (void)vm; (void)cp;
    if (f_out_count++ > FUZZ_MAX_OUT) exit(0);  /* never let output flood */
}
static int64_t fz_char_in(InkVM *vm) { (void)vm; return -1; }
static uint64_t fz_rand_seed(InkVM *vm) { (void)vm; return 0x9e3779b97f4a7c15ULL; }
static void fz_fatal(InkVM *vm, const char *what) {
    (void)vm; (void)what;
    exit(0);   /* oom/fatal ends the input, cleanly */
}
static void *fz_realloc(void *p, size_t n) {
    /* the core's calling convention uses realloc(ptr, 0) as free(); ASan's
       realloc(NULL, 0) allocates, so normalise that to a real free */
    if (n == 0) {
        if (p) free(p);
        return NULL;
    }
    void *q = realloc(p, n);
    if (!q) exit(0);   /* sandbox: treat oom as end of input */
    return q;
}
static int64_t fz_sys(InkVM *vm, int64_t num, int argc, const int64_t *args) {
    /* sandbox: everything host-side is unavailable; the core's 18/19 are
       already handled before this hook — anything else is "no" */
    (void)vm; (void)num; (void)argc; (void)args;
    return -1;
}
static const InkHost fz_host = {
    .char_out = fz_char_out, .char_in = fz_char_in,
    .rand_seed = fz_rand_seed, .fatal = fz_fatal, .realloc = fz_realloc,
    .suspend = NULL, .now_ms = NULL, .sys = fz_sys,
};

static int fuzz_one(const uint8_t *data, size_t size) {
    size_t n = size / 4;
    if (n == 0) return 0;
    cp_t *cps = malloc(n * sizeof(cp_t));
    if (!cps) return 0;
    for (size_t i = 0; i < n; i++)
        cps[i] = (cp_t)(data[4*i] | (data[4*i+1] << 8) | (data[4*i+2] << 16) | ((cp_t)data[4*i+3] << 24));

    InkVM vm;
    vm_init(&vm, &fz_host);
    vm.max_steps = FUZZ_MAX_STEPS;
    vm.mem_limit = FUZZ_MEM_CELLS;
    vm.stack_limit = FUZZ_STACK_CELLS;
    vm.max_depth = FUZZ_MAX_DEPTH;
    vm_load_program(&vm, cps, n);      /* takes ownership of cps */
    if (vm.outcome == OUT_NONE) {
        vm_run(&vm);
    }
    vm_destroy(&vm);
    return 0;
}

#ifdef LIBFUZZER
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    return fuzz_one(data, size);
}
#else
int main(int argc, char **argv) {
    for (int a = 1; a < argc; a++) {
        FILE *f = fopen(argv[a], "rb");
        if (!f) { perror(argv[a]); return 1; }
        static uint8_t buf[1 << 16];
        size_t got = fread(buf, 1, sizeof(buf), f);
        fclose(f);
        fuzz_one(buf, got);
    }
    printf("ok\n");
    return 0;
}
#endif

/*
 * fuzz_parse.c — parser fuzzer for the Invisible Ink front end.
 *
 * The interpreter is the kernel attack surface: every process is an
 * untrusted program fed straight into parse + exec. This harness feeds
 * arbitrary codepoint streams to the parser in standalone mode
 * (gcc + ASan/UBSan, walks a corpus) and under libFuzzer (clang).
 *
 *   clang:   make fuzz CC=clang CFLAGS="-std=c11 -g -O1 -fsanitize=fuzzer,address"
 *   gcc:     cc $(SPECIAL_FLAGS) -DFUZZ_STANDALONE fuzz_parse.c ... ; ./fuzz_parse file...
 */
#include "ink.h"
#include "vm.h"
#include "parse.h"
#include "util.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

static int fuzz_one(const uint8_t *data, size_t size) {
    /* every 4 bytes is one codepoint: the whole value space is reachable */
    size_t n = size / 4;
    cp_t *cps = malloc((n ? n : 1) * sizeof(cp_t));
    if (!cps) return 0;
    for (size_t i = 0; i < n; i++)
        cps[i] = (cp_t)(data[4*i] | (data[4*i+1] << 8) | (data[4*i+2] << 16) | ((cp_t)data[4*i+3] << 24));

    Block **words = split_and_parse_words(cps, n, &n);
    if (words) {
        for (size_t i = 0; i < n; i++) block_free(words[i]);
        free(words);
    }
    free(cps);
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

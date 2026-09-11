#include "vm.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>

unsigned long g_max_steps = DEFAULT_MAX_STEPS;
int g_max_depth = DEFAULT_MAX_DEPTH;

Block **g_words = NULL;
size_t g_word_count = 0;

int64_t *g_stack = NULL;
size_t g_sp = 0, g_cap = 0;

int64_t *g_mem = NULL;
size_t g_mem_cap = 0;

static uint64_t g_prng_state = 0;
static bool g_prng_seeded = false;

unsigned long g_steps = 0;

jmp_buf g_jmp;
Outcome g_outcome = OUT_NONE;
char g_errmsg[256] = {0};

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
    
    seed ^= seed >> 12;
    seed ^= seed << 25;
    seed ^= seed >> 27;
    g_prng_state = seed;
    g_prng_seeded = true;
}

static uint64_t prng_next(void) {
    prng_seed();
    
    uint64_t x = g_prng_state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    g_prng_state = x;
    return x * 0x2545F4914F6CDD1DULL;
}

static void exec_instr(Instr *ins, int depth);

void exec_block(Block *blk, int depth) {
    for (size_t i = 0; i < blk->count; i++) {
        exec_instr(&blk->items[i], depth);
    }
}

static void exec_instr(Instr *ins, int depth) {
    g_steps++;
    if (g_steps > g_max_steps) vm_error("step limit exceeded (possible infinite loop)");

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
            if (depth > g_max_depth) vm_error("call depth exceeded (possible infinite recursion)");
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

const char *op_name(OpCode op) {
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

void vm_reset(void) {
    g_sp = 0;
    g_mem_cap = 0;
    free(g_stack); g_stack=NULL; g_cap=0;
    free(g_mem); g_mem=NULL;
    g_prng_seeded = false;
}

void vm_reset_prng(void) {
    g_prng_seeded = false;
}

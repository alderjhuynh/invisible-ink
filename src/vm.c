#include "vm.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <dirent.h>

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

static char *mem_to_cstr(int64_t addr, int64_t len, bool *ok) {
    if (len < 0) { if (ok) *ok = false; return NULL; }
    if (addr < 0) { if (ok) *ok = false; return NULL; }
    char *s = malloc((size_t)len + 1);
    if (!s) { perror("malloc"); exit(1); }
    for (int64_t i = 0; i < len; i++) {
        int64_t v = mem_load(addr + i);
        if (v < 0 || v > 255) v = 0xFFFD; /* allow ascii only */
        s[i] = (char)(v & 0xFF);
    }
    s[len] = '\0';
    if (ok) *ok = true;
    return s;
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
    if (g_max_steps != 0 && g_steps > g_max_steps) vm_error("step limit exceeded (possible infinite loop)");

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
        case OP_AND: {
            int64_t b = pop(), a = pop();
            push((int64_t)((uint64_t)a & (uint64_t)b));
            break;
        }
        case OP_OR: {
            int64_t b = pop(), a = pop();
            push((int64_t)((uint64_t)a | (uint64_t)b));
            break;
        }
        case OP_XOR: {
            int64_t b = pop(), a = pop();
            push((int64_t)((uint64_t)a ^ (uint64_t)b));
            break;
        }
        case OP_SHL: {
            int64_t b = pop(), a = pop();
            push((int64_t)((uint64_t)a << (uint64_t)(b & 63)));
            break;
        }
        case OP_SHR: {
            int64_t b = pop(), a = pop();
            push((int64_t)((uint64_t)a >> (uint64_t)(b & 63)));
            break;
        }
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
        case OP_SOCK_CREATE: {
            int fd = socket(AF_INET, SOCK_STREAM, 0);
            if (fd < 0) push(-1);
            else {
                int opt = 1;
                setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
                push((int64_t)fd);
            }
            break;
        }
        case OP_SOCK_BIND: {
            int64_t port = pop();
            int64_t fd = pop();
            if (fd < 0 || port < 0 || port > 65535) { push(-1); break; }
            struct sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = htonl(INADDR_ANY);
            addr.sin_port = htons((uint16_t)port);
            int rc = bind((int)fd, (struct sockaddr*)&addr, sizeof(addr));
            push(rc == 0 ? 0 : -1);
            break;
        }
        case OP_SOCK_LISTEN: {
            int64_t backlog = pop();
            int64_t fd = pop();
            if (fd < 0 || backlog < 0) { push(-1); break; }
            if (backlog > 128) backlog = 128;
            if (backlog < 1) backlog = 5;
            int rc = listen((int)fd, (int)backlog);
            push(rc == 0 ? 0 : -1);
            break;
        }
        case OP_SOCK_ACCEPT: {
            int64_t fd = pop();
            if (fd < 0) { push(-1); break; }
            fflush(stdout);
            int cfd = accept((int)fd, NULL, NULL);
            if (cfd < 0) push(-1);
            else push((int64_t)cfd);
            break;
        }
        case OP_SOCK_RECV: {
            int64_t len = pop();
            int64_t buf = pop();
            int64_t fd = pop();
            if (fd < 0 || buf < 0 || len < 0) { push(-1); break; }
            if (len == 0) { push(0); break; }
            if (len > 8192) len = 8192;
            char *tmp = malloc((size_t)len);
            if (!tmp) { perror("malloc"); exit(1); }
            fflush(stdout);
            ssize_t n = recv((int)fd, tmp, (size_t)len, 0);
            if (n < 0) { free(tmp); push(-1); break; }
            for (ssize_t i = 0; i < n; i++) mem_store(buf + i, (int64_t)(unsigned char)tmp[i]);
            free(tmp);
            push((int64_t)n);
            break;
        }
        case OP_SOCK_SEND: {
            int64_t len = pop();
            int64_t buf = pop();
            int64_t fd = pop();
            if (fd < 0 || buf < 0 || len < 0) { push(-1); break; }
            if (len == 0) { push(0); break; }
            if (len > 8192) len = 8192;
            char *tmp = malloc((size_t)len);
            if (!tmp) { perror("malloc"); exit(1); }
            for (int64_t i = 0; i < len; i++) {
                int64_t v = mem_load(buf + i);
                tmp[i] = (char)(v & 0xFF);
            }
            /* A client that disconnects mid-response must not kill the
               server with SIGPIPE; report a short write instead. */
            void (*prev_sigpipe)(int) = signal(SIGPIPE, SIG_IGN);
            ssize_t total = 0;
            while (total < len) {
                ssize_t sent = send((int)fd, tmp + total, (size_t)(len - total), 0);
                if (sent < 0) {
                    if (errno == EINTR) continue;
                    break;
                }
                if (sent == 0) break;
                total += sent;
            }
            signal(SIGPIPE, prev_sigpipe);
            free(tmp);
            push((int64_t)total);
            break;
        }
        case OP_SOCK_CLOSE: {
            int64_t fd = pop();
            if (fd < 0) { push(-1); break; }
            int rc = close((int)fd);
            push(rc == 0 ? 0 : -1);
            break;
        }
        case OP_FILE_OPEN: {
            int64_t flags = pop();
            int64_t path_len = pop();
            int64_t path_ptr = pop();
            (void)flags;
            if (path_ptr < 0 || path_len < 0 || path_len > 4096) { push(-1); break; }
            bool ok;
            char *path = mem_to_cstr(path_ptr, path_len, &ok);
            if (!ok || !path) { push(-1); break; }
            int fd = open(path, O_RDONLY);
            free(path);
            if (fd < 0) push(-1);
            else push((int64_t)fd);
            break;
        }
        case OP_FILE_READ: {
            int64_t len = pop();
            int64_t buf = pop();
            int64_t fd = pop();
            if (fd < 0 || buf < 0 || len < 0) { push(-1); break; }
            if (len == 0) { push(0); break; }
            if (len > 8192) len = 8192;
            char *tmp = malloc((size_t)len);
            if (!tmp) { perror("malloc"); exit(1); }
            ssize_t n = read((int)fd, tmp, (size_t)len);
            if (n < 0) { free(tmp); push(-1); break; }
            for (ssize_t i = 0; i < n; i++) mem_store(buf + i, (int64_t)(unsigned char)tmp[i]);
            free(tmp);
            push((int64_t)n);
            break;
        }
        case OP_FILE_STAT: {
            int64_t path_len = pop();
            int64_t path_ptr = pop();
            if (path_ptr < 0 || path_len < 0 || path_len > 4096) { push(-1); break; }
            bool ok;
            char *path = mem_to_cstr(path_ptr, path_len, &ok);
            if (!ok || !path) { push(-1); break; }
            struct stat st;
            int rc = stat(path, &st);
            free(path);
            if (rc != 0) push(-1);
            else if (S_ISDIR(st.st_mode)) push(-2);
            else push((int64_t)st.st_size);
            break;
        }
        case OP_DIR_LIST: {
            int64_t out_cap = pop();
            int64_t out_buf = pop();
            int64_t path_len = pop();
            int64_t path_ptr = pop();
            if (path_ptr < 0 || path_len < 0 || path_len > 4096 || out_buf < 0 || out_cap < 0) { push(-1); break; }
            if (out_cap == 0) { push(0); break; }
            bool ok;
            char *path = mem_to_cstr(path_ptr, path_len, &ok);
            if (!ok || !path) { push(-1); break; }
            DIR *d = opendir(path[0] ? path : ".");
            if (!d) { free(path); push(-1); break; }
            /* Collect entries first so the listing matches
               `python3 -m http.server` (sorted case-insensitively). */
            char **names = NULL;
            bool *isdir = NULL;
            size_t nentries = 0, entries_cap = 0;
            struct dirent *ent;
            // need original path string for stat check
            size_t orig_len = strlen(path);
            while ((ent = readdir(d)) != NULL) {
                const char *name = ent->d_name;
                if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;
                // check if entry is directory via stat on full path
                bool is_dir = false;
                {
                    size_t nl0 = strlen(name);
                    size_t full_len = orig_len + 1 + nl0;
                    char *full = malloc(full_len + 1);
                    if (full) {
                        if (orig_len == 0 || (orig_len==1 && path[0]=='.')) {
                            memcpy(full, name, nl0);
                            full[nl0]=0;
                        } else {
                            memcpy(full, path, orig_len);
                            full[orig_len]='/';
                            memcpy(full+orig_len+1, name, nl0);
                            full[full_len]=0;
                        }
                        struct stat st2;
                        if (stat(full, &st2)==0 && S_ISDIR(st2.st_mode)) is_dir = true;
                        free(full);
                    }
                }
                if (nentries == entries_cap) {
                    entries_cap = entries_cap ? entries_cap * 2 : 64;
                    names = realloc(names, entries_cap * sizeof(char *));
                    isdir = realloc(isdir, entries_cap * sizeof(bool));
                    if (!names || !isdir) { perror("realloc"); exit(1); }
                }
                names[nentries] = strdup(name);
                isdir[nentries] = is_dir;
                nentries++;
            }
            closedir(d);
            /* insertion sort by case-insensitive name (python sorts with
               key=str.lower); n is small so O(n^2) is fine. */
            for (size_t a = 1; a < nentries; a++) {
                char *tn = names[a];
                bool td = isdir[a];
                size_t b = a;
                while (b > 0 && strcasecmp(names[b - 1], tn) > 0) {
                    names[b] = names[b - 1];
                    isdir[b] = isdir[b - 1];
                    b--;
                }
                names[b] = tn;
                isdir[b] = td;
            }
            int64_t written = 0;
            for (size_t e = 0; e < nentries; e++) {
                size_t nl = strlen(names[e]);
                size_t need = nl + (isdir[e] ? 1 : 0) + (written ? 1 : 0);
                if (written + (int64_t)need > out_cap) break;
                if (written) { mem_store(out_buf + written, (int64_t)'\n'); written++; }
                for (size_t i = 0; i < nl; i++) { mem_store(out_buf + written, (int64_t)(unsigned char)names[e][i]); written++; }
                if (isdir[e]) { mem_store(out_buf + written, (int64_t)'/'); written++; }
            }
            for (size_t e = 0; e < nentries; e++) free(names[e]);
            free(names);
            free(isdir);
            free(path);
            push(written);
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
        case OP_LOOP: {
            /* cond block must leave one value; falsy exits.
               Steps are still counted per-instruction inside exec_block,
               so the step limit bounds runaway loops. */
            for (;;) {
                exec_block(ins->then_blk, depth);
                int64_t c = pop();
                if (!c) break;
                exec_block(ins->else_blk, depth);
            }
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
        case OP_AND: return "and"; case OP_OR: return "or"; case OP_XOR: return "xor";
        case OP_SHL: return "shl"; case OP_SHR: return "shr";
        case OP_NUMOUT: return "num-out"; case OP_CHAROUT: return "char-out"; case OP_CHARIN: return "char-in";
        case OP_CALL: return "call"; case OP_IF: return "if"; case OP_LOOP: return "while"; case OP_HALT: return "halt";
        case OP_RAND: return "rand"; case OP_STORE: return "store"; case OP_LOAD: return "load"; case OP_SLEEP: return "sleep";
        case OP_SOCK_CREATE: return "sock-create";
        case OP_SOCK_BIND: return "sock-bind";
        case OP_SOCK_LISTEN: return "sock-listen";
        case OP_SOCK_ACCEPT: return "sock-accept";
        case OP_SOCK_RECV: return "sock-recv";
        case OP_SOCK_SEND: return "sock-send";
        case OP_SOCK_CLOSE: return "sock-close";
        case OP_FILE_OPEN: return "file-open";
        case OP_FILE_READ: return "file-read";
        case OP_FILE_STAT: return "file-stat";
        case OP_DIR_LIST: return "dir-list";
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

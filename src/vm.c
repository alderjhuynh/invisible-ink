/*
 * vm.c — the Invisible Ink core.
 *
 * Freestanding: no stdio, no libc. All VM state is per-instance (see ink.h)
 * and all environment interaction (I/O, allocator, syscalls, scheduling)
 * goes through vm->host. A process is one InkVM; the kernel of inkOS embeds
 * this file and one bare-metal InkHost.
 *
 * History: it also used to contain every legacy syscall impl; those now live
 * in the host layer (vmhost.c) behind SYS_* numbers — the legacy sock, file
 * and sleep codepoints compile down to them.
 */
#include "vm.h"
#include "parse.h"

/* ---------- tiny stdlib-free helpers ---------- */

static void xstr_copy(char *dst, size_t cap, const char *src) {
    size_t i = 0;
    while (src[i] && i + 1 < cap) { dst[i] = src[i]; i++; }
    dst[i] = 0;
}

/* fatal: allocator failure inside the core — the embedder decides what dying
   means (desktop: message + exit(1); kernel: panic, but never silently). */
static void oom(InkVM *vm, const char *what) {
    /* best effort message, no snprintf in the core */
    xstr_copy(vm->errmsg, sizeof(vm->errmsg), "out of memory");
    (void)what;
    if (vm->host->fatal) vm->host->fatal(vm, what);
    for (;;) { /* no host fatal: cannot proceed, trap */
        /* bare metal would spin into its fault handler here */
    }
}

/* ---------- outcomes ---------- */

void vm_suspend(InkVM *vm, Outcome reason) {
    vm->outcome = reason;
    if (!vm->host->suspend || !vm->allow_yield) {
        /* refusing to abandon state we could not save: treat as a fault */
        if (vm->allow_yield)
            xstr_copy(vm->errmsg, sizeof(vm->errmsg), "yield with no scheduler");
        else
            xstr_copy(vm->errmsg, sizeof(vm->errmsg), "yield not allowed");
        vm->outcome = OUT_ERROR;
        longjmp(vm->jmp, 1);
    }
    vm->host->suspend(vm, reason);
    /* resumed: execution continues exactly here */
    vm->outcome = OUT_NONE;
    vm->wait.kind = INK_WAIT_NONE;
}

void vm_wait_fd(InkVM *vm, int fd, bool writable) {
    if (!vm->host->suspend || !vm->allow_yield) return; /* host blocks itself */
    vm->wait.kind = writable ? INK_WAIT_FD_OUT : INK_WAIT_FD_IN;
    vm->wait.fd = fd;
    vm->wait.deadline_ms = 0;
    vm_suspend(vm, OUT_WAIT);
}

void vm_wait_deadline(InkVM *vm, int64_t deadline_ms) {
    if (!vm->host->suspend || !vm->allow_yield) return;
    vm->wait.kind = INK_WAIT_DEADLINE;
    vm->wait.fd = -1;
    vm->wait.deadline_ms = deadline_ms;
    vm_suspend(vm, OUT_WAIT);
}

static void vm_error(InkVM *vm, const char *msg) {
    size_t i = 0;
    while (msg[i] && i < sizeof(vm->errmsg) - 1) { vm->errmsg[i] = msg[i]; i++; }
    vm->errmsg[i] = 0;
    vm->outcome = OUT_ERROR;
    longjmp(vm->jmp, 1);
}

/* ---------- stack ---------- */

/*
 * ISA arithmetic is defined, not UB: every op wraps like two's-complement
 * 64-bit hardware. add/sub/mul/neg do their math in uint64_t and cast back;
 * INT64_MIN / -1 is INT64_MIN and INT64_MIN % -1 is 0 (the C operators are
 * UB there, so they are handled before division). pow likewise wraps and
 * bills one step per multiply so a huge exponent hits the step limit
 * instead of hanging.
 */
static void push(InkVM *vm, int64_t v) {
    /* stack_limit counts *cells in use*, not capacity: a program with a
       16-cell allotment fails on the 17th push even if capacity allows */
    if (vm->stack_limit && (size_t)vm->sp >= vm->stack_limit) {
        vm_error(vm, "stack limit exceeded (guard against runaway growth)");
    }
    if (vm->sp == vm->cap) {
        /* cap doubling must not overflow: a stack_limit of 0 (unlimited) is
           still bounded by addressable memory */
        if (vm->cap > (SIZE_MAX / sizeof(int64_t)) / 2) oom(vm, "stack");
        size_t ncap = vm->cap ? vm->cap * 2 : 256;
        /* keep within the process allotment */
        if (vm->stack_limit && ncap > vm->stack_limit) ncap = vm->stack_limit;
        int64_t *ns = vm->host->realloc(vm->stack, ncap * sizeof(int64_t));
        if (!ns) oom(vm, "stack");
        vm->stack = ns;
        vm->cap = ncap;
    }
    vm->stack[vm->sp++] = v;
}

int64_t vm_pop(InkVM *vm) {
    if (vm->sp == 0) vm_error(vm, "stack underflow");
    return vm->stack[--vm->sp];
}

void vm_push(InkVM *vm, int64_t v) { push(vm, v); }

/* ---------- memory tape ---------- */

static void mem_ensure(InkVM *vm, size_t addr) {
    if (addr < vm->mem_cap) return;
    if (vm->mem_limit && addr >= vm->mem_limit)
        vm_error(vm, "memory limit exceeded (store past the process allotment)");
    /* the tape is a flat int64 array: refuse addresses it could never back
       instead of looping forever as the doubling wraps */
    if (addr > (SIZE_MAX - 64) / sizeof(int64_t))
        vm_error(vm, "address too large for memory tape");
    size_t new_cap = vm->mem_cap ? vm->mem_cap * 2 : 64;
    while (new_cap <= addr) {
        if (new_cap > (SIZE_MAX / sizeof(int64_t)) / 2)
            vm_error(vm, "address too large for memory tape");
        new_cap *= 2;
    }
    if (vm->mem_limit && new_cap > vm->mem_limit) new_cap = vm->mem_limit;
    int64_t *nm = vm->host->realloc(vm->mem, new_cap * sizeof(int64_t));
    if (!nm) oom(vm, "memory");
    vm->mem = nm;
    for (size_t i = vm->mem_cap; i < new_cap; i++) vm->mem[i] = 0;
    vm->mem_cap = new_cap;
}

static int64_t mem_load(InkVM *vm, int64_t addr) {
    if (addr < 0) vm_error(vm, "invalid memory address");
    if ((size_t)addr >= vm->mem_cap) return 0;
    return vm->mem[(size_t)addr];
}

static void mem_store(InkVM *vm, int64_t addr, int64_t value) {
    if (addr < 0) vm_error(vm, "invalid memory address");
    mem_ensure(vm, (size_t)addr);
    vm->mem[(size_t)addr] = value;
}

int64_t vm_mem_read(InkVM *vm, int64_t addr) { return mem_load(vm, addr); }
void vm_mem_write(InkVM *vm, int64_t addr, int64_t v) { mem_store(vm, addr, v); }

char *vm_mem_cstr(InkVM *vm, int64_t addr, int64_t len, bool *ok) {
    if (len < 0 || addr < 0) { if (ok) *ok = false; return NULL; }
    char *s = vm->host->realloc(NULL, (size_t)len + 1);
    if (!s) oom(vm, "host string buffer");
    for (int64_t i = 0; i < len; i++) {
        int64_t v = mem_load(vm, addr + i);   /* may fault on limits — good */
        if (v < 0 || v > 255) v = 0xFFFD;     /* allow ascii only */
        s[i] = (char)(v & 0xFF);
    }
    s[len] = '\0';
    if (ok) *ok = true;
    return s;
}

/* ---------- prng (same spec as always, seed from host) ---------- */

static void prng_seed(InkVM *vm) {
    if (vm->prng_seeded) return;
    uint64_t seed = vm->host->rand_seed(vm);
    if (seed == 0) seed = 0x9e3779b97f4a7c15ULL;
    seed ^= seed >> 12;
    seed ^= seed << 25;
    seed ^= seed >> 27;
    vm->prng_state = seed;
    vm->prng_seeded = true;
}

static uint64_t prng_next(InkVM *vm) {
    prng_seed(vm);
    uint64_t x = vm->prng_state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    vm->prng_state = x;
    return x * 0x2545F4914F6CDD1DULL;
}

/* ---------- decimal num-out (pure, no printf) ---------- */

static void num_out(InkVM *vm, int64_t v) {
    unsigned char buf[32];
    size_t n = 0;
    uint64_t uv;
    if (v < 0) {
        vm->host->char_out(vm, '-');
        uv = (uint64_t)(-(v + 1)) + 1; /* INT64_MIN-safe negate */
    } else {
        uv = (uint64_t)v;
    }
    do {
        buf[n++] = (unsigned char)('0' + (int)(uv % 10));
        uv /= 10;
    } while (uv);
    while (n) vm->host->char_out(vm, buf[--n]);
}

/* ---------- syscall plumbing ---------- */

static const uint8_t sys_argc_table[] = {
    /* 1 */ 0, /* sock-create */
    /* 2 */ 2, /* sock-bind */
    /* 3 */ 2, /* sock-listen */
    /* 4 */ 1, /* sock-accept */
    /* 5 */ 3, /* sock-recv */
    /* 6 */ 3, /* sock-send */
    /* 7 */ 1, /* sock-close */
    /* 8 */ 3, /* file-open */
    /* 9 */ 3, /* file-read */
    /*10 */ 2, /* file-stat */
    /*11 */ 4, /* dir-list */
    /*12 */ 2, /* file-create */
    /*13 */ 3, /* file-write */
    /*14 */ 2, /* file-append */
    /*15 */ 1, /* sleep */
    /*16 */ 0, /* yield */
    /*17 */ 1, /* exit */
    /*18 */ 3, /* mem-set  (core ABI: in vm.c, not the host) */
    /*19 */ 3, /* mem-copy (core ABI: in vm.c, not the host) */
};

static int sys_argc(int64_t num) {
    if (num < 1 || num > (int64_t)(sizeof(sys_argc_table)))
        return -1; /* not a core-ABI syscall; hosts own it */
    return sys_argc_table[num - 1];
}

/* ---------- core-ABI memory facilities (18/19), host-independent ---------- */

/* (addr, fill, len) -> 0. Fills len cells beginning at addr with value.
   Respects the tape's own rules (limits, growth, negative-address faults).
   Returns 0; range/limit faults surface as VM errors like store would. */
static int64_t core_memset(InkVM *vm, int64_t addr, int64_t fill, int64_t len) {
    if (addr < 0 || len < 0) vm_error(vm, "mem-set: negative address or length");
    if (len == 0) return 0;
    if (addr > INT64_MAX - len) vm_error(vm, "mem-set: range overflow");
    for (int64_t i = 0; i < len; i++)
        vm_mem_write(vm, addr + i, fill);
    return 0;
}

/* (dst, src, len) -> 0. Copies len cells src[0..len) -> dst[0..len) with
   memmove semantics (overlap-safe: source is snapshotted first). */
static int64_t core_memcopy(InkVM *vm, int64_t dst, int64_t src, int64_t len) {
    if (dst < 0 || src < 0 || len < 0) vm_error(vm, "mem-copy: negative address or length");
    if (len == 0) return 0;
    if (dst > INT64_MAX - len || src > INT64_MAX - len)
        vm_error(vm, "mem-copy: range overflow");
    int64_t *tmp = vm->host->realloc(NULL, (size_t)len * sizeof(int64_t));
    if (!tmp) oom(vm, "mem-copy buffer");
    for (int64_t i = 0; i < len; i++) tmp[i] = vm_mem_read(vm, src + i);
    for (int64_t i = 0; i < len; i++) vm_mem_write(vm, dst + i, tmp[i]);
    vm->host->realloc(tmp, 0);
    return 0;
}

/* exported for hosts that want the core facilities without going through
   do_syscall (e.g. a kernel routing its own prefix first) */
int64_t vm_sys_core(InkVM *vm, int64_t num, int argc, const int64_t *args) {
    int64_t a0 = argc > 0 ? args[0] : 0;
    int64_t a1 = argc > 1 ? args[1] : 0;
    int64_t a2 = argc > 2 ? args[2] : 0;
    if (num == SYS_MEMSET)  return core_memset(vm, a0, a1, a2);
    if (num == SYS_MEMCOPY) return core_memcopy(vm, a0, a1, a2);
    return -1;
}

/*
 * Drivers: the legacy codepoints (sock, file, sleep) compile down to the
 * same syscall numbers, so the host implements each facility once and both
 * spellings get it.
 */
static int64_t do_syscall(InkVM *vm, int64_t num, int argc, int64_t *args);
static int64_t do_syscall(InkVM *vm, int64_t num, int argc, int64_t *args) {
    if (num == SYS_EXIT) {
        vm->exit_code = argc >= 1 ? args[0] : 0;
        vm->outcome = OUT_HALT;
        longjmp(vm->jmp, 1);
    }
    if (num == SYS_YIELD) {
        if (vm->allow_yield && vm->host->suspend) vm_suspend(vm, OUT_YIELD);
        return 0;
    }
    if (num == SYS_MEMSET || num == SYS_MEMCOPY) {
        /* core-ABI facility (18/19): lives in the interpreter core, so every
           host gets mem-set/mem-copy even before its own kernel grows any */
        return vm_sys_core(vm, num, argc, args);
    }
    return vm->host->sys(vm, num, argc, args);
}

/* legacy-codepoint driver: pop args per ABI, run, push result */
static void op_sys(InkVM *vm, int64_t num) {
    int argc = sys_argc(num);
    if (argc < 0) vm_error(vm, "unknown syscall number");
    int64_t args[INK_MAX_SYS_ARGS];
    for (int i = argc; i-- > 0;) args[i] = vm_pop(vm);   /* natural order out */
    push(vm, do_syscall(vm, num, argc, args));
}

/* ---------- execution ---------- */


static void exec_block(InkVM *vm, Block *blk, int depth, int nest);

static void exec_instr(InkVM *vm, Instr *ins, int depth, int nest) {
    vm->steps++;

    /* scheduling: quantum expiry is a yield, never a global halt */
    if (vm->allow_yield && vm->steps >= vm->quantum_end) {
        vm_suspend(vm, OUT_YIELD);
    }
    if (vm->max_steps != 0 && vm->steps > vm->max_steps) {
        vm_error(vm, "step limit exceeded (possible infinite loop)");
    }

    switch (ins->op) {
        case OP_PUSH: push(vm, ins->value); break;
        case OP_DUP:  { int64_t a = vm_pop(vm); push(vm, a); push(vm, a); break; }
        case OP_SWAP: { int64_t b = vm_pop(vm), a = vm_pop(vm); push(vm, b); push(vm, a); break; }
        case OP_DROP: vm_pop(vm); break;
        case OP_ROT:  { int64_t c = vm_pop(vm), b = vm_pop(vm), a = vm_pop(vm); push(vm, b); push(vm, c); push(vm, a); break; }
        case OP_NEG:  { int64_t a = vm_pop(vm); push(vm, (int64_t)(0 - (uint64_t)a)); break; }
        case OP_SUB:  { int64_t b = vm_pop(vm), a = vm_pop(vm); push(vm, (int64_t)((uint64_t)a - (uint64_t)b)); break; }
        case OP_MOD:  {
            int64_t b = vm_pop(vm), a = vm_pop(vm);
            if (b == 0) vm_error(vm, "modulo by zero");
            if (a == INT64_MIN && b == -1) { push(vm, 0); break; }  /* C op is UB */
            push(vm, a % b);
            break;
        }
        case OP_ID:   break;
        case OP_ADD:  { int64_t b = vm_pop(vm), a = vm_pop(vm); push(vm, (int64_t)((uint64_t)a + (uint64_t)b)); break; }
        case OP_DIV:  {
            int64_t b = vm_pop(vm), a = vm_pop(vm);
            if (b == 0) vm_error(vm, "divide by zero");
            if (a == INT64_MIN && b == -1) { push(vm, INT64_MIN); break; } /* C op is UB */
            push(vm, a / b);
            break;
        }
        case OP_SIGN: { int64_t a = vm_pop(vm); push(vm, a > 0 ? 1 : a < 0 ? -1 : 0); break; }
        case OP_MUL:  { int64_t b = vm_pop(vm), a = vm_pop(vm); push(vm, (int64_t)((uint64_t)a * (uint64_t)b)); break; }
        case OP_POW:  {
            int64_t b = vm_pop(vm), a = vm_pop(vm);
            if (b < 0) { push(vm, 0); break; }
            uint64_t r = 1;
            /* each multiply is a billed step; the quantum/limit checks get
               to suspend/error between iterations so `pow` cannot hide an
               unbounded loop from the scheduler */
            for (int64_t k = 0; k < b; k++) {
                vm->steps++;
                if (vm->allow_yield && vm->steps >= vm->quantum_end)
                    vm_suspend(vm, OUT_YIELD);   /* transparent resume */
                if (vm->max_steps != 0 && vm->steps > vm->max_steps)
                    vm_error(vm, "step limit exceeded (possible infinite loop)");
                r *= (uint64_t)a;
            }
            push(vm, (int64_t)r);
            break;
        }
        case OP_NUMOUT: num_out(vm, vm_pop(vm)); break;
        case OP_CHAROUT: vm->host->char_out(vm, vm_pop(vm)); break;
        case OP_CHARIN: push(vm, vm->host->char_in(vm)); break;
        case OP_AND:  { int64_t b = vm_pop(vm), a = vm_pop(vm); push(vm, (int64_t)((uint64_t)a & (uint64_t)b)); break; }
        case OP_OR:   { int64_t b = vm_pop(vm), a = vm_pop(vm); push(vm, (int64_t)((uint64_t)a | (uint64_t)b)); break; }
        case OP_XOR:  { int64_t b = vm_pop(vm), a = vm_pop(vm); push(vm, (int64_t)((uint64_t)a ^ (uint64_t)b)); break; }
        case OP_SHL:  { int64_t b = vm_pop(vm), a = vm_pop(vm); push(vm, (int64_t)((uint64_t)a << (uint64_t)(b & 63))); break; }
        case OP_SHR:  { int64_t b = vm_pop(vm), a = vm_pop(vm); push(vm, (int64_t)((uint64_t)a >> (uint64_t)(b & 63))); break; }
        case OP_RAND: {
            int64_t n = vm_pop(vm);
            if (n <= 0) vm_error(vm, "invalid random range");
            uint64_t un = (uint64_t)n;
            uint64_t r = prng_next(vm);
            uint64_t limit = UINT64_MAX - (UINT64_MAX % un);
            while (r >= limit) r = prng_next(vm);
            push(vm, (int64_t)(r % un));
            break;
        }
        case OP_STORE: {
            int64_t value = vm_pop(vm);
            int64_t addr = vm_pop(vm);
            mem_store(vm, addr, value);
            break;
        }
        case OP_LOAD: {
            int64_t addr = vm_pop(vm);
            push(vm, mem_load(vm, addr));
            break;
        }
        /* everything below is a syscall in ABI clothing */
        case OP_SLEEP: {
            /* legacy compat: the sleep codepoint pops ms and pushes nothing.
               The SYS_SLEEP syscall itself returns 0 for modern `sys 15`
               callers; only the result carrying is dropped here. */
            int64_t ms = vm_pop(vm);
            int64_t sargs[1];
            sargs[0] = ms;
            do_syscall(vm, SYS_SLEEP, 1, sargs);
            break;
        }
        case OP_SOCK_CREATE: op_sys(vm, SYS_SOCK_CREATE); break;
        case OP_SOCK_BIND:   op_sys(vm, SYS_SOCK_BIND); break;
        case OP_SOCK_LISTEN: op_sys(vm, SYS_SOCK_LISTEN); break;
        case OP_SOCK_ACCEPT: op_sys(vm, SYS_SOCK_ACCEPT); break;
        case OP_SOCK_RECV:   op_sys(vm, SYS_SOCK_RECV); break;
        case OP_SOCK_SEND:   op_sys(vm, SYS_SOCK_SEND); break;
        case OP_SOCK_CLOSE:  op_sys(vm, SYS_SOCK_CLOSE); break;
        case OP_FILE_OPEN:   op_sys(vm, SYS_FILE_OPEN); break;
        case OP_FILE_READ:   op_sys(vm, SYS_FILE_READ); break;
        case OP_FILE_STAT:   op_sys(vm, SYS_FILE_STAT); break;
        case OP_DIR_LIST:    op_sys(vm, SYS_DIR_LIST); break;
        case OP_FILE_CREATE: op_sys(vm, SYS_FILE_CREATE); break;
        case OP_FILE_WRITE:  op_sys(vm, SYS_FILE_WRITE); break;
        case OP_FILE_APPEND: op_sys(vm, SYS_FILE_APPEND); break;
        case OP_SYSCALL: {
            int64_t num = vm_pop(vm);
            int argc = sys_argc(num);
            if (argc > 0) {
                int64_t args[INK_MAX_SYS_ARGS];
                if (argc > INK_MAX_SYS_ARGS) argc = INK_MAX_SYS_ARGS;
                for (int i = argc; i-- > 0;) args[i] = vm_pop(vm);
                push(vm, do_syscall(vm, num, argc, args));
            } else {
                push(vm, do_syscall(vm, num, 0, NULL));
            }
            break;
        }
        case OP_CALL: {
            int64_t n = vm_pop(vm);
            if (depth > vm->max_depth) vm_error(vm, "call depth exceeded (possible infinite recursion)");
            if (n < 0 || (size_t)n >= vm->word_count) vm_error(vm, "call to undefined word");
            exec_block(vm, vm->words[(size_t)n], depth + 1, 0);
            break;
        }
        case OP_IF: {
            int64_t c = vm_pop(vm);
            if (c) exec_block(vm, ins->then_blk, depth, nest + 1);
            else if (ins->else_blk) exec_block(vm, ins->else_blk, depth, nest + 1);
            break;
        }
        case OP_LOOP: {
            for (;;) {
                exec_block(vm, ins->then_blk, depth, nest + 1);
                int64_t c = vm_pop(vm);
                if (!c) break;
                exec_block(vm, ins->else_blk, depth, nest + 1);
            }
            break;
        }
        case OP_HALT:
            vm->outcome = OUT_HALT;
            longjmp(vm->jmp, 1);
    }
}

static void exec_block(InkVM *vm, Block *blk, int depth, int nest) {
    /* `nest` counts *static* if/while structure only: a call re-roots at 0
       (call chains are bounded by max_depth), while each if/while descent
       costs one. This recursion lives on the host's C stack, so bound it —
       a crafted pathologically nested blob must fault cleanly instead of
       exhausting the stack. */
    if (nest > INK_MAX_BLOCK_NEST)
        vm_error(vm, "block nesting too deep (static if/while structure)");
    for (size_t i = 0; i < blk->count; i++) {
        exec_instr(vm, &blk->items[i], depth, nest);
    }
}

/* ---------- public ---------- */

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
        case OP_FILE_CREATE: return "file-create";
        case OP_FILE_WRITE: return "file-write";
        case OP_FILE_APPEND: return "file-append";
        case OP_SYSCALL: return "sys";
    }
    return "?";
}

void vm_init(InkVM *vm, const InkHost *host) {
    unsigned char *b = (unsigned char *)vm;
    for (size_t i = 0; i < sizeof(*vm); i++) b[i] = 0;
    vm->host = host;
    vm->max_steps = DEFAULT_MAX_STEPS;
    vm->max_depth = DEFAULT_MAX_DEPTH;
    vm->quantum_end = UINT64_MAX;
    vm->wait.fd = -1;
}

void vm_destroy(InkVM *vm) {
    if (vm->words) {
        for (size_t i = 0; i < vm->word_count; i++) block_free(vm->words[i]);
        vm->host->realloc(vm->words, 0);
        vm->words = NULL;
        vm->word_count = 0;
    }
    vm->host->realloc(vm->stack, 0); vm->stack = NULL; vm->sp = vm->cap = 0;
    vm->host->realloc(vm->mem, 0);   vm->mem = NULL;   vm->mem_cap = 0;
    vm->prng_seeded = false;
}

void vm_load_program(InkVM *vm, cp_t *cps, size_t len) {
    vm->words = split_and_parse_words(cps, len, &vm->word_count);
    vm->host->realloc(cps, 0);
    if (!vm->words || vm->word_count == 0 || len == 0) {
        xstr_copy(vm->errmsg, sizeof(vm->errmsg),
                  g_parse_err[0] ? g_parse_err : "no program found in document");
        if (vm->words) {
            for (size_t i = 0; i < vm->word_count; i++) block_free(vm->words[i]);
            vm->host->realloc(vm->words, 0);
            vm->words = NULL;
            vm->word_count = 0;
        }
        vm->outcome = OUT_ERROR;
        return;
    }
    vm->outcome = OUT_NONE;
}

Outcome vm_run(InkVM *vm) {
    if (!vm->words || vm->word_count == 0) {
        if (vm->outcome != OUT_ERROR) {
            xstr_copy(vm->errmsg, sizeof(vm->errmsg), "no program loaded");
            vm->outcome = OUT_ERROR;
        }
        return vm->outcome;
    }
    vm->outcome = OUT_NONE;
    if (setjmp(vm->jmp) == 0) {
        exec_block(vm, vm->words[0], 0, 0);
        vm->outcome = OUT_HALT;  /* word 0 returns implicitly: success */
    }
    return vm->outcome;
}

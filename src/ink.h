#ifndef INK_H
#define INK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <setjmp.h>

#define INK_VERSION "1.5.0"

typedef uint32_t cp_t;

/* spec codepoints */
enum {
    CP_ITIMES  = 0x2062,
    CP_NBSP    = 0x00A0,
    CP_FIGURE  = 0x2007,
    CP_THIN    = 0x2009,
    CP_HAIR    = 0x200A,
    CP_DIG0    = 0x2005,
    CP_DIG1    = 0x2006,
    CP_IDEO    = 0x3000,
    CP_PUNCT   = 0x2008,
    CP_MEDMATH = 0x205F,
    CP_EN      = 0x2002,
    CP_EM      = 0x2003,
    CP_THREE   = 0x2004,
    CP_ZWNJ    = 0x200C,
    CP_ZWJ     = 0x200D,
    CP_WJ      = 0x2060,
    CP_FA      = 0x2061,
    CP_LS      = 0x2000,
    CP_PS      = 0x2001,
    CP_ZWSP    = 0x200B,
    CP_BOM     = 0xFEFF,
    CP_INV_SEP = 0x2063,
    CP_INV_PLUS= 0x2064,
    CP_LRM     = 0x200E,
    CP_RLM     = 0x200F,
    CP_SOFT    = 0x00AD,
    /* isa v2: bitwise + shifts */
    CP_LRE     = 0x202A,            /* and */
    CP_RLE     = 0x202B,            /* or */
    CP_PDF     = 0x202C,            /* xor */
    CP_LRO     = 0x202D,            /* shl */
    CP_RLO     = 0x202E,            /* shr */
    /* syscall extensions: socket + file */
    CP_SOCK_CREATE = 0x2065,
    CP_SOCK_BIND   = 0x2066,
    CP_SOCK_LISTEN = 0x2067,
    CP_SOCK_ACCEPT = 0x2068,
    CP_SOCK_RECV   = 0x2069,
    CP_SOCK_SEND   = 0x206A,
    CP_SOCK_CLOSE  = 0x206B,
    CP_FILE_OPEN   = 0x206C,
    CP_FILE_READ   = 0x206D,
    CP_FILE_STAT   = 0x206E,
    CP_DIR_LIST    = 0x206F,
    /* interlinear annotation trio: file writes */
    CP_FILE_CREATE = 0xFFF9,            /* INTERLINEAR ANNOTATION ANCHOR */
    CP_FILE_WRITE  = 0xFFFA,            /* INTERLINEAR ANNOTATION SEPARATOR */
    CP_FILE_APPEND = 0xFFFB,            /* INTERLINEAR ANNOTATION TERMINATOR */
    /* isa v4: the one generic syscall op. U+FFF0 is unassigned; it sits in the
       same extension neighborhood as the file-write trio and is treated like
       the rest of the spec: parsed codepoint, not a printable character.
       ALL FUTURE KERNEL FACILITIES ARE SYS NUMBERS, NEVER NEW CODEPOINTS. */
    CP_SYS = 0xFFF0,
    /* isa v3: loops */
    CP_ALM = 0x061C                  /* ARABIC LETTER MARK: while */
};

/* opcode */
typedef enum {
    OP_PUSH, OP_DUP, OP_SWAP, OP_DROP, OP_ROT,
    OP_NEG, OP_SUB, OP_MOD, OP_ID, OP_ADD, OP_DIV, OP_SIGN, OP_MUL, OP_POW,
    OP_AND, OP_OR, OP_XOR, OP_SHL, OP_SHR,
    OP_NUMOUT, OP_CHAROUT, OP_CHARIN,
    OP_CALL, OP_IF, OP_LOOP, OP_HALT,
    OP_RAND, OP_STORE, OP_LOAD, OP_SLEEP,
    OP_SOCK_CREATE, OP_SOCK_BIND, OP_SOCK_LISTEN, OP_SOCK_ACCEPT,
    OP_SOCK_RECV, OP_SOCK_SEND, OP_SOCK_CLOSE,
    OP_FILE_OPEN, OP_FILE_READ, OP_FILE_STAT, OP_DIR_LIST,
    OP_FILE_CREATE, OP_FILE_WRITE, OP_FILE_APPEND,
    OP_SYSCALL
} OpCode;

typedef struct Block Block;

typedef struct {
    OpCode op;
    int64_t value;
    Block *then_blk;
    Block *else_blk;
} Instr;

struct Block {
    Instr *items;
    size_t count;
    size_t cap;
};

/*
 * Syscall ABI v1. Fixed numbers 1..17 are implemented by every host; 18..19
 * (mem-set / mem-copy) are implemented in the core itself and work on every
 * host automatically. Kernels are free to add numbers above 19 — no new
 * codepoints are ever burned for them (see InkSys for the assignment policy).
 *
 * Calling convention: push the args in order, push the syscall number,
 * then `syscall`. The VM pops the number, pops that many args (the most
 * recently pushed is closest to the number) and delivers them to the host
 * in first-pushed-first order. Result is pushed.
 *
 * As raw ISA:  HAIR <bits-of-num> EN QUAD, U+FFF0
 * In Quill:    sys <num>
 * In Scribe:   sys(num[, a, b, c, d])       (value builtin, 1..4 args? no: 1..N)
 */
enum InkSys {
    SYS_SOCK_CREATE = 1,     /* () -> fd | -1 */
    SYS_SOCK_BIND,           /* (fd, port) -> 0 | -1 */
    SYS_SOCK_LISTEN,         /* (fd, backlog) -> 0 | -1 */
    SYS_SOCK_ACCEPT,         /* (fd) -> cfd | -1  (may suspend) */
    SYS_SOCK_RECV,           /* (fd, buf, len) -> n | -1 (may suspend) */
    SYS_SOCK_SEND,           /* (fd, buf, len) -> n */
    SYS_SOCK_CLOSE,          /* (fd) -> 0 | -1 */
    SYS_FILE_OPEN,           /* (path_ptr, path_len, flags) -> fd | -1 */
    SYS_FILE_READ,           /* (fd, buf, len) -> n | -1 */
    SYS_FILE_STAT,           /* (path_ptr, path_len) -> size | -1 | -2 */
    SYS_DIR_LIST,            /* (path_ptr, path_len, out_buf, out_cap) -> n | -1 */
    SYS_FILE_CREATE,         /* (path_ptr, path_len) -> fd | -1 */
    SYS_FILE_WRITE,          /* (fd, buf, len) -> n */
    SYS_FILE_APPEND,         /* (path_ptr, path_len) -> fd | -1 */
    SYS_SLEEP,               /* (ms) -> 0 (may suspend until deadline) */
    SYS_YIELD,               /* () -> 0 (voluntary reschedule) */
    SYS_EXIT,                /* (code) -> does not return; process exit code */

    /*
     * Core-ABI extension (ISA v4.1): numbers 18..19 live IN the core (vm.c),
     * not in any host — they only touch the memory tape, so every host
     * (desktop, scheduler, bare-metal kernel) gets them for free. Numbers
     * 20 and up are unassigned: they are reserved for a future OS-class ABI
     * (spawn/wait/pid/IPC/clock/framebuffer/blockr) and are NOT to be claimed
     * ad hoc by individual hosts; kernels assign them once, here, together.
     */
    SYS_MEMSET = 18,         /* (addr, fill, len) -> 0  (may grow the tape) */
    SYS_MEMCOPY = 19         /* (dst, src, len) -> 0    (memmove semantics) */
};
#define INK_MAX_SYS_ARGS 8

/*
 * Outcomes. The interpreter is resumable at op boundaries:
 *   OUT_HALT   terminal, word 0 ended (implicitly or via ZWSP/halt)
 *   OUT_ERROR  terminal, vm->errmsg holds the fault; the process dies,
 *              the scheduler lives
 *   OUT_YIELD  quantum expired / voluntary reschedule; the scheduler
 *              regains control, the VM keeps its exact state and resumes
 *              later transparently
 *   OUT_WAIT   parked on vm->wait (fd event or deadline); same resume
 *              contract as OUT_YIELD
 */
typedef enum { OUT_NONE, OUT_HALT, OUT_ERROR, OUT_YIELD, OUT_WAIT } Outcome;

enum {
    INK_WAIT_NONE     = 0,
    INK_WAIT_FD_IN    = 1,   /* wait until vm->wait.fd is readable */
    INK_WAIT_FD_OUT   = 2,   /* wait until vm->wait.fd is writable */
    INK_WAIT_DEADLINE = 3    /* wait until env->now_ms() >= vm->wait.deadline_ms */
};

typedef struct InkVM InkVM;

typedef struct {
    int kind;               /* INK_WAIT_* */
    int fd;
    int64_t deadline_ms;
} InkWait;

/*
 * Host environment: everything the core may not do itself (libc, syscalls,
 * I/O, context switching). The core is freestanding; hosts plug in here.
 *
 * suspend is the scheduling hook. It is called with vm->outcome already set
 * to OUT_YIELD or OUT_WAIT; on return from the call the interpreter
 * continues transparently from exactly where it was. A host without a
 * scheduler leaves suspend NULL and sets allow_yield = false; desktop
 * `ink run` does exactly that and keeps its historical step-limit/IO
 * behavior bit for bit.
 */
typedef struct InkHost InkHost;
struct InkHost {
    void    (*char_out)(InkVM *vm, int64_t cp);
    int64_t (*char_in)(InkVM *vm);          /* -1 on EOF; may suspend */
    uint64_t(*rand_seed)(InkVM *vm);        /* 64 bits of entropy */
    void    (*fatal)(InkVM *vm, const char *what);   /* abort whole program */
    void   *(*realloc)(void *p, size_t n);
    void    (*suspend)(InkVM *vm, Outcome reason);   /* NULL = no scheduler */
    int64_t (*now_ms)(InkVM *vm);                    /* monotonic ms; may be NULL */
    int64_t (*sys)(InkVM *vm, int64_t num, int argc, const int64_t *args);
    /* sys: returns result value; may suspend mid-way (accept/recv/sleep). */
};

struct InkVM {
    /* environment */
    const InkHost *host;

    /* program (per-instance: a process is one VM) */
    Block   **words;
    size_t    word_count;

    /* stack */
    int64_t  *stack;
    size_t    sp, cap;

    /* memory tape */
    int64_t  *mem;
    size_t    mem_cap;
    size_t    mem_limit;        /* 0 = unlimited; store past it = fault */

    size_t    stack_limit;      /* 0 = unlimited; growth past it = fault (cells) */

    /* limits / scheduling */
    unsigned long steps;      /* total executed */
    unsigned long max_steps;  /* 0 = unlimited (absolute cap) */
    uint64_t      quantum_end;/* when allow_yield: yield at this step count */
    int           max_depth;

    bool          allow_yield;/* enable OUT_YIELD / OUT_WAIT */

    /* prng */
    uint64_t prng_state;
    bool     prng_seeded;

    /* outcomes */
    Outcome  outcome;
    int64_t  exit_code;       /* set by SYS_EXIT */
    InkWait  wait;
    char     errmsg[256];

    jmp_buf  jmp;
};

#define DEFAULT_MAX_STEPS 10000000UL
#define DEFAULT_MAX_DEPTH 20000

/* Cap on *static* if/while nesting ( recursing on the host's C stack,
   unlike call depth which is checked at 20000 ). Scribe/Quill programs
   nest a handful of levels; this bound is panic headroom for hostile blobs. */
#define INK_MAX_BLOCK_NEST 1024

#endif /* INK_H */

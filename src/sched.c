#include "sched.h"
#include "vm.h"
#include "parse.h"
#include "progload.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <ucontext.h>
#include <poll.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_PROCS      64
#define PROC_STACK     (512 * 1024)
#define RECV_CAP       8192
#define SEND_CAP       1048576

typedef struct Proc {
    InkVM        vm;      /* MUST be first: (Proc*)vm casts are used below */
    ucontext_t   ctx;
    char        *stk;
    const char  *path;
    bool         alive;   /* still schedulable (not halted/errored) */
    bool         ready;   /* in the round-robin rotation */
    /* wait state after a suspension */
    int          wait_kind;    /* INK_WAIT_* */
    int          wait_fd;
    int64_t      wait_deadline; /* absolute monotonic ms */
} Proc;

static Proc      *g_procs[MAX_PROCS];
static int        g_nprocs = 0;
static ucontext_t g_sched_ctx;
static ucontext_t g_exit_ctx;
static unsigned long g_quantum = 50000;

/* ------------------------------------------------------------------ */
/* non-blocking host                                                   */
/* ------------------------------------------------------------------ */

static void sh_char_out(InkVM *vm, int64_t cp) {
    (void)vm;
    utf8_put(cp);
    fflush(stdout);   /* processes interleave: flush per char */
}

static int rd1(unsigned char *out) {
    ssize_t n = read(0, out, 1);
    if (n == 1) return 1;
    if (n == 0) return 0;              /* EOF */
    if (errno == EINTR) return -2;
    if (errno == EAGAIN || errno == EWOULDBLOCK) return -1; /* would wait */
    return 0;
}

static int64_t sh_char_in(InkVM *vm) {
    unsigned char c;
    for (;;) {
        int r = rd1(&c);
        if (r == 1) break;
        if (r == 0) return -1;                       /* EOF */
        if (r == -2) continue;                       /* EINTR */
        vm_wait_fd(vm, 0, false);                    /* park on stdin */
    }
    if (c < 0x80) return c;
    int extra = ((c & 0xE0) == 0xC0) ? 1 : ((c & 0xF0) == 0xE0) ? 2 : 3;
    int64_t v = c & (extra == 1 ? 0x1F : extra == 2 ? 0x0F : 0x07);
    for (int k = 0; k < extra; k++) {
        unsigned char cc;
        for (;;) {
            int r = rd1(&cc);
            if (r == 1) break;
            if (r == 0) return -1;
            if (r == -2) continue;
            vm_wait_fd(vm, 0, false);
        }
        v = (v << 6) | (cc & 0x3F);
    }
    return v;
}

/* The heart of the story: syscalls park instead of block. */
static int64_t sh_sys(InkVM *vm, int64_t num, int argc, const int64_t *args) {
    switch (num) {
        case SYS_SOCK_CREATE: {
            (void)argc;
            int fd = socket(AF_INET, SOCK_STREAM, 0);
            if (fd < 0) return -1;
            int opt = 1;
            setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
            fcntl(fd, F_SETFL, O_NONBLOCK);
            return (int64_t)fd;
        }
        case SYS_SOCK_ACCEPT: {
            (void)argc;
            int64_t fd = args[0];
            if (fd < 0) return -1;
            for (;;) {
                int cfd = accept((int)fd, NULL, NULL);
                if (cfd >= 0) return (int64_t)cfd;
                if (errno == EINTR) continue;
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    vm_wait_fd(vm, (int)fd, false);
                    continue;
                }
                return -1;
            }
        }
        case SYS_SOCK_RECV: {
            (void)argc;
            int64_t fd = args[0], buf = args[1], len = args[2];
            if (fd < 0 || buf < 0 || len < 0) return -1;
            if (len == 0) return 0;
            if (len > RECV_CAP) len = RECV_CAP;
            char *tmp = malloc((size_t)len);
            if (!tmp) { vm->host->fatal(vm, "out of memory"); return -1; }
            for (;;) {
                ssize_t n = recv((int)fd, tmp, (size_t)len, MSG_DONTWAIT);
                if (n >= 0) {
                    const unsigned char *b = (const unsigned char *)tmp;
                    for (ssize_t i = 0; i < n; i++)
                        vm_mem_write(vm, buf + (int64_t)i, (int64_t)b[i]);
                    free(tmp);
                    return (int64_t)n;
                }
                if (errno == EINTR) continue;
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    /* keep tmp: the buffer is part of the suspended state */
                    vm_wait_fd(vm, (int)fd, false);
                    continue;
                }
                free(tmp);
                return -1;
            }
        }
        case SYS_SOCK_SEND: {
            (void)argc;
            int64_t fd = args[0], buf = args[1], len = args[2];
            if (fd < 0 || buf < 0 || len < 0) return -1;
            if (len == 0) return 0;
            if (len > SEND_CAP) len = SEND_CAP;
            char *tmp = malloc((size_t)len);
            if (!tmp) { vm->host->fatal(vm, "out of memory"); return -1; }
            for (int64_t i = 0; i < len; i++)
                tmp[i] = (char)(vm_mem_read(vm, buf + i) & 0xFF);
            ssize_t total = 0;
            while (total < len) {
                ssize_t sent = send((int)fd, tmp + total, (size_t)(len - total), MSG_DONTWAIT);
                if (sent > 0) { total += sent; continue; }
                if (sent < 0 && errno == EINTR) continue;
                if (sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                    /* keep tmp: the buffer is part of the suspended state */
                    vm_wait_fd(vm, (int)fd, true);
                    continue;
                }
                break;
            }
            free(tmp);
            return (int64_t)total;
        }
        case SYS_SLEEP: {
            (void)argc;
            int64_t ms = args[0];
            if (ms <= 0) return 0;
            int64_t now = ink_now_ms_posix(vm);
            vm_wait_deadline(vm, now + ms);   /* park until the deadline */
            return 0;
        }
        default:
            return ink_sys_posix(vm, num, argc, args);
    }
}

/* suspend = context switch. On resume, execution continues inside whatever
   host call parked the process — no re-entry logic anywhere. */
static void sh_suspend(InkVM *vm, Outcome reason) {
    (void)reason;
    Proc *p = (Proc *)vm;   /* InkVM is Proc's first member */
    swapcontext(&p->ctx, &g_sched_ctx);
}

static void *sh_realloc(void *q, size_t n) { return realloc(q, n); }

static void sh_fatal(InkVM *vm, const char *what) {
    (void)vm;
    fprintf(stderr, "sched: fatal: %s\n", what);
    exit(1);
}

static const InkHost sched_host = {
    .char_out  = sh_char_out,
    .char_in   = sh_char_in,
    .rand_seed = ink_rand_seed_posix,
    .fatal     = sh_fatal,
    .realloc   = sh_realloc,
    .suspend   = sh_suspend,
    .now_ms    = ink_now_ms_posix,
    .sys       = sh_sys,
};

/* ------------------------------------------------------------------ */
/* process bodies                                                      */
/* ------------------------------------------------------------------ */

static void proc_body(int idx) {
    Proc *p = g_procs[idx];
    Outcome o = vm_run(&p->vm);
    (void)o;                       /* terminal outcome lives in p->vm */
    p->alive = false;
    swapcontext(&p->ctx, &g_sched_ctx);   /* hand control back for good */
}

static void exit_guard(void) {
    /* only reachable if a finished context is ever resumed by mistake */
    fprintf(stderr, "sched: internal error: resumed a finished process\n");
    exit(3);
}

/* ------------------------------------------------------------------ */
/* scheduler                                                           */
/* ------------------------------------------------------------------ */

static void report_proc(Proc *p) {
    if (p->vm.outcome == OUT_HALT) {
        fprintf(stderr, "[sched] %s: exit %lld (halt), steps=%lu\n",
                p->path, (long long)p->vm.exit_code, p->vm.steps);
    } else if (p->vm.outcome == OUT_ERROR) {
        fprintf(stderr, "[sched] %s: runtime error: %s (steps=%lu)\n",
                p->path, p->vm.errmsg, p->vm.steps);
    }
}

int ink_sched_run(const char *const *paths, int npaths,
                  unsigned long quantum, unsigned long steps, int depth,
                  unsigned long long stack) {
    if (npaths < 1) { fprintf(stderr, "sched: no input files\n"); return 2; }
    if (quantum == 0) { fprintf(stderr, "sched: --quantum must be >= 1\n"); return 2; }
    if (npaths > MAX_PROCS) { fprintf(stderr, "sched: max %d processes\n", MAX_PROCS); return 2; }

    g_quantum = quantum;
    signal(SIGPIPE, SIG_IGN);

    int old_stdin_flags = fcntl(0, F_GETFL, 0);
    if (old_stdin_flags >= 0) fcntl(0, F_SETFL, old_stdin_flags | O_NONBLOCK);

    getcontext(&g_exit_ctx);
    g_exit_ctx.uc_stack.ss_sp = malloc(PROC_STACK);
    g_exit_ctx.uc_stack.ss_size = PROC_STACK;
    g_exit_ctx.uc_link = NULL;
    makecontext(&g_exit_ctx, (void (*)(void))exit_guard, 0);

    for (int i = 0; i < npaths; i++) {
        Proc *p = calloc(1, sizeof(Proc));
        if (!p) { fprintf(stderr, "sched: out of memory\n"); return 1; }
        p->path = paths[i];
        p->alive = true;
        p->ready = true;
        p->wait_kind = INK_WAIT_NONE;
        p->wait_fd = -1;

        InkProg prog;
        if (!ink_prog_load_path(paths[i], &prog)) {
            fprintf(stderr, "%s: %s\n", paths[i], prog.err);
            ink_prog_free(&prog);
            free(p);
            return 1;
        }

        vm_init(&p->vm, &sched_host);
        p->vm.max_steps = steps;
        p->vm.max_depth = depth > 0 ? depth : DEFAULT_MAX_DEPTH;
        p->vm.stack_limit = stack ? (size_t)stack : 0;
        p->vm.allow_yield = true;
        p->vm.quantum_end = p->vm.steps + quantum;
        if (!ink_prog_into_vm(&prog, &p->vm)) {
            if (prog.kind == PROG_TEXT && prog.err_pos)
                fprintf(stderr, "%s: parse error: %s (at codepoint %zu)\n",
                        paths[i], prog.err, prog.err_pos);
            else
                fprintf(stderr, "%s: %s\n", paths[i], prog.err);
            ink_prog_free(&prog);
            vm_destroy(&p->vm);
            free(p);
            return 1;
        }
        ink_prog_free(&prog);   /* mem slots already copied; frees leftovers */

        g_procs[g_nprocs++] = p;
    }

    /* per-process contexts (getcontext is returns_twice: keep i in memory) */
    for (volatile int i = 0; i < g_nprocs; i++) {
        Proc *p = g_procs[i];
        getcontext(&p->ctx);
        p->stk = malloc(PROC_STACK);
        if (!p->stk) { fprintf(stderr, "sched: out of memory\n"); return 1; }
        p->ctx.uc_stack.ss_sp = p->stk;
        p->ctx.uc_stack.ss_size = PROC_STACK;
        p->ctx.uc_link = &g_exit_ctx;
        makecontext(&p->ctx, (void (*)(void))proc_body, 1, i);
    }

    /* round-robin with parking */
    int alive = g_nprocs;
    while (alive > 0) {
        /* run every ready process once; `pi` must live in memory, not a
           register, because swapcontext saves/restores contexts around it */
        for (volatile int pi = 0; pi < g_nprocs; pi++) {
            Proc *p = g_procs[pi];
            if (!p->alive || !p->ready) continue;
            p->ready = false;                 /* consumed this turn */
            p->vm.quantum_end = p->vm.steps + g_quantum;
            p->vm.outcome = OUT_NONE;
            swapcontext(&g_sched_ctx, &p->ctx);
            if (!p->alive) { report_proc(p); alive--; continue; }
            /* it suspended: outcome says why */
            if (p->vm.outcome == OUT_WAIT) {
                p->wait_kind = p->vm.wait.kind;
                p->wait_fd = p->vm.wait.fd;
                p->wait_deadline = p->vm.wait.deadline_ms;
            } else if (p->vm.outcome == OUT_YIELD) {
                p->ready = true;
            } else {
                /* OUT_NONE here would be a core bug; treat as yield */
                p->ready = true;
            }
            p->wait_kind = (p->vm.outcome == OUT_WAIT) ? p->wait_kind : INK_WAIT_NONE;
        }

        /* anyone still alive must be waiting on something now */
        bool any_ready = false;
        bool any_deadline = false;
        int64_t earliest = 0;
        struct pollfd fds[MAX_PROCS];
        Proc *fd_proc[MAX_PROCS];
        int nfds = 0;
        for (int i = 0; i < g_nprocs; i++) {
            Proc *p = g_procs[i];
            if (!p->alive) continue;
            if (p->wait_kind == INK_WAIT_FD_IN || p->wait_kind == INK_WAIT_FD_OUT) {
                fds[nfds].fd = p->wait_fd;
                fds[nfds].events = (p->wait_kind == INK_WAIT_FD_IN) ? POLLIN : POLLOUT;
                fds[nfds].revents = 0;
                fd_proc[nfds] = p;
                nfds++;
            }
            if (p->wait_kind == INK_WAIT_DEADLINE) {
                if (!any_deadline || p->wait_deadline < earliest) {
                    earliest = p->wait_deadline;
                }
                any_deadline = true;
            }
            if (p->ready) any_ready = true;
        }

        if (any_ready) continue;   /* round robin continues immediately */

        if (nfds == 0 && !any_deadline) {
            /* waiting on nothing pollable and no deadline: a blocked listener
               with dead clients — nothing will ever wake these. Report and
               stop rather than hang. */
            break;
        }

        int timeout = -1;
        if (any_deadline) {
            int64_t now = ink_now_ms_posix(NULL);
            timeout = (int)(earliest - now);
            if (timeout < 0) timeout = 0;
            if (timeout > 1000) timeout = 1000; /* re-check fds often */
        }
        int rc = poll(fds, (nfds_t)nfds, timeout);
        if (rc < 0 && errno != EINTR) break;

        /* wake poll-able fds */
        for (int k = 0; k < nfds; k++) {
            if (fds[k].revents & (POLLIN | POLLOUT | POLLERR | POLLHUP)) {
                fd_proc[k]->ready = true;
                fd_proc[k]->wait_kind = INK_WAIT_NONE;
            }
        }
        /* wake expired deadlines */
        int64_t now2 = ink_now_ms_posix(NULL);
        for (int i = 0; i < g_nprocs; i++) {
            Proc *p = g_procs[i];
            if (p->alive && p->wait_kind == INK_WAIT_DEADLINE && now2 >= p->wait_deadline) {
                p->ready = true;
                p->wait_kind = INK_WAIT_NONE;
            }
        }
        /* if poll timed out and nothing woke, loop re-enters; ready procs run */
        bool woke = false;
        for (int i = 0; i < g_nprocs; i++)
            if (g_procs[i]->alive && g_procs[i]->ready) woke = true;
        if (!woke && timeout == 0) {
            /* avoid a hot spin between deadline checks */
            struct timespec ts = { .tv_sec = 0, .tv_nsec = 1000000 }; /* 1ms */
            nanosleep(&ts, NULL);
        }
    }

    if (old_stdin_flags >= 0) fcntl(0, F_SETFL, old_stdin_flags);

    int failures = 0;
    for (int i = 0; i < g_nprocs; i++) {
        Proc *p = g_procs[i];
        if (p->vm.outcome == OUT_HALT && p->vm.exit_code != 0) failures++;
        if (p->vm.outcome != OUT_HALT) failures++;
        vm_destroy(&p->vm);
        free(p->stk);
        free(p);
    }
    return failures ? 1 : 0;
}

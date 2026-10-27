/*
 * vmhost.c — POSIX host layer (desktop `ink`).
 *
 * Only this file (and parse/quill/scc/etc. tooling) touches libc. The blocking
 * desktop host behaves byte-identically to the pre-instance interpreter:
 * char-in flushes stdout, sleep flushes, accept flushes, /dev/urandom seeding,
 * SIGPIPE ignored around send, 8192/1048576 buffer caps, dir-list sorted
 * case-insensitively, etc.
 *
 * Schedulers (sched.c, kernels) reuse individual impls and add non-blocking
 * wrappers for accept/recv/char-in/sleep.
 */
#include "vm.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <dirent.h>

#define RECV_CAP   8192
#define SEND_CAP   1048576
#define PATH_CAP   4096

/* ---------- host vtable pieces ---------- */

static void posix_char_out(InkVM *vm, int64_t cp) {
    (void)vm;
    utf8_put(cp);
}

static int64_t posix_char_in(InkVM *vm) {
    (void)vm;
    fflush(stdout);
    return utf8_get_stdin();
}

uint64_t ink_rand_seed_posix(InkVM *vm) {
    (void)vm;
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
    return seed;
}

static void posix_fatal(InkVM *vm, const char *what) {
    (void)vm;
    fprintf(stderr, "ink: fatal: %s\n", what);
    exit(1);
}

static void *posix_realloc(void *p, size_t n) {
    return realloc(p, n);
}

int64_t ink_now_ms_posix(InkVM *vm) {
    (void)vm;
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

/* ---------- byte-string helpers over the tape ---------- */

static bool tape_cstr(InkVM *vm, int64_t addr, int64_t len, char **out) {
    bool ok;
    char *s = vm_mem_cstr(vm, addr, len, &ok);
    if (!ok || !s) return false;
    *out = s;
    return true;
}

static int64_t tape_read_into(InkVM *vm, int64_t buf, const void *tmp, size_t n) {
    const unsigned char *b = tmp;
    for (size_t i = 0; i < n; i++)
        vm_mem_write(vm, buf + (int64_t)i, (int64_t)b[i]);
    return (int64_t)n;
}

/* ---------- syscall implementations (ABI v1) ---------- */

/* The blocking implementation every non-suspending number shares. Schedulers
   override only SYS_SOCK_ACCEPT, SYS_SOCK_RECV, SYS_SLEEP (and char_in) with
   waiting versions and delegate everything else here. */
int64_t ink_sys_posix(InkVM *vm, int64_t num, int argc, const int64_t *args) {
    (void)argc;
    switch (num) {
        case SYS_SOCK_CREATE: {
            int fd = socket(AF_INET, SOCK_STREAM, 0);
            if (fd < 0) return -1;
            int opt = 1;
            setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
            return (int64_t)fd;
        }
        case SYS_SOCK_BIND: {
            int64_t fd = args[0], port = args[1];
            if (fd < 0 || port < 0 || port > 65535) return -1;
            struct sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = htonl(INADDR_ANY);
            addr.sin_port = htons((uint16_t)port);
            return bind((int)fd, (struct sockaddr *)&addr, sizeof(addr)) == 0 ? 0 : -1;
        }
        case SYS_SOCK_LISTEN: {
            int64_t fd = args[0], backlog = args[1];
            if (fd < 0 || backlog < 0) return -1;
            if (backlog > 128) backlog = 128;
            if (backlog < 1) backlog = 5;
            return listen((int)fd, (int)backlog) == 0 ? 0 : -1;
        }
        case SYS_SOCK_CLOSE: {
            int64_t fd = args[0];
            if (fd < 0) return -1;
            return close((int)fd) == 0 ? 0 : -1;
        }
        case SYS_FILE_OPEN: {
            int64_t path_ptr = args[0], path_len = args[1];
            if (path_ptr < 0 || path_len < 0 || path_len > PATH_CAP) return -1;
            char *path;
            if (!tape_cstr(vm, path_ptr, path_len, &path)) return -1;
            int fd = open(path, O_RDONLY);
            vm->host->realloc(path, 0);
            return fd;
        }
        case SYS_FILE_READ: {
            int64_t fd = args[0], buf = args[1], len = args[2];
            if (fd < 0 || buf < 0 || len < 0) return -1;
            if (len == 0) return 0;
            if (len > RECV_CAP) len = RECV_CAP;
            char *tmp = malloc((size_t)len);
            if (!tmp) { fprintf(stderr, "ink: fatal: out of memory\n"); exit(1); }
            ssize_t n = read((int)fd, tmp, (size_t)len);
            if (n < 0) { free(tmp); return -1; }
            tape_read_into(vm, buf, tmp, (size_t)n);
            free(tmp);
            return (int64_t)n;
        }
        case SYS_FILE_STAT: {
            int64_t path_ptr = args[0], path_len = args[1];
            if (path_ptr < 0 || path_len < 0 || path_len > PATH_CAP) return -1;
            char *path;
            if (!tape_cstr(vm, path_ptr, path_len, &path)) return -1;
            struct stat st;
            int rc = stat(path, &st);
            vm->host->realloc(path, 0);
            if (rc != 0) return -1;
            if (S_ISDIR(st.st_mode)) return -2;
            return (int64_t)st.st_size;
        }
        case SYS_DIR_LIST: {
            int64_t path_ptr = args[0], path_len = args[1];
            int64_t out_buf = args[2], out_cap = args[3];
            if (path_ptr < 0 || path_len < 0 || path_len > PATH_CAP || out_buf < 0 || out_cap < 0) return -1;
            if (out_cap == 0) return 0;
            char *path;
            if (!tape_cstr(vm, path_ptr, path_len, &path)) return -1;
            DIR *d = opendir(path[0] ? path : ".");
            if (!d) { vm->host->realloc(path, 0); return -1; }
            /* Collect entries first so the listing matches
               `python3 -m http.server` (sorted case-insensitively). */
            char **names = NULL;
            bool *isdir = NULL;
            size_t nentries = 0, entries_cap = 0;
            struct dirent *ent;
            size_t orig_len = strlen(path);
            while ((ent = readdir(d)) != NULL) {
                const char *name = ent->d_name;
                if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;
                bool is_dir = false;
                {
                    size_t nl0 = strlen(name);
                    size_t full_len = orig_len + 1 + nl0;
                    char *full = malloc(full_len + 1);
                    if (full) {
                        if (orig_len == 0 || (orig_len == 1 && path[0] == '.')) {
                            memcpy(full, name, nl0);
                            full[nl0] = 0;
                        } else {
                            memcpy(full, path, orig_len);
                            full[orig_len] = '/';
                            memcpy(full + orig_len + 1, name, nl0);
                            full[full_len] = 0;
                        }
                        struct stat st2;
                        if (stat(full, &st2) == 0 && S_ISDIR(st2.st_mode)) is_dir = true;
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
            vm->host->realloc(path, 0);
            /* insertion sort by case-insensitive name; n is small so O(n^2) is fine. */
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
                if (written) { vm_mem_write(vm, out_buf + written, (int64_t)'\n'); written++; }
                for (size_t i = 0; i < nl; i++) {
                    vm_mem_write(vm, out_buf + written, (int64_t)(unsigned char)names[e][i]);
                    written++;
                }
                if (isdir[e]) { vm_mem_write(vm, out_buf + written, (int64_t)'/'); written++; }
            }
            for (size_t e = 0; e < nentries; e++) free(names[e]);
            free(names);
            free(isdir);
            return written;
        }
        case SYS_FILE_CREATE:
        case SYS_FILE_APPEND: {
            int64_t path_ptr = args[0], path_len = args[1];
            if (path_ptr < 0 || path_len < 0 || path_len > PATH_CAP) return -1;
            char *path;
            if (!tape_cstr(vm, path_ptr, path_len, &path)) return -1;
            int fd = (num == SYS_FILE_CREATE)
                ? open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644)
                : open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
            vm->host->realloc(path, 0);
            return fd;
        }
        case SYS_FILE_WRITE: {
            int64_t fd = args[0], buf = args[1], len = args[2];
            if (fd < 0 || buf < 0 || len < 0) return -1;
            if (len == 0) return 0;
            if (len > SEND_CAP) len = SEND_CAP;
            char *tmp = malloc((size_t)len);
            if (!tmp) { fprintf(stderr, "ink: fatal: out of memory\n"); exit(1); }
            for (int64_t i = 0; i < len; i++)
                tmp[i] = (char)(vm_mem_read(vm, buf + i) & 0xFF);
            ssize_t n = write((int)fd, tmp, (size_t)len);
            free(tmp);
            return (int64_t)n;
        }
        case SYS_SOCK_SEND: {
            int64_t fd = args[0], buf = args[1], len = args[2];
            if (fd < 0 || buf < 0 || len < 0) return -1;
            if (len == 0) return 0;
            if (len > SEND_CAP) len = SEND_CAP;
            char *tmp = malloc((size_t)len);
            if (!tmp) { fprintf(stderr, "ink: fatal: out of memory\n"); exit(1); }
            for (int64_t i = 0; i < len; i++)
                tmp[i] = (char)(vm_mem_read(vm, buf + i) & 0xFF);
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
            return (int64_t)total;
        }
        case SYS_SOCK_RECV: {
            int64_t fd = args[0], buf = args[1], len = args[2];
            if (fd < 0 || buf < 0 || len < 0) return -1;
            if (len == 0) return 0;
            if (len > RECV_CAP) len = RECV_CAP;
            char *tmp = malloc((size_t)len);
            if (!tmp) { fprintf(stderr, "ink: fatal: out of memory\n"); exit(1); }
            fflush(stdout);
            ssize_t n = recv((int)fd, tmp, (size_t)len, 0);
            if (n < 0) { free(tmp); return -1; }
            tape_read_into(vm, buf, tmp, (size_t)n);
            free(tmp);
            return (int64_t)n;
        }
        case SYS_SOCK_ACCEPT: {
            int64_t fd = args[0];
            if (fd < 0) return -1;
            fflush(stdout);
            int cfd = accept((int)fd, NULL, NULL);
            return cfd < 0 ? -1 : (int64_t)cfd;
        }
        case SYS_SLEEP: {
            int64_t ms = args[0];
            if (ms > 0) {
                fflush(stdout);
                struct timespec ts;
                ts.tv_sec = ms / 1000;
                ts.tv_nsec = (ms % 1000) * 1000000L;
                nanosleep(&ts, NULL);
            }
            return 0;
        }
        default:
            return -1; /* unknown syscall for the POSIX host */
    }
}

static int64_t posix_sys(InkVM *vm, int64_t num, int argc, const int64_t *args) {
    return ink_sys_posix(vm, num, argc, args);
}

const InkHost ink_host_posix = {
    .char_out  = posix_char_out,
    .char_in   = posix_char_in,
    .rand_seed = ink_rand_seed_posix,
    .fatal     = posix_fatal,
    .realloc   = posix_realloc,
    .suspend   = NULL,          /* desktop has no scheduler: yields are hard errors */
    .now_ms    = ink_now_ms_posix,
    .sys       = posix_sys,
};

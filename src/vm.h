#ifndef VM_H
#define VM_H

#include "ink.h"

/*
 * Instance-based VM. Every process is one InkVM; nothing global remains.
 * The core (vm.c) is freestanding: no stdio, no libc — all environment
 * interaction goes through vm->host. Desktop `ink run` uses ink_host_posix
 * (blocking, byte-identical historical behavior); schedulers/hosts supply
 * their own InkHost (see sched.c for the bare-metal shape of the story).
 */

/* lifecycle */
void    vm_init(InkVM *vm, const InkHost *host);
void    vm_destroy(InkVM *vm);
/* Takes ownership of `cps`, parses it into the VM's per-instance word list.
   On parse error sets outcome=OUT_ERROR with vm->errmsg. */
void    vm_load_program(InkVM *vm, cp_t *cps, size_t len);
/* Runs word 0 to a terminal outcome (OUT_HALT / OUT_ERROR) in the current
   context. Yield/wait points call host->suspend and transparently resume
   inside this call. */
Outcome vm_run(InkVM *vm);

/* helpers for hosts implementing custom syscalls (num > 17) */
int64_t vm_pop(InkVM *vm);
void    vm_push(InkVM *vm, int64_t v);
int64_t vm_mem_read(InkVM *vm, int64_t addr);
void    vm_mem_write(InkVM *vm, int64_t addr, int64_t v);
/* The core's own memory facilities (SYS_MEMSET / SYS_MEMCOPY, numbers 18/19).
   Everything else above 19 is host territory: dispatch to vm->host->sys. */
int64_t vm_sys_core(InkVM *vm, int64_t num, int argc, const int64_t *args);
/* Copy of a byte-string from the tape: one byte per cell, values >255 or <0
   become 0xFFFD. malloc'd via host->realloc-free malloc; NULL on bad range. */
char   *vm_mem_cstr(InkVM *vm, int64_t addr, int64_t len, bool *ok);

/* suspend points for host impls / kernels */
void    vm_suspend(InkVM *vm, Outcome reason);
void    vm_wait_fd(InkVM *vm, int fd, bool writable);
void    vm_wait_deadline(InkVM *vm, int64_t deadline_ms);

const char *op_name(OpCode op);

/* shared POSIX host pieces (vmhost.c) */
extern const InkHost ink_host_posix;      /* blocking desktop host           */
int64_t  ink_sys_posix(InkVM *vm, int64_t num, int argc, const int64_t *args);
uint64_t ink_rand_seed_posix(InkVM *vm);
int64_t  ink_now_ms_posix(InkVM *vm);

#endif

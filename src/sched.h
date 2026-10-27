#ifndef SCHED_H
#define SCHED_H

/*
 * sched.c — the desktop proof of the inkOS process model.
 *
 * Runs several programs as processes of one VM core: round-robin op-quantum
 * scheduler. Every process runs on its own ucontext so the interpreter's C
 * stack survives suspension: when accept/recv/char-in/sleep would block, the
 * non-blocking host parks the process (swapcontext out) instead of blocking
 * the machine, and execution resumes transparently inside the same host call
 * once the fd is readable or the deadline has passed.
 *
 * This is also, in miniature, the shape of the inkOS kernel: the exact same
 * core (vm.c), a host vtable where suspend = context switch, and syscalls
 * that park instead of block.
 */
int ink_sched_run(const char *const *paths, int npaths,
                  unsigned long quantum, unsigned long steps, int depth,
                  unsigned long long stack);

#endif

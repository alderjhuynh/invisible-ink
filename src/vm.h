#ifndef VM_H
#define VM_H

#include "ink.h"
#include <stdint.h>

extern unsigned long g_max_steps;
extern int g_max_depth;

extern Block **g_words;
extern size_t g_word_count;

extern int64_t *g_stack;
extern size_t g_sp, g_cap;

extern int64_t *g_mem;
extern size_t g_mem_cap;

extern unsigned long g_steps;

extern jmp_buf g_jmp;
extern Outcome g_outcome;
extern char g_errmsg[256];

void vm_reset(void);
void vm_reset_prng(void);
void exec_block(Block *blk, int depth);
const char *op_name(OpCode op);

#endif

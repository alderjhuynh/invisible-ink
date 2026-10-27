#ifndef PARSE_H
#define PARSE_H

#include "ink.h"

extern const char *g_src_name;

/* Parse errors never exit: they record state and unwind as NULL. The CLI
   prints them with the historical message shape; a kernel gets the raw
   message. */
extern char  g_parse_err[256];
extern size_t g_parse_pos;

Block *block_new(void);
void block_free(Block *b);
Instr *block_push(Block *b, OpCode op);

Block *parse_block(const cp_t *cps, size_t len, size_t *pos, bool stop_at_zwj);
cp_t *extract_program(const cp_t *full, size_t full_len, size_t *out_len, bool *embedded);
/* NULL on parse error (g_parse_err / g_parse_pos set). */
Block **split_and_parse_words(const cp_t *cps, size_t len, size_t *out_word_count);

#endif
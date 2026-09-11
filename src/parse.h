#ifndef PARSE_H
#define PARSE_H

#include "ink.h"

extern const char *g_src_name;

Block *block_new(void);
void block_free(Block *b);
Instr *block_push(Block *b, OpCode op);

Block *parse_block(const cp_t *cps, size_t len, size_t *pos, bool stop_at_zwj);
cp_t *extract_program(const cp_t *full, size_t full_len, size_t *out_len, bool *embedded);
Block **split_and_parse_words(const cp_t *cps, size_t len, size_t *out_word_count);

#endif

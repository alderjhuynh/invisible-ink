#ifndef QUILL_H
#define QUILL_H

#include "ink.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

extern char q_errmsg[1024];

typedef struct { cp_t *data; size_t count; size_t cap; } QCpVec;
void q_cv_init(QCpVec *v);
void q_cv_push(QCpVec *v, cp_t c);
void q_cv_free(QCpVec *v);

bool q_assemble(const char *source, size_t src_len, QCpVec *out_prog);
char *q_disassemble_bytes(const unsigned char *data, size_t data_len);
/* Disassemble already-parsed word lists (used for .inkb objects). */
char *q_disassemble_words(Block **blocks, size_t word_count);

/* decompile scc-shaped Quill to Scribe (NULL + q_errmsg on failure) */
char *sdec_decompile(const char *source, size_t src_len);

#endif

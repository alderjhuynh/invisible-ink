#ifndef UTIL_H
#define UTIL_H

#include "ink.h"
#include <stddef.h>

bool is_spec_cp(cp_t c);

/* UTF-8 helpers */
cp_t *utf8_decode(const unsigned char *buf, size_t len, size_t *out_count);
size_t utf8_encode_cp(cp_t cp, unsigned char *out);
void utf8_put(int64_t code);
int64_t utf8_get_stdin(void);

/* generic file helper */
unsigned char *read_file_raw(const char *path, size_t *out_len);

#endif

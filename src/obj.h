#ifndef OBJ_H
#define OBJ_H

#include "ink.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/*
 * .inkb — the compiled Ink object format (ISA v4.1).
 *
 * Invisible Ink text is hideously appropriate for prose, and hideous to boot
 * from: every boot re-runs a Unicode scanner and a tree builder, and any
 * text pipeline on the way (editors, mailers, NFKC normalization) can
 * mangle the program. A kernel should read one flat image and start.
 *
 * Layout (all integers little-endian, all counts word-addressable):
 *
 *   offset  size  field
 *   0       4     magic  "INKB"
 *   4       2     format version (1)
 *   6       2     ISA version this image targets (4)
 *   8       4     flags (0; reserved)
 *   12      8     word count           W
 *   20      8     memory image count   M
 *   28      16*M  memory image: per entry { u64 addr, i64 value }
 *   ..            words: W block trees, depth-first, in order:
 *                   per block:   u32 item_count
 *                   per item:    u8 opcode
 *                                i64 value (always present, e.g. PUSH)
 *                                if opcode is OP_IF or OP_LOOP, the two
 *                                child blocks follow: u8 has_else, then
 *                                then_block, then else_block (if present).
 *
 * The memory image is applied to the tape before word 0 runs (read-only data
 * section of the future: compilers may emit data here instead of generating
 * init words; the desktop toolchain today always emits a zero-entry image).
 */

#define INKOBJ_MAGIC_0 'I'
#define INKOBJ_MAGIC_1 'N'
#define INKOBJ_MAGIC_2 'K'
#define INKOBJ_MAGIC_3 'B'
#define INKOBJ_VERSION    1
#define INKOBJ_ISA        4

typedef struct {
    int64_t addr;
    int64_t value;
} ObjMemEntry;

/* true if the bytes look like an .inkb object (matches header exactly) */
bool inkobj_sniff(const uint8_t *data, size_t len);

/* Serialize a parsed word list (+ optional memory image).
   Returns malloc'd bytes; NULL + error message on failure. */
bool inkobj_save(const Block *const *words, size_t word_count,
                 const ObjMemEntry *mem, size_t mem_count,
                 uint8_t **out_data, size_t *out_len,
                 char *err, size_t errsz);

/*
 * Load the word list + memory image out of an object.
 *   out_words / out_word_count: parsed blocks (caller owns; block_free/free)
 *   out_mem / out_mem_count:    the image (caller owns; free)
 * Both out pointers are set even on the empty cases (words of count 0 fail).
 * NULL + err on malformed input.
 */
bool inkobj_load(const uint8_t *data, size_t len,
                 Block ***out_words, size_t *out_word_count,
                 ObjMemEntry **out_mem, size_t *out_mem_count,
                 char *err, size_t errsz);

#endif /* OBJ_H */

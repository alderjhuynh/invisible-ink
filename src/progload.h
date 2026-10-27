#ifndef PROGLOAD_H
#define PROGLOAD_H

/*
 * progload.c — load an Invisible Ink program from a file, in either of the
 * two forms the toolchain produces:
 *
 *   .inkb   compiled object (obj.c):
 *           one memmap + ready-to-run word trees; the memory image is
 *           applied to the tape before word 0; the cheapest thing a kernel
 *           or scheduler can boot.
 *   text    raw invisible codepoints with cover text allowed (FEFF-embedded
 *           aware), parsed on load.
 */

#include "obj.h"
#include <stddef.h>

typedef struct {
    /* exactly one of these representations is populated */
    enum { PROG_TEXT, PROG_OBJ } kind;
    cp_t    *cps;  size_t cps_len;    /* PROG_TEXT: owned once loaded into a VM */
    Block  **words; size_t word_count; /* PROG_OBJ: block trees */
    ObjMemEntry *mem; size_t mem_count;
    bool embedded;
    /* error messaging (errno-free, no strerror — freestanding friendly) */
    char err[256];
    size_t err_pos;   /* nonzero for text parse errors: codepoint position */
} InkProg;

/* Read + prepare. Returns false with p->err filled (parse/load/content). */
bool ink_prog_load_path(const char *path, InkProg *p);

/* Move a loaded program into a VM (takes ownership per the kind's contract).
   Returns false with errmsg filled. */
bool ink_prog_into_vm(InkProg *p, InkVM *vm);

/* Free whatever the loader still owns (safe on a program already moved). */
void ink_prog_free(InkProg *p);

#endif /* PROGLOAD_H */

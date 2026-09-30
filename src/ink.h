#ifndef INK_H
#define INK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <setjmp.h>

#define INK_VERSION "1.2.0"

typedef uint32_t cp_t;

/* spec codepoints */
enum {
    CP_ITIMES  = 0x2062,
    CP_NBSP    = 0x00A0,
    CP_FIGURE  = 0x2007,
    CP_THIN    = 0x2009,
    CP_HAIR    = 0x200A,
    CP_DIG0    = 0x2005,
    CP_DIG1    = 0x2006,
    CP_IDEO    = 0x3000,
    CP_PUNCT   = 0x2008,
    CP_MEDMATH = 0x205F,
    CP_EN      = 0x2002,
    CP_EM      = 0x2003,
    CP_THREE   = 0x2004,
    CP_ZWNJ    = 0x200C,
    CP_ZWJ     = 0x200D,
    CP_WJ      = 0x2060,
    CP_FA      = 0x2061,
    CP_LS      = 0x2000,
    CP_PS      = 0x2001,
    CP_ZWSP    = 0x200B,
    CP_BOM     = 0xFEFF,
    CP_INV_SEP = 0x2063,
    CP_INV_PLUS= 0x2064,
    CP_LRM     = 0x200E,
    CP_RLM     = 0x200F,
    CP_SOFT    = 0x00AD,
    /* syscall extensions: socket + file */
    CP_SOCK_CREATE = 0x2065,
    CP_SOCK_BIND   = 0x2066,
    CP_SOCK_LISTEN = 0x2067,
    CP_SOCK_ACCEPT = 0x2068,
    CP_SOCK_RECV   = 0x2069,
    CP_SOCK_SEND   = 0x206A,
    CP_SOCK_CLOSE  = 0x206B,
    CP_FILE_OPEN   = 0x206C,
    CP_FILE_READ   = 0x206D,
    CP_FILE_STAT   = 0x206E,
    CP_DIR_LIST    = 0x206F
};

/* opcode */
typedef enum {
    OP_PUSH, OP_DUP, OP_SWAP, OP_DROP, OP_ROT,
    OP_NEG, OP_SUB, OP_MOD, OP_ID, OP_ADD, OP_DIV, OP_SIGN, OP_MUL, OP_POW,
    OP_NUMOUT, OP_CHAROUT, OP_CHARIN,
    OP_CALL, OP_IF, OP_HALT,
    OP_RAND, OP_STORE, OP_LOAD, OP_SLEEP,
    OP_SOCK_CREATE, OP_SOCK_BIND, OP_SOCK_LISTEN, OP_SOCK_ACCEPT,
    OP_SOCK_RECV, OP_SOCK_SEND, OP_SOCK_CLOSE,
    OP_FILE_OPEN, OP_FILE_READ, OP_FILE_STAT, OP_DIR_LIST
} OpCode;

typedef struct Block Block;

typedef struct {
    OpCode op;
    int64_t value;
    Block *then_blk;
    Block *else_blk;
} Instr;

struct Block {
    Instr *items;
    size_t count;
    size_t cap;
};

#define DEFAULT_MAX_STEPS 10000000UL
#define DEFAULT_MAX_DEPTH 20000

typedef enum { OUT_NONE, OUT_HALT, OUT_ERROR } Outcome;

#endif /* INK_H */

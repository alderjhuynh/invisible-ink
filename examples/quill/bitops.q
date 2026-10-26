word _program
    ; and: 12 & 10 = 8
    push 12
    push 10
    and
    num-out
    push 32
    char-out
    ; or: 12 | 10 = 14
    push 12
    push 10
    or
    num-out
    push 32
    char-out
    ; xor: 12 ^ 10 = 6
    push 12
    push 10
    xor
    num-out
    push 32
    char-out
    ; and of disjoint bits = 1
    push 1
    push 3
    and
    num-out
    push 32
    char-out
    ; logical shrink of -1: (-1) shr 8 = 72057594037927935
    push -1
    push 8
    shr
    num-out
    push 32
    char-out
    ; shl: 1 << 4 = 16
    push 1
    push 4
    shl
    num-out
    push 32
    char-out
    ; shr on positive: 64 >> 3 = 8
    push 64
    push 3
    shr
    num-out
    push 32
    char-out
    ; shift counts are masked mod 64: 1 << 64 == 1 << 0 == 1
    push 1
    push 64
    shl
    num-out
    push 10
    char-out
    halt

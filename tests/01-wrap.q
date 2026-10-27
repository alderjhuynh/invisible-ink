; Arithmetic is *defined* to wrap, two's-complement 64-bit — the OS kernel
; depends on this never being undefined. Every previously-UB case must be
; a defined result: add/sub overflow, mul wrap, neg of INT64_MIN,
; div INT64_MIN / -1, mod INT64_MIN % -1, pow wrap, shift counts >= 64,
; and the `not` sugar (push 1; neg; xor).

word main
  ; INT64_MAX + 1 -> INT64_MIN
  push 9223372036854775807
  push 1
  add
  num-out
  push 10
  char-out
  ; INT64_MIN - 1 -> INT64_MAX
  push -9223372036854775808
  push 1
  sub
  num-out
  push 10
  char-out
  ; 2^32 * 2^32 wraps to 0
  push 4294967296
  push 4294967296
  mul
  num-out
  push 10
  char-out
  ; neg(INT64_MIN) -> INT64_MIN
  push -9223372036854775808
  neg
  num-out
  push 10
  char-out
  ; INT64_MIN / -1 -> INT64_MIN
  push -9223372036854775808
  push -1
  div
  num-out
  push 10
  char-out
  ; INT64_MIN % -1 -> 0
  push -9223372036854775808
  push -1
  mod
  num-out
  push 10
  char-out
  ; 2^65 wraps: low 64 bits are 0
  push 2
  push 65
  pow
  num-out
  push 10
  char-out
  ; 0^0 == 1
  push 0
  push 0
  pow
  num-out
  push 10
  char-out
  ; shifts mask the count: 1 << 64 -> 1
  push 1
  push 64
  shl
  num-out
  push 10
  char-out
  ; -1 >> 64 -> -1
  push -1
  push 64
  shr
  num-out
  push 10
  char-out
  ; logical not sugar
  push 12
  not
  num-out
  push 10
  char-out

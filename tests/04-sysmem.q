; Core-ABI syscalls 18 (mem-set) and 19 (mem-copy, memmove semantics):
; fill, copy forward, copy *overlapping* (dst inside src), and reaching
; beyond the previously-touched tape must extend it.

word main
  ; fill addr 100..104 with 'A' (65)
  push 100
  push 65
  push 5
  sys 18
  ; copy 100..104 -> 200..204
  push 200
  push 100
  push 5
  sys 19
  push 200
  load
  num-out
  push 10
  char-out
  push 202
  load
  num-out
  push 10
  char-out
  push 204
  load
  num-out
  push 10
  char-out
  ; overlapping: mem 300..304 = 1,2,3,4,5; copy 301..305 -> 300..304
  ; (dst inside src; snapshot semantics: 2,3,4,5, then the old 305 which is 0)
  push 300
  push 1
  push 5
  sys 18
  push 301
  push 2
  push 5
  sys 18
  push 302
  push 3
  push 5
  sys 18
  push 303
  push 4
  push 5
  sys 18
  push 304
  push 5
  push 5
  sys 18
  push 300
  push 301
  push 5
  sys 19
  push 300
  load
  num-out
  push 10
  char-out
  push 301
  load
  num-out
  push 10
  char-out
  push 302
  load
  num-out
  push 10
  char-out
  push 303
  load
  num-out
  push 10
  char-out
  push 304
  load
  num-out
  push 10
  char-out

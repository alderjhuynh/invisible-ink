; .inkb round-trip: this same output must come out of
; 1) the raw text, 2) `ink obj` + run, 3) disassemble(obj) -> assemble -> run
word main
  push 1234
  num-out
  push 10
  char-out
  push -55
  num-out
  push 10
  char-out
  push -1
  push 1
  xor
  num-out
  push 10
  char-out

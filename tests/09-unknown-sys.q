; Unknown syscall numbers: hosts return -1 and nothing else may corrupt.
word main
  push 999
  sys
  num-out
  push 10
  char-out
  push 0
  sys
  num-out
  push 10
  char-out
  push -5
  sys
  num-out
  push 10
  char-out

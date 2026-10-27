; One deterministic program run under the scheduler — direct run and sched
; must produce byte-identical output; also runs as a .inkb.
word main
  push 83
  char-out
  push 10
  char-out
  push 11
  push 22
  add
  num-out
  push 10
  char-out

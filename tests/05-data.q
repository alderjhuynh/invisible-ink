; Quill `data`: integers and strings (one cell per byte, one zero terminator
; per string). `push <name>` gives the base address; cells address like any
; other tape address.

data hello = "Hi!\n"
data table = 7, 9, 42

word main
  ; hello is 'H','i','!','\n' then the implied 0 terminator cell
  push hello
  load
  char-out
  push hello
  push 1
  add
  load
  char-out
  push hello
  push 2
  add
  load
  char-out
  push hello
  push 3
  add
  load
  char-out
  push hello
  push 4
  add
  load
  num-out
  push 10
  char-out
  ; table is plain ints, no terminator
  push table
  load
  num-out
  push 10
  char-out
  push table
  push 1
  add
  load
  num-out
  push 10
  char-out
  push table
  push 2
  add
  load
  num-out
  push 10
  char-out
  ; one past the end: the tape is zero-initialised
  push table
  push 3
  add
  load
  num-out
  push 10
  char-out

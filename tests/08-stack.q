; stack-limit enforcement: build a runaway data stack under `--stack 16`
; and the VM must fault cleanly (exit 1), not malloc forever.
word main
  push 0
  while {
    push 1
  } {
    push 1
  }

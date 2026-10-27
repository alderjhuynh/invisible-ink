; `pow` bills one step per multiply, so a huge exponent trips the step
; limit instead of hanging (it used to be one billless op). Exit MUST be 1
; with a step-limit error — this is a run-forever regression test.
word main
  push 2
  push 9223372036854775807
  pow
  num-out

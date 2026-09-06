word main
    push 5
    call countdown
    halt
word countdown
    dup
    if {
        dup
        num-out
        push 10
        char-out
        push 1
        sub
        call countdown
    } else {
        drop
    }

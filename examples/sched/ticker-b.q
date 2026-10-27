word main
    push 66
    char-out
    push 32
    char-out
    push 4
    call tick
    push 10
    char-out
    halt
word tick
    dup
    if {
        dup
        num-out
        push 10
        char-out
        push 100
        sleep
        push 1
        sub
        call tick
    } else {
        drop
    }

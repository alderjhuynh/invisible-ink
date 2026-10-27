; echoes one character per line until EOF - for ink sched (parks on stdin)
word main
    char-in
    dup
    push 1
    add
    if {
        char-out
        push 10
        char-out
        call main
    } else {
        drop
    }

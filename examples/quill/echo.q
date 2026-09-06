word main
    char-in
    dup
    push -1
    sub
    if {
        dup
        char-out
        call main
    } else {
        drop
    }
    halt

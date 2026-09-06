; Fibonacci, printed up to n terms.
word main
    let n = 10
    let a = 0
    let b = 1
    load n
    call loop
    halt

word loop ; ( count -- )
    dup
    if {
        load a
        num-out
        push 10
        char-out
        load a
        load b
        add
        load b
        store a
        store b
        push 1
        sub
        call loop
    } else {
        drop
    }

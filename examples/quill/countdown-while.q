word main
    let i = 0
    push 5
    store i
    while {
        load i          ; condition: truthy until i reaches 0
    } {
        load i
        num-out
        push 10
        char-out
        load i
        push 1
        sub
        store i         ; i = i - 1
    }

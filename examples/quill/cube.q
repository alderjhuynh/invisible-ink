word _program
    ; scc program entry
    let SP = 2025;
    let FP = 0;
    push 2019
    push 0
    store
    push 2020
    push 0
    store
    push 2021
    push 0
    store
    push 2022
    push 0
    store
    push 2023
    push 0
    store
    push 2024
    push 0
    store
    call fn_main
    drop
    halt

word fn_tsin
    ; fn tsin (1 args)
    load SP
    push 7
    add
    store SP
    load FP
    load SP
    push 7
    sub
    swap
    store
    load SP
    push 7
    sub
    store FP
    load SP
    push 5
    sub
    push 0
    store
    load SP
    push 4
    sub
    push 0
    store
    load SP
    push 3
    sub
    push 0
    store
    load SP
    push 2
    sub
    push 0
    store
    load SP
    push 1
    sub
    push 0
    store
    load SP
    push 6
    sub
    swap
    store
    load FP
    push 2
    add
    load FP
    push 1
    add
    load
    push 32
    div
    store
    load FP
    push 3
    add
    load FP
    push 1
    add
    load
    load FP
    push 2
    add
    load
    push 32
    mul
    sub
    store
    load FP
    push 4
    add
    push 0
    store
    load FP
    push 2
    add
    load
    push 0
    sub
    sign
    dup
    mul
    push 1
    swap
    sub
    if {
    load FP
    push 4
    add
    push 2
    load FP
    push 3
    add
    load
    add
    load
    store
    }
    load FP
    push 2
    add
    load
    push 1
    sub
    sign
    dup
    mul
    push 1
    swap
    sub
    if {
    load FP
    push 4
    add
    push 2
    push 32
    load FP
    push 3
    add
    load
    sub
    add
    load
    store
    }
    load FP
    push 2
    add
    load
    push 2
    sub
    sign
    dup
    mul
    push 1
    swap
    sub
    if {
    load FP
    push 4
    add
    push 0
    push 2
    load FP
    push 3
    add
    load
    add
    load
    sub
    store
    }
    load FP
    push 2
    add
    load
    push 3
    sub
    sign
    dup
    mul
    push 1
    swap
    sub
    if {
    load FP
    push 4
    add
    push 0
    push 2
    push 32
    load FP
    push 3
    add
    load
    sub
    add
    load
    sub
    store
    }
    load FP
    push 5
    add
    load FP
    push 4
    add
    load
    store
    load FP
    push 6
    add
    push 1
    store
    load SP
    push 7
    sub
    store SP
    load SP
    load
    store FP
    load SP
    push 5
    add
    load


word fn_tcos
    ; fn tcos (1 args)
    load SP
    push 7
    add
    store SP
    load FP
    load SP
    push 7
    sub
    swap
    store
    load SP
    push 7
    sub
    store FP
    load SP
    push 5
    sub
    push 0
    store
    load SP
    push 4
    sub
    push 0
    store
    load SP
    push 3
    sub
    push 0
    store
    load SP
    push 2
    sub
    push 0
    store
    load SP
    push 1
    sub
    push 0
    store
    load SP
    push 6
    sub
    swap
    store
    load FP
    push 2
    add
    load FP
    push 1
    add
    load
    push 32
    div
    store
    load FP
    push 3
    add
    load FP
    push 1
    add
    load
    load FP
    push 2
    add
    load
    push 32
    mul
    sub
    store
    load FP
    push 4
    add
    push 0
    store
    load FP
    push 2
    add
    load
    push 0
    sub
    sign
    dup
    mul
    push 1
    swap
    sub
    if {
    load FP
    push 4
    add
    push 2
    push 32
    load FP
    push 3
    add
    load
    sub
    add
    load
    store
    }
    load FP
    push 2
    add
    load
    push 1
    sub
    sign
    dup
    mul
    push 1
    swap
    sub
    if {
    load FP
    push 4
    add
    push 0
    push 2
    load FP
    push 3
    add
    load
    add
    load
    sub
    store
    }
    load FP
    push 2
    add
    load
    push 2
    sub
    sign
    dup
    mul
    push 1
    swap
    sub
    if {
    load FP
    push 4
    add
    push 0
    push 2
    push 32
    load FP
    push 3
    add
    load
    sub
    add
    load
    sub
    store
    }
    load FP
    push 2
    add
    load
    push 3
    sub
    sign
    dup
    mul
    push 1
    swap
    sub
    if {
    load FP
    push 4
    add
    push 2
    load FP
    push 3
    add
    load
    add
    load
    store
    }
    load FP
    push 5
    add
    load FP
    push 4
    add
    load
    store
    load FP
    push 6
    add
    push 1
    store
    load SP
    push 7
    sub
    store SP
    load SP
    load
    store FP
    load SP
    push 5
    add
    load


word fn_rotproj
    ; fn rotproj (1 args)
    load SP
    push 12
    add
    store SP
    load FP
    load SP
    push 12
    sub
    swap
    store
    load SP
    push 12
    sub
    store FP
    load SP
    push 10
    sub
    push 0
    store
    load SP
    push 9
    sub
    push 0
    store
    load SP
    push 8
    sub
    push 0
    store
    load SP
    push 7
    sub
    push 0
    store
    load SP
    push 6
    sub
    push 0
    store
    load SP
    push 5
    sub
    push 0
    store
    load SP
    push 4
    sub
    push 0
    store
    load SP
    push 3
    sub
    push 0
    store
    load SP
    push 2
    sub
    push 0
    store
    load SP
    push 1
    sub
    push 0
    store
    load SP
    push 11
    sub
    swap
    store
    load FP
    push 2
    add
    push 35
    load FP
    push 1
    add
    load
    push 3
    mul
    add
    load
    store
    load FP
    push 3
    add
    push 35
    load FP
    push 1
    add
    load
    push 3
    mul
    push 1
    add
    add
    load
    store
    load FP
    push 4
    add
    push 35
    load FP
    push 1
    add
    load
    push 3
    mul
    push 2
    add
    add
    load
    store
    load FP
    push 5
    add
    load FP
    push 2
    add
    load
    push 2022
    load
    mul
    load FP
    push 4
    add
    load
    push 2021
    load
    mul
    add
    push 1024
    div
    store
    load FP
    push 6
    add
    load FP
    push 4
    add
    load
    push 2022
    load
    mul
    load FP
    push 2
    add
    load
    push 2021
    load
    mul
    sub
    push 1024
    div
    store
    load FP
    push 7
    add
    load FP
    push 3
    add
    load
    push 2024
    load
    mul
    load FP
    push 6
    add
    load
    push 2023
    load
    mul
    sub
    push 1024
    div
    store
    load FP
    push 8
    add
    load FP
    push 3
    add
    load
    push 2023
    load
    mul
    load FP
    push 6
    add
    load
    push 2024
    load
    mul
    add
    push 1024
    div
    store
    load FP
    push 9
    add
    load FP
    push 8
    add
    load
    push 4096
    add
    store
    push 2003
    load FP
    push 1
    add
    load
    add
    push 40
    load FP
    push 5
    add
    load
    push 80
    mul
    load FP
    push 9
    add
    load
    div
    add
    store
    push 2011
    load FP
    push 1
    add
    load
    add
    push 12
    load FP
    push 7
    add
    load
    push 80
    mul
    load FP
    push 9
    add
    load
    div
    push 2
    div
    add
    store
    load FP
    push 10
    add
    push 0
    store
    load FP
    push 11
    add
    push 1
    store
    load SP
    push 12
    sub
    store SP
    load SP
    load
    store FP
    load SP
    push 10
    add
    load


word fn_line
    ; fn line (4 args)
    load SP
    push 15
    add
    store SP
    load FP
    load SP
    push 15
    sub
    swap
    store
    load SP
    push 15
    sub
    store FP
    load SP
    push 10
    sub
    push 0
    store
    load SP
    push 9
    sub
    push 0
    store
    load SP
    push 8
    sub
    push 0
    store
    load SP
    push 7
    sub
    push 0
    store
    load SP
    push 6
    sub
    push 0
    store
    load SP
    push 5
    sub
    push 0
    store
    load SP
    push 4
    sub
    push 0
    store
    load SP
    push 3
    sub
    push 0
    store
    load SP
    push 2
    sub
    push 0
    store
    load SP
    push 1
    sub
    push 0
    store
    load SP
    push 11
    sub
    swap
    store
    load SP
    push 12
    sub
    swap
    store
    load SP
    push 13
    sub
    swap
    store
    load SP
    push 14
    sub
    swap
    store
    load FP
    push 5
    add
    load FP
    push 3
    add
    load
    load FP
    push 1
    add
    load
    sub
    store
    load FP
    push 6
    add
    push 1
    store
    load FP
    push 5
    add
    load
    push 0
    sub
    sign
    push 1
    swap
    sub
    push 1
    shr
    if {
    load FP
    push 6
    add
    push 0
    push 1
    sub
    store
    load FP
    push 5
    add
    push 0
    load FP
    push 5
    add
    load
    sub
    store
    }
    load FP
    push 7
    add
    load FP
    push 4
    add
    load
    load FP
    push 2
    add
    load
    sub
    store
    load FP
    push 8
    add
    push 1
    store
    load FP
    push 7
    add
    load
    push 0
    sub
    sign
    push 1
    swap
    sub
    push 1
    shr
    if {
    load FP
    push 8
    add
    push 0
    push 1
    sub
    store
    load FP
    push 7
    add
    push 0
    load FP
    push 7
    add
    load
    sub
    store
    }
    load FP
    push 9
    add
    load FP
    push 5
    add
    load
    load FP
    push 7
    add
    load
    sub
    store
    load FP
    push 10
    add
    load FP
    push 1
    add
    load
    store
    load FP
    push 11
    add
    load FP
    push 2
    add
    load
    store
    call fn_line__w0
    load FP
    push 3
    add
    load
    push 0
    sub
    sign
    push 1
    swap
    sub
    push 1
    shr
    push 1
    xor
    if {
    load FP
    push 3
    add
    load
    push 80
    sub
    sign
    push 1
    swap
    sub
    push 1
    shr
    if {
    load FP
    push 4
    add
    load
    push 0
    sub
    sign
    push 1
    swap
    sub
    push 1
    shr
    push 1
    xor
    if {
    load FP
    push 4
    add
    load
    push 24
    sub
    sign
    push 1
    swap
    sub
    push 1
    shr
    if {
    push 83
    load FP
    push 4
    add
    load
    push 80
    mul
    load FP
    push 3
    add
    load
    add
    add
    push 35
    store
    }
    }
    }
    }
    load FP
    push 13
    add
    push 0
    store
    load FP
    push 14
    add
    push 1
    store
    load SP
    push 15
    sub
    store SP
    load SP
    load
    store FP
    load SP
    push 13
    add
    load


word fn_main
    ; fn main (0 args)
    load SP
    push 10
    add
    store SP
    load FP
    load SP
    push 10
    sub
    swap
    store
    load SP
    push 10
    sub
    store FP
    load SP
    push 9
    sub
    push 0
    store
    load SP
    push 8
    sub
    push 0
    store
    load SP
    push 7
    sub
    push 0
    store
    load SP
    push 6
    sub
    push 0
    store
    load SP
    push 5
    sub
    push 0
    store
    load SP
    push 4
    sub
    push 0
    store
    load SP
    push 3
    sub
    push 0
    store
    load SP
    push 2
    sub
    push 0
    store
    load SP
    push 1
    sub
    push 0
    store
    load FP
    push 1
    add
    push 0
    store
    load FP
    push 2
    add
    push 0
    store
    push 2
    push 0
    add
    push 0
    store
    push 2
    push 1
    add
    push 50
    store
    push 2
    push 2
    add
    push 100
    store
    push 2
    push 3
    add
    push 150
    store
    push 2
    push 4
    add
    push 200
    store
    push 2
    push 5
    add
    push 249
    store
    push 2
    push 6
    add
    push 297
    store
    push 2
    push 7
    add
    push 345
    store
    push 2
    push 8
    add
    push 392
    store
    push 2
    push 9
    add
    push 438
    store
    push 2
    push 10
    add
    push 483
    store
    push 2
    push 11
    add
    push 526
    store
    push 2
    push 12
    add
    push 569
    store
    push 2
    push 13
    add
    push 610
    store
    push 2
    push 14
    add
    push 650
    store
    push 2
    push 15
    add
    push 688
    store
    push 2
    push 16
    add
    push 724
    store
    push 2
    push 17
    add
    push 759
    store
    push 2
    push 18
    add
    push 792
    store
    push 2
    push 19
    add
    push 822
    store
    push 2
    push 20
    add
    push 851
    store
    push 2
    push 21
    add
    push 878
    store
    push 2
    push 22
    add
    push 903
    store
    push 2
    push 23
    add
    push 926
    store
    push 2
    push 24
    add
    push 946
    store
    push 2
    push 25
    add
    push 964
    store
    push 2
    push 26
    add
    push 980
    store
    push 2
    push 27
    add
    push 993
    store
    push 2
    push 28
    add
    push 1004
    store
    push 2
    push 29
    add
    push 1013
    store
    push 2
    push 30
    add
    push 1019
    store
    push 2
    push 31
    add
    push 1023
    store
    push 2
    push 32
    add
    push 1024
    store
    load FP
    push 1
    add
    push 0
    store
    call fn_main__w0
    load FP
    push 2
    add
    push 0
    store
    load FP
    push 1
    add
    push 0
    store
    call fn_main__w1
    load FP
    push 5
    add
    push 0
    store
    call fn_main__w3
    load FP
    push 8
    add
    push 0
    store
    load FP
    push 9
    add
    push 1
    store
    load SP
    push 10
    sub
    store SP
    load SP
    load
    store FP
    load SP
    push 8
    add
    load


word fn_line__w0
    load FP
    push 14
    add
    load
    push 1
    sub
    if {
    load FP
    push 10
    add
    load
    load FP
    push 3
    add
    load
    sub
    sign
    dup
    mul
    load FP
    push 11
    add
    load
    load FP
    push 4
    add
    load
    sub
    sign
    dup
    mul
    swap
    sign
    dup
    mul
    swap
    sign
    dup
    mul
    or
    if {
    load FP
    push 10
    add
    load
    push 0
    sub
    sign
    push 1
    swap
    sub
    push 1
    shr
    push 1
    xor
    if {
    load FP
    push 10
    add
    load
    push 80
    sub
    sign
    push 1
    swap
    sub
    push 1
    shr
    if {
    load FP
    push 11
    add
    load
    push 0
    sub
    sign
    push 1
    swap
    sub
    push 1
    shr
    push 1
    xor
    if {
    load FP
    push 11
    add
    load
    push 24
    sub
    sign
    push 1
    swap
    sub
    push 1
    shr
    if {
    push 83
    load FP
    push 11
    add
    load
    push 80
    mul
    load FP
    push 10
    add
    load
    add
    add
    push 35
    store
    }
    }
    }
    }
    load FP
    push 12
    add
    load FP
    push 9
    add
    load
    push 2
    mul
    store
    load FP
    push 12
    add
    load
    push 0
    load FP
    push 7
    add
    load
    sub
    sub
    sign
    push 1
    add
    push 1
    shr
    if {
    load FP
    push 9
    add
    load FP
    push 9
    add
    load
    load FP
    push 7
    add
    load
    sub
    store
    load FP
    push 10
    add
    load FP
    push 10
    add
    load
    load FP
    push 6
    add
    load
    add
    store
    }
    load FP
    push 12
    add
    load
    load FP
    push 5
    add
    load
    sub
    sign
    push 1
    swap
    sub
    push 1
    shr
    if {
    load FP
    push 9
    add
    load FP
    push 9
    add
    load
    load FP
    push 5
    add
    load
    add
    store
    load FP
    push 11
    add
    load FP
    push 11
    add
    load
    load FP
    push 8
    add
    load
    add
    store
    }
    call fn_line__w0
    }
    }

word fn_main__w0
    load FP
    push 9
    add
    load
    push 1
    sub
    if {
    load FP
    push 1
    add
    load
    push 8
    sub
    sign
    push 1
    swap
    sub
    push 1
    shr
    if {
    push 35
    load FP
    push 1
    add
    load
    push 3
    mul
    add
    load FP
    push 1
    add
    load
    push 2
    mod
    push 1024
    mul
    push 512
    sub
    store
    push 35
    load FP
    push 1
    add
    load
    push 3
    mul
    push 1
    add
    add
    load FP
    push 1
    add
    load
    push 2
    div
    push 2
    mod
    push 1024
    mul
    push 512
    sub
    store
    push 35
    load FP
    push 1
    add
    load
    push 3
    mul
    push 2
    add
    add
    load FP
    push 1
    add
    load
    push 4
    div
    push 1024
    mul
    push 512
    sub
    store
    load FP
    push 1
    add
    load FP
    push 1
    add
    load
    push 1
    add
    store
    call fn_main__w0
    }
    }

word fn_main__w2
    load FP
    push 9
    add
    load
    push 1
    sub
    if {
    load FP
    push 3
    add
    load
    push 8
    sub
    sign
    push 1
    swap
    sub
    push 1
    shr
    if {
    load FP
    push 4
    add
    load FP
    push 1
    add
    load
    load FP
    push 3
    add
    load
    xor
    store
    load FP
    push 4
    add
    load
    load FP
    push 1
    add
    load
    sub
    sign
    push 1
    add
    push 1
    shr
    if {
    push 59
    load FP
    push 2
    add
    load
    push 2
    mul
    add
    load FP
    push 1
    add
    load
    store
    push 59
    load FP
    push 2
    add
    load
    push 2
    mul
    push 1
    add
    add
    load FP
    push 4
    add
    load
    store
    load FP
    push 2
    add
    load FP
    push 2
    add
    load
    push 1
    add
    store
    }
    load FP
    push 3
    add
    load FP
    push 3
    add
    load
    push 2
    mul
    store
    call fn_main__w2
    }
    }

word fn_main__w1
    load FP
    push 9
    add
    load
    push 1
    sub
    if {
    load FP
    push 1
    add
    load
    push 8
    sub
    sign
    push 1
    swap
    sub
    push 1
    shr
    if {
    load FP
    push 3
    add
    push 1
    store
    call fn_main__w2
    load FP
    push 1
    add
    load FP
    push 1
    add
    load
    push 1
    add
    store
    call fn_main__w1
    }
    }

word fn_main__w4
    load FP
    push 9
    add
    load
    push 1
    sub
    if {
    load FP
    push 1
    add
    load
    push 1920
    sub
    sign
    push 1
    swap
    sub
    push 1
    shr
    if {
    push 83
    load FP
    push 1
    add
    load
    add
    push 32
    store
    load FP
    push 1
    add
    load FP
    push 1
    add
    load
    push 1
    add
    store
    call fn_main__w4
    }
    }

word fn_main__w5
    load FP
    push 9
    add
    load
    push 1
    sub
    if {
    load FP
    push 1
    add
    load
    push 8
    sub
    sign
    push 1
    swap
    sub
    push 1
    shr
    if {
    load FP
    push 1
    add
    load
    call fn_rotproj
    drop
    load FP
    push 1
    add
    load FP
    push 1
    add
    load
    push 1
    add
    store
    call fn_main__w5
    }
    }

word fn_main__w6
    load FP
    push 9
    add
    load
    push 1
    sub
    if {
    load FP
    push 2
    add
    load
    push 12
    sub
    sign
    push 1
    swap
    sub
    push 1
    shr
    if {
    push 2003
    push 59
    load FP
    push 2
    add
    load
    push 2
    mul
    add
    load
    add
    load
    push 2011
    push 59
    load FP
    push 2
    add
    load
    push 2
    mul
    add
    load
    add
    load
    push 2003
    push 59
    load FP
    push 2
    add
    load
    push 2
    mul
    push 1
    add
    add
    load
    add
    load
    push 2011
    push 59
    load FP
    push 2
    add
    load
    push 2
    mul
    push 1
    add
    add
    load
    add
    load
    call fn_line
    drop
    load FP
    push 2
    add
    load FP
    push 2
    add
    load
    push 1
    add
    store
    call fn_main__w6
    }
    }

word fn_main__w8
    load FP
    push 9
    add
    load
    push 1
    sub
    if {
    load FP
    push 7
    add
    load
    push 80
    sub
    sign
    push 1
    swap
    sub
    push 1
    shr
    if {
    push 83
    load FP
    push 6
    add
    load
    push 80
    mul
    load FP
    push 7
    add
    load
    add
    add
    load
    char-out
    load FP
    push 7
    add
    load FP
    push 7
    add
    load
    push 1
    add
    store
    call fn_main__w8
    }
    }

word fn_main__w7
    load FP
    push 9
    add
    load
    push 1
    sub
    if {
    load FP
    push 6
    add
    load
    push 24
    sub
    sign
    push 1
    swap
    sub
    push 1
    shr
    if {
    load FP
    push 7
    add
    push 0
    store
    call fn_main__w8
    push 10
    char-out
    load FP
    push 6
    add
    load FP
    push 6
    add
    load
    push 1
    add
    store
    call fn_main__w7
    }
    }

word fn_main__w3
    load FP
    push 9
    add
    load
    push 1
    sub
    if {
    push 1
    if {
    push 2019
    load FP
    push 5
    add
    load
    push 127
    and
    store
    push 2020
    load FP
    push 5
    add
    load
    push 3
    mul
    push 127
    and
    store
    push 2021
    push 2019
    load
    call fn_tsin
    store
    push 2022
    push 2019
    load
    call fn_tcos
    store
    push 2023
    push 2020
    load
    call fn_tsin
    store
    push 2024
    push 2020
    load
    call fn_tcos
    store
    load FP
    push 1
    add
    push 0
    store
    call fn_main__w4
    load FP
    push 1
    add
    push 0
    store
    call fn_main__w5
    load FP
    push 2
    add
    push 0
    store
    call fn_main__w6
    load FP
    push 6
    add
    push 0
    store
    call fn_main__w7
    push 50
    sleep
    load FP
    push 5
    add
    load FP
    push 5
    add
    load
    push 1
    add
    store
    call fn_main__w3
    }
    }

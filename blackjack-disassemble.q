word word_0
    push 100
    push 100
    store
    push 101
    push 10
    store
    call 29
    call 30

word word_1
    dup
    push 115
    swap
    store
    dup
    mul
    sign
    push 1
    swap
    sub
    if {
        push 11
    } else {
        push 115
        load
        dup
        push 115
        swap
        store
        push 10
        swap
        sub
        sign
        push 1
        sub
        dup
        mul
        sign
        push 1
        swap
        sub
        if {
            push 115
            load
            push 1
            add
        } else {
            push 10
        }
    }

word word_2
    dup
    push 52
    swap
    sub
    sign
    push 1
    sub
    dup
    mul
    sign
    push 1
    swap
    sub
    if {
        dup
        push 110
        swap
        store
        dup
        store
        push 110
        load
        push 1
        add
        call 2
    } else {
        drop
    }

word word_3
    push 0
    call 2
    push 52
    push 52
    store
    push 71
    push 0
    store
    push 91
    push 0
    store
    push 51
    call 4

word word_4
    dup
    sign
    push 1
    sub
    dup
    mul
    sign
    push 1
    swap
    sub
    if {
        dup
        push 110
        swap
        store
        dup
        push 1
        add
        rand
        dup
        push 111
        swap
        store
        swap
        dup
        push 110
        swap
        store
        drop
        drop
        push 110
        load
        load
        dup
        push 112
        swap
        store
        push 111
        load
        load
        dup
        push 113
        swap
        store
        drop
        drop
        push 110
        load
        push 113
        load
        store
        push 111
        load
        push 112
        load
        store
        push 110
        load
        push 1
        sub
        call 4
    } else {
        drop
    }

word word_5
    push 52
    load
    push 1
    sub
    dup
    push 110
    swap
    store
    load
    dup
    push 111
    swap
    store
    push 71
    load
    dup
    push 112
    swap
    store
    push 60
    swap
    add
    swap
    store
    push 112
    load
    push 1
    add
    push 71
    swap
    store
    push 110
    load
    push 52
    swap
    store

word word_6
    push 52
    load
    push 1
    sub
    dup
    push 110
    swap
    store
    load
    dup
    push 111
    swap
    store
    push 91
    load
    dup
    push 112
    swap
    store
    push 80
    swap
    add
    swap
    store
    push 112
    load
    push 1
    add
    push 91
    swap
    store
    push 110
    load
    push 52
    swap
    store

word word_7
    dup
    push 115
    swap
    store
    push 0
    sub
    dup
    mul
    sign
    push 1
    swap
    sub
    if {
        push 65
        char-out
    } else {
        push 115
        load
        push 1
        sub
        dup
        mul
        sign
        push 1
        swap
        sub
        if {
            push 50
            char-out
        } else {
            push 115
            load
            push 2
            sub
            dup
            mul
            sign
            push 1
            swap
            sub
            if {
                push 51
                char-out
            } else {
                push 115
                load
                push 3
                sub
                dup
                mul
                sign
                push 1
                swap
                sub
                if {
                    push 52
                    char-out
                } else {
                    push 115
                    load
                    push 4
                    sub
                    dup
                    mul
                    sign
                    push 1
                    swap
                    sub
                    if {
                        push 53
                        char-out
                    } else {
                        push 115
                        load
                        push 5
                        sub
                        dup
                        mul
                        sign
                        push 1
                        swap
                        sub
                        if {
                            push 54
                            char-out
                        } else {
                            push 115
                            load
                            push 6
                            sub
                            dup
                            mul
                            sign
                            push 1
                            swap
                            sub
                            if {
                                push 55
                                char-out
                            } else {
                                push 115
                                load
                                push 7
                                sub
                                dup
                                mul
                                sign
                                push 1
                                swap
                                sub
                                if {
                                    push 56
                                    char-out
                                } else {
                                    push 115
                                    load
                                    push 8
                                    sub
                                    dup
                                    mul
                                    sign
                                    push 1
                                    swap
                                    sub
                                    if {
                                        push 57
                                        char-out
                                    } else {
                                        push 115
                                        load
                                        push 9
                                        sub
                                        dup
                                        mul
                                        sign
                                        push 1
                                        swap
                                        sub
                                        if {
                                            push 49
                                            char-out
                                            push 48
                                            char-out
                                        } else {
                                            push 115
                                            load
                                            push 10
                                            sub
                                            dup
                                            mul
                                            sign
                                            push 1
                                            swap
                                            sub
                                            if {
                                                push 74
                                                char-out
                                            } else {
                                                push 115
                                                load
                                                push 11
                                                sub
                                                dup
                                                mul
                                                sign
                                                push 1
                                                swap
                                                sub
                                                if {
                                                    push 81
                                                    char-out
                                                } else {
                                                    push 75
                                                    char-out
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

word word_8
    dup
    push 115
    swap
    store
    push 0
    sub
    dup
    mul
    sign
    push 1
    swap
    sub
    if {
        push 9824
        char-out
    } else {
        push 115
        load
        push 1
        sub
        dup
        mul
        sign
        push 1
        swap
        sub
        if {
            push 9829
            char-out
        } else {
            push 115
            load
            push 2
            sub
            dup
            mul
            sign
            push 1
            swap
            sub
            if {
                push 9830
                char-out
            } else {
                push 9827
                char-out
            }
        }
    }

word word_9
    dup
    push 110
    swap
    store
    push 110
    load
    push 4
    div
    call 7
    push 110
    load
    push 4
    mod
    call 8
    push 32
    char-out

word word_10
    push 122
    load
    push 123
    load
    swap
    sub
    sign
    push 1
    sub
    dup
    mul
    sign
    push 1
    swap
    sub
    if {
        push 122
        load
        push 60
        swap
        add
        load
        dup
        push 110
        swap
        store
        push 110
        load
        push 4
        div
        dup
        push 115
        swap
        store
        call 1
        push 120
        load
        add
        push 120
        swap
        store
        push 115
        load
        push 0
        sub
        dup
        mul
        sign
        push 1
        swap
        sub
        if {
            push 121
            load
            push 1
            add
            push 121
            swap
            store
        } else {
        }
        push 122
        load
        push 1
        add
        push 122
        swap
        store
        call 10
    } else {
    }

word word_11
    push 120
    load
    push 21
    swap
    sub
    sign
    push 1
    sub
    dup
    mul
    sign
    push 1
    swap
    sub
    if {
        push 121
        load
        sign
        push 1
        sub
        dup
        mul
        sign
        push 1
        swap
        sub
        if {
            push 120
            load
            push 10
            sub
            push 120
            swap
            store
            push 121
            load
            push 1
            sub
            push 121
            swap
            store
            call 11
        } else {
        }
    } else {
    }

word word_12
    push 120
    push 0
    store
    push 121
    push 0
    store
    push 122
    push 0
    store
    push 71
    load
    push 123
    swap
    store
    call 10
    call 11
    push 120
    load

word word_13
    push 122
    load
    push 123
    load
    swap
    sub
    sign
    push 1
    sub
    dup
    mul
    sign
    push 1
    swap
    sub
    if {
        push 122
        load
        push 80
        swap
        add
        load
        dup
        push 110
        swap
        store
        push 110
        load
        push 4
        div
        dup
        push 115
        swap
        store
        call 1
        push 120
        load
        add
        push 120
        swap
        store
        push 115
        load
        push 0
        sub
        dup
        mul
        sign
        push 1
        swap
        sub
        if {
            push 121
            load
            push 1
            add
            push 121
            swap
            store
        } else {
        }
        push 122
        load
        push 1
        add
        push 122
        swap
        store
        call 13
    } else {
    }

word word_14
    push 120
    load
    push 21
    swap
    sub
    sign
    push 1
    sub
    dup
    mul
    sign
    push 1
    swap
    sub
    if {
        push 121
        load
        sign
        push 1
        sub
        dup
        mul
        sign
        push 1
        swap
        sub
        if {
            push 120
            load
            push 10
            sub
            push 120
            swap
            store
            push 121
            load
            push 1
            sub
            push 121
            swap
            store
            call 14
        } else {
        }
    } else {
    }

word word_15
    push 120
    push 0
    store
    push 121
    push 0
    store
    push 122
    push 0
    store
    push 91
    load
    push 123
    swap
    store
    call 13
    call 14
    push 120
    load

word word_16
    push 71
    load
    push 2
    sub
    dup
    mul
    sign
    push 1
    swap
    sub
    if {
        call 12
        push 21
        sub
        dup
        mul
        sign
        push 1
        swap
        sub
    } else {
        push 0
    }

word word_17
    push 91
    load
    push 2
    sub
    dup
    mul
    sign
    push 1
    swap
    sub
    if {
        call 15
        push 21
        sub
        dup
        mul
        sign
        push 1
        swap
        sub
    } else {
        push 0
    }

word word_18
    push 122
    load
    push 123
    load
    swap
    sub
    sign
    push 1
    sub
    dup
    mul
    sign
    push 1
    swap
    sub
    if {
        push 122
        load
        push 80
        swap
        add
        load
        push 4
        div
        push 0
        sub
        dup
        mul
        sign
        push 1
        swap
        sub
        if {
            push 1
        } else {
            push 122
            load
            push 1
            add
            push 122
            swap
            store
            call 18
        }
    } else {
        push 0
    }

word word_19
    push 122
    push 0
    store
    push 91
    load
    push 123
    swap
    store
    call 18

word word_20
    call 15
    dup
    push 110
    swap
    store
    push 17
    swap
    sub
    sign
    push 1
    sub
    dup
    mul
    sign
    push 1
    swap
    sub
    if {
        push 1
    } else {
        push 110
        load
        push 17
        sub
        dup
        mul
        sign
        push 1
        swap
        sub
        if {
            call 19
        } else {
            push 0
        }
    }

word word_21
    push 122
    load
    push 123
    load
    swap
    sub
    sign
    push 1
    sub
    dup
    mul
    sign
    push 1
    swap
    sub
    if {
        push 114
        load
        push 122
        load
        push 1
        sub
        dup
        mul
        sign
        push 1
        swap
        sub
        mul
        if {
            push 63
            char-out
            push 63
            char-out
            push 32
            char-out
        } else {
            push 122
            load
            push 80
            swap
            add
            load
            call 9
        }
        push 122
        load
        push 1
        add
        push 122
        swap
        store
        call 21
    } else {
    }

word word_22
    push 114
    swap
    store
    push 122
    push 0
    store
    push 91
    load
    push 123
    swap
    store
    call 21
    push 10
    char-out

word word_23
    push 122
    load
    push 123
    load
    swap
    sub
    sign
    push 1
    sub
    dup
    mul
    sign
    push 1
    swap
    sub
    if {
        push 122
        load
        push 60
        swap
        add
        load
        call 9
        push 122
        load
        push 1
        add
        push 122
        swap
        store
        call 23
    } else {
    }

word word_24
    push 122
    push 0
    store
    push 71
    load
    push 123
    swap
    store
    call 23
    push 10
    char-out

word word_25
    push 114
    swap
    store
    push 27
    char-out
    push 91
    char-out
    push 50
    char-out
    push 74
    char-out
    push 27
    char-out
    push 91
    char-out
    push 72
    char-out
    push 27
    char-out
    push 91
    char-out
    push 57
    char-out
    push 51
    char-out
    push 109
    char-out
    push 27
    char-out
    push 91
    char-out
    push 49
    char-out
    push 109
    char-out
    push 9556
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9559
    char-out
    push 27
    char-out
    push 91
    char-out
    push 48
    char-out
    push 109
    char-out
    push 10
    char-out
    push 27
    char-out
    push 91
    char-out
    push 57
    char-out
    push 51
    char-out
    push 109
    char-out
    push 27
    char-out
    push 91
    char-out
    push 49
    char-out
    push 109
    char-out
    push 9553
    char-out
    push 32
    char-out
    push 32
    char-out
    push 9824
    char-out
    push 32
    char-out
    push 9829
    char-out
    push 32
    char-out
    push 32
    char-out
    push 66
    char-out
    push 32
    char-out
    push 76
    char-out
    push 32
    char-out
    push 65
    char-out
    push 32
    char-out
    push 67
    char-out
    push 32
    char-out
    push 75
    char-out
    push 32
    char-out
    push 74
    char-out
    push 32
    char-out
    push 65
    char-out
    push 32
    char-out
    push 67
    char-out
    push 32
    char-out
    push 75
    char-out
    push 32
    char-out
    push 32
    char-out
    push 9830
    char-out
    push 32
    char-out
    push 9827
    char-out
    push 32
    char-out
    push 32
    char-out
    push 9553
    char-out
    push 27
    char-out
    push 91
    char-out
    push 48
    char-out
    push 109
    char-out
    push 10
    char-out
    push 27
    char-out
    push 91
    char-out
    push 57
    char-out
    push 51
    char-out
    push 109
    char-out
    push 27
    char-out
    push 91
    char-out
    push 49
    char-out
    push 109
    char-out
    push 9568
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9571
    char-out
    push 27
    char-out
    push 91
    char-out
    push 48
    char-out
    push 109
    char-out
    push 10
    char-out
    push 67
    char-out
    push 114
    char-out
    push 101
    char-out
    push 100
    char-out
    push 105
    char-out
    push 116
    char-out
    push 115
    char-out
    push 58
    char-out
    push 32
    char-out
    push 100
    load
    num-out
    push 32
    char-out
    push 32
    char-out
    push 32
    char-out
    push 66
    char-out
    push 101
    char-out
    push 116
    char-out
    push 58
    char-out
    push 32
    char-out
    push 101
    load
    num-out
    push 10
    char-out
    push 68
    char-out
    push 69
    char-out
    push 65
    char-out
    push 76
    char-out
    push 69
    char-out
    push 82
    char-out
    push 32
    char-out
    push 32
    char-out
    push 91
    char-out
    push 114
    load
    if {
        push 80
        load
        load
        push 4
        div
        call 1
        num-out
    } else {
        call 15
        num-out
    }
    push 93
    char-out
    push 10
    char-out
    push 114
    load
    call 22
    push 89
    char-out
    push 79
    char-out
    push 85
    char-out
    push 32
    char-out
    push 32
    char-out
    push 32
    char-out
    push 32
    char-out
    push 32
    char-out
    push 91
    char-out
    call 12
    num-out
    push 93
    char-out
    push 10
    char-out
    call 24
    push 27
    char-out
    push 91
    char-out
    push 57
    char-out
    push 51
    char-out
    push 109
    char-out
    push 27
    char-out
    push 91
    char-out
    push 49
    char-out
    push 109
    char-out
    push 9562
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9565
    char-out
    push 27
    char-out
    push 91
    char-out
    push 48
    char-out
    push 109
    char-out
    push 10
    char-out

word word_26
    char-in
    dup
    push 10
    sub
    dup
    mul
    sign
    push 1
    swap
    sub
    if {
        drop
    } else {
        drop
        call 26
    }

word word_27
    dup
    push 115
    swap
    store
    push 72
    sub
    dup
    mul
    sign
    push 1
    swap
    sub
    if {
        push 104
    } else {
        push 115
        load
        push 83
        sub
        dup
        mul
        sign
        push 1
        swap
        sub
        if {
            push 115
        } else {
            push 115
            load
            push 81
            sub
            dup
            mul
            sign
            push 1
            swap
            sub
            if {
                push 113
            } else {
                push 115
                load
                push 66
                sub
                dup
                mul
                sign
                push 1
                swap
                sub
                if {
                    push 98
                } else {
                    push 115
                    load
                }
            }
        }
    }

word word_28
    push 32
    char-out
    push 32
    char-out
    push 62
    char-out
    push 32
    char-out
    push 1
    sleep
    char-in
    dup
    push 1
    neg
    sub
    dup
    mul
    sign
    push 1
    swap
    sub
    if {
        drop
        push 1
        neg
    } else {
        dup
        push 10
        sub
        dup
        mul
        sign
        push 1
        swap
        sub
        if {
            drop
            push 10
        } else {
            push 110
            swap
            store
            call 26
            push 110
            load
            call 27
        }
    }

word word_29
    push 27
    char-out
    push 91
    char-out
    push 50
    char-out
    push 74
    char-out
    push 27
    char-out
    push 91
    char-out
    push 72
    char-out
    push 27
    char-out
    push 91
    char-out
    push 57
    char-out
    push 54
    char-out
    push 109
    char-out
    push 27
    char-out
    push 91
    char-out
    push 49
    char-out
    push 109
    char-out
    push 32
    char-out
    push 32
    char-out
    push 9556
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9559
    char-out
    push 10
    char-out
    push 32
    char-out
    push 32
    char-out
    push 9553
    char-out
    push 32
    char-out
    push 32
    char-out
    push 32
    char-out
    push 32
    char-out
    push 9824
    char-out
    push 32
    char-out
    push 9829
    char-out
    push 32
    char-out
    push 32
    char-out
    push 66
    char-out
    push 76
    char-out
    push 65
    char-out
    push 67
    char-out
    push 75
    char-out
    push 74
    char-out
    push 65
    char-out
    push 67
    char-out
    push 75
    char-out
    push 32
    char-out
    push 32
    char-out
    push 9830
    char-out
    push 32
    char-out
    push 9827
    char-out
    push 32
    char-out
    push 32
    char-out
    push 32
    char-out
    push 32
    char-out
    push 32
    char-out
    push 32
    char-out
    push 9553
    char-out
    push 10
    char-out
    push 32
    char-out
    push 32
    char-out
    push 9553
    char-out
    push 32
    char-out
    push 32
    char-out
    push 32
    char-out
    push 32
    char-out
    push 32
    char-out
    push 32
    char-out
    push 32
    char-out
    push 66
    char-out
    push 97
    char-out
    push 115
    char-out
    push 101
    char-out
    push 32
    char-out
    push 66
    char-out
    push 108
    char-out
    push 97
    char-out
    push 99
    char-out
    push 107
    char-out
    push 106
    char-out
    push 97
    char-out
    push 99
    char-out
    push 107
    char-out
    push 32
    char-out
    push 32
    char-out
    push 32
    char-out
    push 32
    char-out
    push 32
    char-out
    push 32
    char-out
    push 32
    char-out
    push 32
    char-out
    push 9553
    char-out
    push 10
    char-out
    push 32
    char-out
    push 32
    char-out
    push 9562
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9552
    char-out
    push 9565
    char-out
    push 10
    char-out
    push 27
    char-out
    push 91
    char-out
    push 48
    char-out
    push 109
    char-out
    push 10
    char-out

word word_30
    push 100
    load
    sign
    push 1
    sub
    dup
    mul
    sign
    push 1
    swap
    sub
    if {
        push 67
        char-out
        push 114
        char-out
        push 101
        char-out
        push 100
        char-out
        push 105
        char-out
        push 116
        char-out
        push 115
        char-out
        push 58
        char-out
        push 32
        char-out
        push 100
        load
        num-out
        push 32
        char-out
        push 32
        char-out
        push 32
        char-out
        push 66
        char-out
        push 101
        char-out
        push 116
        char-out
        push 58
        char-out
        push 32
        char-out
        push 101
        load
        num-out
        push 10
        char-out
        push 91
        char-out
        push 69
        char-out
        push 78
        char-out
        push 84
        char-out
        push 69
        char-out
        push 82
        char-out
        push 93
        char-out
        push 32
        char-out
        push 68
        char-out
        push 101
        char-out
        push 97
        char-out
        push 108
        char-out
        push 32
        char-out
        push 91
        char-out
        push 98
        char-out
        push 93
        char-out
        push 32
        char-out
        push 67
        char-out
        push 104
        char-out
        push 97
        char-out
        push 110
        char-out
        push 103
        char-out
        push 101
        char-out
        push 32
        char-out
        push 98
        char-out
        push 101
        char-out
        push 116
        char-out
        push 32
        char-out
        push 91
        char-out
        push 113
        char-out
        push 93
        char-out
        push 32
        char-out
        push 81
        char-out
        push 117
        char-out
        push 105
        char-out
        push 116
        char-out
        push 10
        char-out
        call 28
        dup
        push 113
        sub
        dup
        mul
        sign
        push 1
        swap
        sub
        if {
            drop
            push 32
            char-out
            push 32
            char-out
            push 70
            char-out
            push 73
            char-out
            push 78
            char-out
            push 65
            char-out
            push 76
            char-out
            push 32
            char-out
            push 67
            char-out
            push 82
            char-out
            push 69
            char-out
            push 68
            char-out
            push 73
            char-out
            push 84
            char-out
            push 83
            char-out
            push 58
            char-out
            push 32
            char-out
            push 100
            load
            num-out
            push 10
            char-out
        } else {
            dup
            push 98
            sub
            dup
            mul
            sign
            push 1
            swap
            sub
            if {
                drop
                call 32
                dup
                push 0
                sub
                dup
                mul
                sign
                push 1
                swap
                sub
                if {
                    drop
                } else {
                    dup
                    push 1
                    swap
                    sub
                    sign
                    push 1
                    sub
                    dup
                    mul
                    sign
                    push 1
                    swap
                    sub
                    if {
                        drop
                        push 1
                    } else {
                        dup
                    }
                    dup
                    push 100
                    load
                    push 0
                    swap
                    sub
                    sign
                    push 1
                    sub
                    dup
                    mul
                    sign
                    push 1
                    swap
                    sub
                    if {
                        push 100
                        swap
                        store
                    } else {
                        drop
                    }
                    push 101
                    swap
                    store
                    call 30
                }
            } else {
                drop
                push 100
                load
                push 101
                load
                sub
                push 100
                swap
                store
                call 31
                push 102
                load
                dup
                push 1
                neg
                sub
                dup
                mul
                sign
                push 1
                swap
                sub
                if {
                    drop
                    push 101
                    load
                    push 100
                    load
                    add
                    push 100
                    swap
                    store
                    push 32
                    char-out
                    push 32
                    char-out
                    push 70
                    char-out
                    push 73
                    char-out
                    push 78
                    char-out
                    push 65
                    char-out
                    push 76
                    char-out
                    push 32
                    char-out
                    push 67
                    char-out
                    push 82
                    char-out
                    push 69
                    char-out
                    push 68
                    char-out
                    push 73
                    char-out
                    push 84
                    char-out
                    push 83
                    char-out
                    push 58
                    char-out
                    push 32
                    char-out
                    push 100
                    load
                    num-out
                    push 10
                    char-out
                } else {
                    push 102
                    load
                    dup
                    push 0
                    sub
                    dup
                    mul
                    sign
                    push 1
                    swap
                    sub
                    if {
                        drop
                    } else {
                        drop
                    }
                    drop
                    push 100
                    load
                    push 102
                    load
                    add
                    push 100
                    swap
                    store
                    push 500
                    sleep
                    call 30
                }
            }
        }
    } else {
        push 32
        char-out
        push 32
        char-out
        push 79
        char-out
        push 85
        char-out
        push 84
        char-out
        push 32
        char-out
        push 79
        char-out
        push 70
        char-out
        push 32
        char-out
        push 67
        char-out
        push 82
        char-out
        push 69
        char-out
        push 68
        char-out
        push 73
        char-out
        push 84
        char-out
        push 83
        char-out
        push 10
        char-out
    }

word word_31
    call 3
    call 5
    call 6
    call 5
    call 6
    push 1
    call 25
    push 101
    load
    push 102
    swap
    store
    call 34

word word_32
    push 32
    char-out
    push 32
    char-out
    push 69
    char-out
    push 78
    char-out
    push 84
    char-out
    push 69
    char-out
    push 82
    char-out
    push 32
    char-out
    push 66
    char-out
    push 69
    char-out
    push 84
    char-out
    push 32
    char-out
    push 40
    char-out
    push 49
    char-out
    push 45
    char-out
    push 100
    load
    num-out
    push 41
    char-out
    push 58
    char-out
    push 32
    char-out
    push 20
    dup
    push 101
    swap

word word_33
    char-in
    dup
    push 10
    sub
    dup
    mul
    sign
    push 1
    swap
    sub
    if {
        drop
        push 111
        load
        dup
        push 0
        sub
        dup
        mul
        sign
        push 1
        swap
        sub
        if {
            push 0
        } else {
            push 110
            load
        }
    } else {
        dup
        push 48
        sub
        sign
        dup
        push 1
        neg
        sub
        dup
        mul
        sign
        push 1
        swap
        sub
        if {
            drop
            call 33
        } else {
            dup
            push 57
            sub
            sign
            dup
            push 1
            sub
            dup
            mul
            sign
            push 1
            swap
            sub
            if {
                drop
                call 33
            } else {
                dup
                push 48
                sub
                push 110
                load
                push 10
                mul
                swap
                add
                push 110
                swap
                store
                drop
                push 1
                push 111
                swap
                store
                call 33
            }
        }
    }

word word_34
    call 12
    push 21
    sub
    sign
    dup
    push 1
    sub
    dup
    mul
    sign
    push 1
    swap
    sub
    if {
        push 0
        push 102
        swap
        store
    } else {
        push 32
        char-out
        push 32
        char-out
        push 91
        char-out
        push 104
        char-out
        push 93
        char-out
        push 32
        char-out
        push 72
        char-out
        push 105
        char-out
        push 116
        char-out
        push 32
        char-out
        push 91
        char-out
        push 115
        char-out
        push 93
        char-out
        push 32
        char-out
        push 83
        char-out
        push 116
        char-out
        push 97
        char-out
        push 110
        char-out
        push 100
        char-out
        call 28
        dup
        push 104
        sub
        dup
        mul
        sign
        push 1
        swap
        sub
        if {
            drop
            call 5
            push 1
            call 25
            call 34
        } else {
            dup
            push 115
            sub
            dup
            mul
            sign
            push 1
            swap
            sub
            if {
                drop
                push 101
                load
                push 2
                mul
                push 102
                swap
                store
            } else {
                dup
                push 113
                sub
                dup
                mul
                sign
                push 1
                swap
                sub
                if {
                    drop
                    push 1
                    neg
                    push 102
                    swap
                    store
                } else {
                    drop
                    push 101
                    load
                    push 2
                    mul
                    push 102
                    swap
                    store
                }
            }
        }
    }

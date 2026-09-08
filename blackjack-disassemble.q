word main
    push 100
    push 100
    store
    push 101
    push 10
    store
    call opening_banner
    call main_loop

word eq
    sub
    dup
    mul
    sign
    push 1
    swap
    sub

word ne
    call eq
    push 1
    swap
    sub

word gt
    sub
    sign
    push 1
    call eq

word lt
    sub
    sign
    push 1
    neg
    call eq

word ge
    call lt
    push 1
    swap
    sub

word le
    call gt
    push 1
    swap
    sub

word not
    push 0
    call eq

word card_value
    ; ( rank -- value )
    dup
    push 0
    call eq
    if {
        drop
        push 11
    } else {
        dup
        push 9
        call gt
        if {
            drop
            push 10
        } else {
            push 1
            add
        }
    }

word print_rank
    ; ( rank -- )
    dup
    push 0
    call eq
    if {
        drop
        push 65
        char-out
    } else {
        dup
        push 9
        call eq
        if {
            drop
            push 49
            char-out
            push 48
            char-out
        } else {
            dup
            push 10
            call eq
            if {
                drop
                push 74
                char-out
            } else {
                dup
                push 11
                call eq
                if {
                    drop
                    push 81
                    char-out
                } else {
                    dup
                    push 12
                    call eq
                    if {
                        drop
                        push 75
                        char-out
                    } else {
                        push 49
                        add
                        char-out
                    }
                }
            }
        }
    }

word print_suit
    ; ( suit -- )
    dup
    push 0
    call eq
    if {
        drop
        push 9824
        char-out
    } else {
        dup
        push 1
        call eq
        if {
            drop
            push 9829
            char-out
        } else {
            dup
            push 2
            call eq
            if {
                drop
                push 9830
                char-out
            } else {
                drop
                push 9827
                char-out
            }
        }
    }

word decode_card
    ; ( cardcode -- )  sets RC_RANK / RC_SUIT / RC_ISRED
    dup
    push 4
    div
    push 171
    swap
    store
    push 4
    mod
    push 172
    swap
    store
    push 172
    load
    push 1
    call eq
    push 172
    load
    push 2
    call eq
    add
    push 173
    swap
    store

word render_card_row1
    push 173
load
if {
push 27
char-out
push 91
char-out
push 57
char-out
push 49
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
} else {
push 27
char-out
push 91
char-out
push 57
char-out
push 55
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
}
    push 9484
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9488
char-out
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out

word render_card_row2
    push 173
load
if {
push 27
char-out
push 91
char-out
push 57
char-out
push 49
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
} else {
push 27
char-out
push 91
char-out
push 57
char-out
push 55
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
}
    push 9474
    char-out
    push 171
    load
    push 9
    call eq
    if {
        push 171
        load
        call print_rank
    } else {
        push 171
        load
        call print_rank
        push 32
        char-out
    }
    push 32
    char-out
    push 32
    char-out
    push 32
    char-out
    push 9474
    char-out
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out

word render_card_row3
    push 173
load
if {
push 27
char-out
push 91
char-out
push 57
char-out
push 49
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
} else {
push 27
char-out
push 91
char-out
push 57
char-out
push 55
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
}
    push 9474
    char-out
    push 32
    char-out
    push 32
    char-out
    push 172
    load
    call print_suit
    push 32
    char-out
    push 32
    char-out
    push 9474
    char-out
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out

word render_card_row4
    push 173
load
if {
push 27
char-out
push 91
char-out
push 57
char-out
push 49
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
} else {
push 27
char-out
push 91
char-out
push 57
char-out
push 55
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
}
    push 9474
    char-out
    push 32
    char-out
    push 32
    char-out
    push 32
    char-out
    push 171
    load
    push 9
    call eq
    if {
        push 171
        load
        call print_rank
    } else {
        push 32
        char-out
        push 171
        load
        call print_rank
    }
    push 9474
    char-out
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out

word render_card_row5
    push 173
load
if {
push 27
char-out
push 91
char-out
push 57
char-out
push 49
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
} else {
push 27
char-out
push 91
char-out
push 57
char-out
push 55
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
}
    push 9492
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9496
char-out
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out

word render_hidden_row1
push 27
char-out
push 91
char-out
push 50
char-out
push 109
char-out
push 9484
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9488
char-out
push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out

word render_hidden_row2
push 27
char-out
push 91
char-out
push 50
char-out
push 109
char-out
push 9474
char-out
push 9617
char-out
push 9617
char-out
push 9617
char-out
push 9617
char-out
push 9617
char-out
push 9474
char-out
push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out

word render_hidden_row3
push 27
char-out
push 91
char-out
push 50
char-out
push 109
char-out
push 9474
char-out
push 9617
char-out
push 9617
char-out
push 9617
char-out
push 9617
char-out
push 9617
char-out
push 9474
char-out
push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out

word render_hidden_row4
push 27
char-out
push 91
char-out
push 50
char-out
push 109
char-out
push 9474
char-out
push 9617
char-out
push 9617
char-out
push 9617
char-out
push 9617
char-out
push 9617
char-out
push 9474
char-out
push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out

word render_hidden_row5
push 27
char-out
push 91
char-out
push 50
char-out
push 109
char-out
push 9492
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9496
char-out
push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out

word init_deck_identity
    ; ( i -- )   sets mem[i]=i for i .. 51, recursing upward
    dup
    push 52
    call lt
    if {
        dup
        dup
        store
        push 1
        add
        call init_deck_identity
    } else {
        drop
    }

word shuffle
    ; ( i -- )  fisher-yates on deck[0..i], i counts down to 0
    dup
    push 0
    call gt
    if {
        dup
        push 140
        swap
        store
        push 1
        add
        rand
        push 141
        swap
        store
        push 140
        load
        load
        push 142
        swap
        store
        push 141
        load
        load
        push 143
        swap
        store
        push 140
        load
        push 143
        load
        store
        push 141
        load
        push 142
        load
        store
        push 140
        load
        push 1
        sub
        call shuffle
    } else {
        drop
    }

word fresh_deck
    push 0
    call init_deck_identity
    push 52
    push 52
    store
    push 76
    push 0
    store
    push 96
    push 0
    store
    push 51
    call shuffle

word deal_to_player
    push 52
    load
    push 1
    sub
    dup
    push 52
    swap
    store
    load
    push 76
    load
    push 60
    add
    swap
    store
    push 76
    load
    push 1
    add
    push 76
    swap
    store

word deal_to_dealer
    push 52
    load
    push 1
    sub
    dup
    push 52
    swap
    store
    load
    push 96
    load
    push 80
    add
    swap
    store
    push 96
    load
    push 1
    add
    push 96
    swap
    store

word hand_total_sum_loop
    push 152
    load
    push 154
    load
    call lt
    if {
        push 152
        load
        push 153
        load
        add
        load
        push 4
        div
        dup
        call card_value
        push 150
        load
        add
        push 150
        swap
        store
        push 0
        call eq
        if {
            push 151
            load
            push 1
            add
            push 151
            swap
            store
        } else {
        }
        push 152
        load
        push 1
        add
        push 152
        swap
        store
        call hand_total_sum_loop
    } else {
    }

word hand_total_reduce_loop
    push 150
    load
    push 21
    call gt
    push 151
    load
    push 0
    call gt
    mul
    if {
        push 150
        load
        push 10
        sub
        push 150
        swap
        store
        push 151
        load
        push 1
        sub
        push 151
        swap
        store
        call hand_total_reduce_loop
    } else {
    }

word hand_total
    ; ( base count -- total )
    push 154
    swap
    store
    push 153
    swap
    store
    push 150
    push 0
    store
    push 151
    push 0
    store
    push 152
    push 0
    store
    call hand_total_sum_loop
    call hand_total_reduce_loop
    push 150
    load

word raw_total_loop
    push 156
    load
    push 158
    load
    call lt
    if {
        push 156
        load
        push 157
        load
        add
        load
        push 4
        div
        call card_value
        push 155
        load
        add
        push 155
        swap
        store
        push 156
        load
        push 1
        add
        push 156
        swap
        store
        call raw_total_loop
    } else {
    }

word raw_total
    ; ( base count -- total )
    push 158
    swap
    store
    push 157
    swap
    store
    push 155
    push 0
    store
    push 156
    push 0
    store
    call raw_total_loop
    push 155
    load

word is_blackjack
    ; ( base count -- bool )   only true for a 2-card 21
    dup
    push 2
    call eq
    if {
        call hand_total
        push 21
        call eq
    } else {
        drop
        drop
        push 0
    }

word dealer_has_ace_loop
    ; ( i -- bool )
    dup
    push 96
    load
    call lt
    if {
        dup
        push 80
        add
        load
        push 4
        div
        push 0
        call eq
        if {
            drop
            push 1
        } else {
            push 1
            add
            call dealer_has_ace_loop
        }
    } else {
        drop
        push 0
    }

word dealer_has_ace
    push 0
    call dealer_has_ace_loop

word dealer_should_hit
    push 80
    push 96
    load
    call hand_total
    dup
    push 17
    call lt
    if {
        drop
        push 1
    } else {
        dup
        push 17
        call eq
        if {
            push 80
            push 96
            load
            call raw_total
            call eq
            if {
                call dealer_has_ace
            } else {
                push 0
            }
        } else {
            drop
            push 0
        }
    }

word draw_hand
    ; ( base count hide -- )
    push 183
    swap
    store
    push 181
    swap
    store
    push 182
    swap
    store
    push 184
    push 1
    store
    call draw_hand_row_loop

word draw_hand_row_loop
    push 184
    load
    push 5
    call le
    if {
        push 32
        char-out
        push 32
        char-out
        push 180
        push 0
        store
        call draw_hand_card_loop
        push 10
        char-out
        push 184
        load
        push 1
        add
        push 184
        swap
        store
        call draw_hand_row_loop
    } else {
    }

word draw_hand_card_loop
    push 180
    load
    push 181
    load
    call lt
    if {
        push 183
        load
        push 1
        call eq
        push 180
        load
        push 1
        call eq
        mul
        if {
            call draw_hand_hidden_row
        } else {
            push 180
            load
            push 182
            load
            add
            load
            call decode_card
            call draw_hand_card_row
        }
        push 32
        char-out
        push 180
        load
        push 1
        add
        push 180
        swap
        store
        call draw_hand_card_loop
    } else {
    }

word draw_hand_card_row
    push 184
    load
    push 1
    call eq
    if {
        call render_card_row1
    } else {
        push 184
        load
        push 2
        call eq
        if {
            call render_card_row2
        } else {
            push 184
            load
            push 3
            call eq
            if {
                call render_card_row3
            } else {
                push 184
                load
                push 4
                call eq
                if {
                    call render_card_row4
                } else {
                    call render_card_row5
                }
            }
        }
    }

word draw_hand_hidden_row
    push 184
    load
    push 1
    call eq
    if {
        call render_hidden_row1
    } else {
        push 184
        load
        push 2
        call eq
        if {
            call render_hidden_row2
        } else {
            push 184
            load
            push 3
            call eq
            if {
                call render_hidden_row3
            } else {
                push 184
                load
                push 4
                call eq
                if {
                    call render_hidden_row4
                } else {
                    call render_hidden_row5
                }
            }
        }
    }

word print_msg_1
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 55
char-out
push 109
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
push 32
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
push 32
char-out
push 32
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
push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out

word print_msg_2
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 55
char-out
push 109
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
push 32
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
push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out

word print_msg_3
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 49
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
push 66
char-out
push 85
char-out
push 83
char-out
push 84
char-out
push 33
char-out
push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out

word print_msg_4
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 53
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
push 33
char-out
push 32
char-out
push 80
char-out
push 97
char-out
push 121
char-out
push 115
char-out
push 32
char-out
push 51
char-out
push 58
char-out
push 50
char-out
push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out

word print_msg_5
push 32
char-out
push 32
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
push 68
char-out
push 101
char-out
push 97
char-out
push 108
char-out
push 101
char-out
push 114
char-out
push 32
char-out
push 114
char-out
push 101
char-out
push 118
char-out
push 101
char-out
push 97
char-out
push 108
char-out
push 115
char-out
push 32
char-out
push 104
char-out
push 111
char-out
push 108
char-out
push 101
char-out
push 32
char-out
push 99
char-out
push 97
char-out
push 114
char-out
push 100
char-out
push 46
char-out
push 46
char-out
push 46
char-out
push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out

word print_msg_6
push 32
char-out
push 32
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
push 68
char-out
push 101
char-out
push 97
char-out
push 108
char-out
push 101
char-out
push 114
char-out
push 32
char-out
push 104
char-out
push 105
char-out
push 116
char-out
push 115
char-out
push 46
char-out
push 46
char-out
push 46
char-out
push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out

word print_msg_7
    push 32
char-out
push 32
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
push 68
char-out
push 101
char-out
push 97
char-out
push 108
char-out
push 101
char-out
push 114
char-out
push 32
char-out
push 115
char-out
push 116
char-out
push 97
char-out
push 110
char-out
push 100
char-out
push 115
char-out
push 32
char-out
push 97
char-out
push 116
char-out
push 32
char-out
    push 105
    load
    num-out
    push 46
char-out
push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out

word print_msg_8
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 50
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
push 50
char-out
push 49
char-out
push 33
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
push 105
char-out
push 110
char-out
push 103
char-out
push 32
char-out
push 97
char-out
push 117
char-out
push 116
char-out
push 111
char-out
push 109
char-out
push 97
char-out
push 116
char-out
push 105
char-out
push 99
char-out
push 97
char-out
push 108
char-out
push 108
char-out
push 121
char-out
push 46
char-out
push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out

word print_msg_9
push 32
char-out
push 32
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
push 80
char-out
push 85
char-out
push 83
char-out
push 72
char-out
push 32
char-out
push 45
char-out
push 45
char-out
push 32
char-out
push 98
char-out
push 111
char-out
push 116
char-out
push 104
char-out
push 32
char-out
push 98
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
push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out

word print_msg_10
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 53
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
push 33
char-out
push 32
char-out
push 72
char-out
push 117
char-out
push 103
char-out
push 101
char-out
push 32
char-out
push 112
char-out
push 97
char-out
push 121
char-out
push 111
char-out
push 117
char-out
push 116
char-out
push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out

word print_msg_11
    push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 49
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
push 66
char-out
push 85
char-out
push 83
char-out
push 84
char-out
push 32
char-out
push 45
char-out
push 45
char-out
push 32
char-out
push 45
char-out
    push 101
    load
    num-out
    push 32
char-out
push 99
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
push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out

word print_msg_12
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 50
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
push 66
char-out
push 85
char-out
push 83
char-out
push 84
char-out
push 83
char-out
push 33
char-out
push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out

word print_msg_13
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 50
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
push 89
char-out
push 79
char-out
push 85
char-out
push 32
char-out
push 87
char-out
push 73
char-out
push 78
char-out
push 33
char-out
push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out

word print_msg_14
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 49
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
push 87
char-out
push 73
char-out
push 78
char-out
push 83
char-out
push 46
char-out
push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out

word print_msg_15
push 32
char-out
push 32
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
push 80
char-out
push 85
char-out
push 83
char-out
push 72
char-out
push 32
char-out
push 45
char-out
push 45
char-out
push 32
char-out
push 116
char-out
push 105
char-out
push 101
char-out
push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out

word print_message
push 111
load
push 0
call eq
if {
} else {
push 111
load
push 1
call eq
if {
    call print_msg_1
} else {
push 111
load
push 2
call eq
if {
    call print_msg_2
} else {
push 111
load
push 3
call eq
if {
    call print_msg_3
} else {
push 111
load
push 4
call eq
if {
    call print_msg_4
} else {
push 111
load
push 5
call eq
if {
    call print_msg_5
} else {
push 111
load
push 6
call eq
if {
    call print_msg_6
} else {
push 111
load
push 7
call eq
if {
    call print_msg_7
} else {
push 111
load
push 8
call eq
if {
    call print_msg_8
} else {
push 111
load
push 9
call eq
if {
    call print_msg_9
} else {
push 111
load
push 10
call eq
if {
    call print_msg_10
} else {
push 111
load
push 11
call eq
if {
    call print_msg_11
} else {
push 111
load
push 12
call eq
if {
    call print_msg_12
} else {
push 111
load
push 13
call eq
if {
    call print_msg_13
} else {
push 111
load
push 14
call eq
if {
    call print_msg_14
} else {
call print_msg_15
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
}
}
}
push 10
char-out

word draw_table
    push 110
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
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
    push 32
char-out
push 32
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
push 27
char-out
push 91
char-out
push 49
char-out
push 109
char-out
    push 100
    load
    num-out
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
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
push 27
char-out
push 91
char-out
push 49
char-out
push 109
char-out
    push 101
    load
    num-out
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
    push 32
char-out
push 32
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
    push 110
    load
    if {
        push 80
        load
        push 4
        div
        call card_value
        num-out
        push 32
char-out
push 43
char-out
push 32
char-out
push 63
char-out
    } else {
        push 80
        push 96
        load
        call hand_total
        num-out
    }
    push 93
char-out
    push 10
    char-out
    push 80
    push 96
    load
    push 110
    load
    call draw_hand
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
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
char-out
push 9472
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
    push 32
char-out
push 32
char-out
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
    push 60
    push 76
    load
    call hand_total
    dup
    push 106
    swap
    store
    num-out
    push 93
char-out
    push 106
    load
    push 21
    call gt
    if {
        push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 49
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
push 66
char-out
push 85
char-out
push 83
char-out
push 84
char-out
push 33
char-out
push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
    } else {
    }
    push 60
    push 76
    load
    call is_blackjack
    push 107
    load
    push 0
    call eq
    mul
    if {
        push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 53
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
push 33
char-out
push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
    } else {
    }
    push 10
    char-out
    push 60
    push 76
    load
    push 0
    call draw_hand
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
    call print_message
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

word consume_line
    ; discard chars up to and including next newline (or EOF)
    char-in
    dup
    push 10
    call eq
    if {
        drop
    } else {
        dup
        push 1
        neg
        call eq
        if {
            drop
        } else {
            drop
            call consume_line
        }
    }

word normalize_choice
    ; ( ch -- ch' )  uppercase H/S/Q/B -> lowercase, else unchanged
    dup
    push 72
    call eq
    if {
        drop
        push 104
    } else {
        dup
        push 83
        call eq
        if {
            drop
            push 115
        } else {
            dup
            push 81
            call eq
            if {
                drop
                push 113
            } else {
                dup
                push 66
                call eq
                if {
                    drop
                    push 98
                } else {
                }
            }
        }
    }

word read_choice
    ; ( -- ch )  -1 on EOF, 10 for bare ENTER, else normalized ascii
    push 1
    sleep
    char-in
    dup
    push 1
    neg
    call eq
    if {
        drop
        push 1
        neg
    } else {
        dup
        push 10
        call eq
        if {
            drop
            push 10
        } else {
            call consume_line
            call normalize_choice
        }
    }

word change_bet_loop
    char-in
    dup
    push 10
    call eq
    if {
        drop
    } else {
        dup
        push 1
        neg
        call eq
        if {
            drop
        } else {
            dup
            dup
            push 48
            call ge
            swap
            push 57
            call le
            mul
            if {
                push 200
                load
                push 10
                mul
                swap
                push 48
                sub
                add
                push 200
                swap
                store
                push 201
                push 1
                store
                call change_bet_loop
            } else {
                drop
                push 202
                push 0
                store
                call change_bet_loop
            }
        }
    }

word change_bet
    push 32
char-out
push 32
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
push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
    push 200
    push 0
    store
    push 201
    push 0
    store
    push 202
    push 1
    store
    call change_bet_loop
    push 201
    load
    push 202
    load
    mul
    if {
        push 200
        load
        push 1
        call lt
        if {
            push 1
            push 200
            swap
            store
        } else {
        }
        push 200
        load
        push 100
        load
        call gt
        if {
            push 100
            load
            push 200
            swap
            store
        } else {
        }
        push 200
        load
        push 101
        swap
        store
    } else {
    }

word flash_push_bj
push 13
char-out
push 32
char-out
push 32
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
    push 80
char-out
push 85
char-out
push 83
char-out
push 72
char-out
push 32
char-out
push 45
char-out
push 45
char-out
push 32
char-out
push 98
char-out
push 111
char-out
push 116
char-out
push 104
char-out
push 32
char-out
push 98
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
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 13
char-out
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 55
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
    push 80
char-out
push 85
char-out
push 83
char-out
push 72
char-out
push 32
char-out
push 45
char-out
push 45
char-out
push 32
char-out
push 98
char-out
push 111
char-out
push 116
char-out
push 104
char-out
push 32
char-out
push 98
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
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 13
char-out
push 32
char-out
push 32
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
    push 80
char-out
push 85
char-out
push 83
char-out
push 72
char-out
push 32
char-out
push 45
char-out
push 45
char-out
push 32
char-out
push 98
char-out
push 111
char-out
push 116
char-out
push 104
char-out
push 32
char-out
push 98
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
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 13
char-out
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 55
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
    push 80
char-out
push 85
char-out
push 83
char-out
push 72
char-out
push 32
char-out
push 45
char-out
push 45
char-out
push 32
char-out
push 98
char-out
push 111
char-out
push 116
char-out
push 104
char-out
push 32
char-out
push 98
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
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 13
char-out
push 32
char-out
push 32
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
    push 80
char-out
push 85
char-out
push 83
char-out
push 72
char-out
push 32
char-out
push 45
char-out
push 45
char-out
push 32
char-out
push 98
char-out
push 111
char-out
push 116
char-out
push 104
char-out
push 32
char-out
push 98
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
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 13
char-out
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 55
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
    push 80
char-out
push 85
char-out
push 83
char-out
push 72
char-out
push 32
char-out
push 45
char-out
push 45
char-out
push 32
char-out
push 98
char-out
push 111
char-out
push 116
char-out
push 104
char-out
push 32
char-out
push 98
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
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 10
    char-out

word flash_blackjack
push 13
char-out
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 53
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
push 33
char-out
push 32
char-out
push 72
char-out
push 117
char-out
push 103
char-out
push 101
char-out
push 32
char-out
push 112
char-out
push 97
char-out
push 121
char-out
push 111
char-out
push 117
char-out
push 116
char-out
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 13
char-out
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 55
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
push 33
char-out
push 32
char-out
push 72
char-out
push 117
char-out
push 103
char-out
push 101
char-out
push 32
char-out
push 112
char-out
push 97
char-out
push 121
char-out
push 111
char-out
push 117
char-out
push 116
char-out
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 13
char-out
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 53
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
push 33
char-out
push 32
char-out
push 72
char-out
push 117
char-out
push 103
char-out
push 101
char-out
push 32
char-out
push 112
char-out
push 97
char-out
push 121
char-out
push 111
char-out
push 117
char-out
push 116
char-out
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 13
char-out
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 55
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
push 33
char-out
push 32
char-out
push 72
char-out
push 117
char-out
push 103
char-out
push 101
char-out
push 32
char-out
push 112
char-out
push 97
char-out
push 121
char-out
push 111
char-out
push 117
char-out
push 116
char-out
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 13
char-out
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 53
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
push 33
char-out
push 32
char-out
push 72
char-out
push 117
char-out
push 103
char-out
push 101
char-out
push 32
char-out
push 112
char-out
push 97
char-out
push 121
char-out
push 111
char-out
push 117
char-out
push 116
char-out
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 13
char-out
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 55
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
push 33
char-out
push 32
char-out
push 72
char-out
push 117
char-out
push 103
char-out
push 101
char-out
push 32
char-out
push 112
char-out
push 97
char-out
push 121
char-out
push 111
char-out
push 117
char-out
push 116
char-out
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 10
    char-out

word flash_bust
push 13
char-out
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 49
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
    push 66
char-out
push 85
char-out
push 83
char-out
push 84
char-out
push 32
char-out
push 45
char-out
push 45
char-out
push 32
char-out
push 45
char-out
    push 101
    load
    num-out
    push 32
char-out
push 99
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
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 13
char-out
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 55
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
    push 66
char-out
push 85
char-out
push 83
char-out
push 84
char-out
push 32
char-out
push 45
char-out
push 45
char-out
push 32
char-out
push 45
char-out
    push 101
    load
    num-out
    push 32
char-out
push 99
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
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 13
char-out
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 49
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
    push 66
char-out
push 85
char-out
push 83
char-out
push 84
char-out
push 32
char-out
push 45
char-out
push 45
char-out
push 32
char-out
push 45
char-out
    push 101
    load
    num-out
    push 32
char-out
push 99
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
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 13
char-out
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 55
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
    push 66
char-out
push 85
char-out
push 83
char-out
push 84
char-out
push 32
char-out
push 45
char-out
push 45
char-out
push 32
char-out
push 45
char-out
    push 101
    load
    num-out
    push 32
char-out
push 99
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
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 13
char-out
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 49
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
    push 66
char-out
push 85
char-out
push 83
char-out
push 84
char-out
push 32
char-out
push 45
char-out
push 45
char-out
push 32
char-out
push 45
char-out
    push 101
    load
    num-out
    push 32
char-out
push 99
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
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 13
char-out
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 55
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
    push 66
char-out
push 85
char-out
push 83
char-out
push 84
char-out
push 32
char-out
push 45
char-out
push 45
char-out
push 32
char-out
push 45
char-out
    push 101
    load
    num-out
    push 32
char-out
push 99
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
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 10
    char-out

word flash_dealer_busts
push 13
char-out
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 50
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
push 66
char-out
push 85
char-out
push 83
char-out
push 84
char-out
push 83
char-out
push 33
char-out
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 13
char-out
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 55
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
push 66
char-out
push 85
char-out
push 83
char-out
push 84
char-out
push 83
char-out
push 33
char-out
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 13
char-out
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 50
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
push 66
char-out
push 85
char-out
push 83
char-out
push 84
char-out
push 83
char-out
push 33
char-out
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 13
char-out
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 55
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
push 66
char-out
push 85
char-out
push 83
char-out
push 84
char-out
push 83
char-out
push 33
char-out
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 13
char-out
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 50
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
push 66
char-out
push 85
char-out
push 83
char-out
push 84
char-out
push 83
char-out
push 33
char-out
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 13
char-out
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 55
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
push 66
char-out
push 85
char-out
push 83
char-out
push 84
char-out
push 83
char-out
push 33
char-out
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 10
    char-out

word flash_you_win
push 13
char-out
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 50
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
    push 89
char-out
push 79
char-out
push 85
char-out
push 32
char-out
push 87
char-out
push 73
char-out
push 78
char-out
push 33
char-out
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 13
char-out
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 55
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
    push 89
char-out
push 79
char-out
push 85
char-out
push 32
char-out
push 87
char-out
push 73
char-out
push 78
char-out
push 33
char-out
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 13
char-out
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 50
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
    push 89
char-out
push 79
char-out
push 85
char-out
push 32
char-out
push 87
char-out
push 73
char-out
push 78
char-out
push 33
char-out
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 13
char-out
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 55
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
    push 89
char-out
push 79
char-out
push 85
char-out
push 32
char-out
push 87
char-out
push 73
char-out
push 78
char-out
push 33
char-out
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 13
char-out
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 50
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
    push 89
char-out
push 79
char-out
push 85
char-out
push 32
char-out
push 87
char-out
push 73
char-out
push 78
char-out
push 33
char-out
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 13
char-out
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 55
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
    push 89
char-out
push 79
char-out
push 85
char-out
push 32
char-out
push 87
char-out
push 73
char-out
push 78
char-out
push 33
char-out
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 10
    char-out

word flash_dealer_wins
push 13
char-out
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 49
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
push 87
char-out
push 73
char-out
push 78
char-out
push 83
char-out
push 46
char-out
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 13
char-out
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 55
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
push 87
char-out
push 73
char-out
push 78
char-out
push 83
char-out
push 46
char-out
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 13
char-out
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 49
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
push 87
char-out
push 73
char-out
push 78
char-out
push 83
char-out
push 46
char-out
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 13
char-out
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 55
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
push 87
char-out
push 73
char-out
push 78
char-out
push 83
char-out
push 46
char-out
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 13
char-out
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 49
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
push 87
char-out
push 73
char-out
push 78
char-out
push 83
char-out
push 46
char-out
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 13
char-out
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 55
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
push 87
char-out
push 73
char-out
push 78
char-out
push 83
char-out
push 46
char-out
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 10
    char-out

word flash_push_tie
push 13
char-out
push 32
char-out
push 32
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
    push 80
char-out
push 85
char-out
push 83
char-out
push 72
char-out
push 32
char-out
push 45
char-out
push 45
char-out
push 32
char-out
push 116
char-out
push 105
char-out
push 101
char-out
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 13
char-out
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 55
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
    push 80
char-out
push 85
char-out
push 83
char-out
push 72
char-out
push 32
char-out
push 45
char-out
push 45
char-out
push 32
char-out
push 116
char-out
push 105
char-out
push 101
char-out
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 13
char-out
push 32
char-out
push 32
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
    push 80
char-out
push 85
char-out
push 83
char-out
push 72
char-out
push 32
char-out
push 45
char-out
push 45
char-out
push 32
char-out
push 116
char-out
push 105
char-out
push 101
char-out
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 13
char-out
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 55
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
    push 80
char-out
push 85
char-out
push 83
char-out
push 72
char-out
push 32
char-out
push 45
char-out
push 45
char-out
push 32
char-out
push 116
char-out
push 105
char-out
push 101
char-out
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 13
char-out
push 32
char-out
push 32
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
    push 80
char-out
push 85
char-out
push 83
char-out
push 72
char-out
push 32
char-out
push 45
char-out
push 45
char-out
push 32
char-out
push 116
char-out
push 105
char-out
push 101
char-out
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 13
char-out
push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 55
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
    push 80
char-out
push 85
char-out
push 83
char-out
push 72
char-out
push 32
char-out
push 45
char-out
push 45
char-out
push 32
char-out
push 116
char-out
push 105
char-out
push 101
char-out
    push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
char-out
    push 120
    sleep
    push 10
    char-out

word dealer_play
    push 107
    push 1
    store
    push 111
    push 5
    store
    push 0
    call draw_table
    push 1000
    sleep
    call dealer_play_loop

word dealer_play_loop
    call dealer_should_hit
    if {
        call deal_to_dealer
        push 111
        push 6
        store
        push 0
        call draw_table
        push 900
        sleep
        call dealer_play_loop
    } else {
        push 80
        push 96
        load
        call hand_total
        push 105
        swap
        store
        push 111
        push 7
        store
        push 0
        call draw_table
        push 800
        sleep
    }

word play_round
    call fresh_deck
    call deal_to_player
    call deal_to_dealer
    call deal_to_player
    call deal_to_dealer
    push 107
    push 0
    store
    push 111
    push 1
    store
    push 1
    call draw_table
    push 60
    push 76
    load
    call is_blackjack
    if {
        push 80
        push 96
        load
        call is_blackjack
        push 107
        push 1
        store
        push 111
        push 4
        store
        push 0
        call draw_table
        push 1500
        sleep
        if {
            push 101
            load
            push 102
            swap
            store
            call flash_push_bj
        } else {
            push 101
            load
            push 5
            mul
            push 2
            div
            push 102
            swap
            store
            call flash_blackjack
        }
    } else {
        push 104
        push 0
        store
        call play_loop
        push 104
        load
        push 0
        call eq
        if {
            call dealer_play
            push 60
            push 76
            load
            call hand_total
            push 106
            swap
            store
            push 80
            push 96
            load
            call hand_total
            push 105
            swap
            store
            push 105
            load
            push 21
            call gt
            if {
                push 101
                load
                push 2
                mul
                push 102
                swap
                store
                call flash_dealer_busts
            } else {
                push 106
                load
                push 105
                load
                call gt
                if {
                    push 101
                    load
                    push 2
                    mul
                    push 102
                    swap
                    store
                    call flash_you_win
                } else {
                    push 106
                    load
                    push 105
                    load
                    call lt
                    if {
                        push 0
                        push 102
                        swap
                        store
                        call flash_dealer_wins
                    } else {
                        push 101
                        load
                        push 102
                        swap
                        store
                        call flash_push_tie
                    }
                }
            }
        } else {
        }
    }

word play_loop
    push 60
    push 76
    load
    call hand_total
    push 106
    swap
    store
    push 106
    load
    push 21
    call gt
    if {
        push 107
        push 1
        store
        push 111
        push 3
        store
        push 1
        call draw_table
        push 1000
        sleep
        push 0
        push 102
        swap
        store
        push 1
        push 104
        swap
        store
        call flash_bust
    } else {
        push 106
        load
        push 21
        call eq
        if {
            push 111
            push 8
            store
            push 1
            call draw_table
            push 1000
            sleep
        } else {
            push 111
            push 2
            store
            push 1
            call draw_table
            call read_choice
            dup
            push 104
            call eq
            if {
                drop
                call deal_to_player
                call play_loop
            } else {
                dup
                push 115
                call eq
                if {
                    drop
                } else {
                    dup
                    push 113
                    call eq
                    if {
                        drop
                        push -1
                        push 102
                        swap
                        store
                        push 1
                        push 104
                        swap
                        store
                    } else {
                        drop
                    }
                }
            }
        }
    }

word opening_banner
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
    push 10
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

word main_loop
    push 100
    load
    push 0
    call le
    if {
        push 10
        char-out
        push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 49
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
push 46
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
        push 10
        char-out
    } else {
        push 101
        load
        push 100
        load
        call gt
        if {
            push 100
            load
            push 101
            swap
            store
        } else {
        }
        push 32
char-out
push 32
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
push 27
char-out
push 91
char-out
push 49
char-out
push 109
char-out
        push 100
        load
        num-out
        push 27
char-out
push 91
char-out
push 48
char-out
push 109
char-out
push 32
char-out
push 32
char-out
push 32
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
push 27
char-out
push 91
char-out
push 49
char-out
push 109
char-out
        push 101
        load
        num-out
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
        push 32
char-out
push 32
char-out
push 27
char-out
push 91
char-out
push 57
char-out
push 55
char-out
push 109
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
push 32
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
push 32
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
        call read_choice
        dup
        push 113
        call eq
        if {
            drop
            push 10
            char-out
            push 32
char-out
push 32
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
push 27
char-out
push 91
char-out
push 49
char-out
push 109
char-out
            push 100
            load
            num-out
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
            push 10
            char-out
        } else {
            dup
            push 98
            call eq
            if {
                drop
                call change_bet
                push 10
                char-out
                call main_loop
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
                push -1
                push 102
                swap
                store
                call play_round
                push 102
                load
                push -1
                call eq
                if {
                    push 100
                    load
                    push 101
                    load
                    add
                    push 100
                    swap
                    store
                    push 10
                    char-out
                    push 32
char-out
push 32
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
push 27
char-out
push 91
char-out
push 49
char-out
push 109
char-out
                    push 100
                    load
                    num-out
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
                    push 10
                    char-out
                } else {
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
                    call opening_banner
                    call main_loop
                }
            }
        }
    }

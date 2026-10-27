; GET and HEAD are supported. Anything else -> 501.
; Run with --steps 0 (unlimited) because a step-limited run would eventually halt (accept loop never returns)

word main
    let REQ_BUF = 1000
    let PATH_BUF = 10000
    let OUT_BUF = 13000
    let FILE_BUF = 16000
    let DIR_BUF = 21000
    let HTML_BUF = 30000
    let INDEX_BUF = 43000
    let SERVER_FD = 0
    let CLIENT_FD = 0
    let REQ_LEN = 0
    let R_N = 0
    let H_IDX = 0
    let METHOD = 0
    let SP1 = 0
    let SP2 = 0
    let LINE_END = 0
    let URL_START = 0
    let URL_END = 0
    let PATH_LEN = 0
    let HAS_SLASH = 0
    let D_SRC = 0
    let D_IDX = 0
    let FS_STAT = 0
    let FILE_FD = 0
    let FILE_SIZE = 0
    let F_N = 0
    let DIR_LEN = 0
    let BODY_LEN = 0
    let HDR_LEN = 0
    let STATUS = 0
    let O_BASE = 0
    let O_POS = 0
    let O_SRC = 0
    let O_LEN = 0
    let O_IDX = 0
    let S_SRC = 0
    let S_LEN = 0
    let S_OFF = 0
    let S_CHUNK = 0
    let E_PTR = 0
    let E_LEN = 0
    let E_IDX = 0
    let E_PC = 0
    let E_EC = 0
    let C_A = 0
    let C_B = 0
    let T_IDX = 0
    let I_LEN = 0
    let G_IDX = 0
    let G_OFF = 0
    let G_END = 0
    let L_IDX = 0
    let F_SCAN = 0
    let HEND = 0
    let BODY_CL = 0
    let CL_P = 0
    let CL_V = 0
    let CH = 0
    let F_I = 0
    let F_P = 0
    let MSG_EQ = 0
    let MSG_LEN = 0
    let E_I = 0
    let D_END = 0
    let EPTR = 0
    let ELEN = 0
    let GPATH = 60000
    let GB_MSG = 50000
    let GFD = 0
    let GB_N = 0
    let WPTR = 60000

    ; extension literals for mime guessing
    push 46100
    push 46
    store
    push 46101
    push 104
    store
    push 46102
    push 116
    store
    push 46103
    push 109
    store
    push 46104
    push 108
    store
    push 46110
    push 46
    store
    push 46111
    push 104
    store
    push 46112
    push 116
    store
    push 46113
    push 109
    store
    push 46120
    push 46
    store
    push 46121
    push 99
    store
    push 46122
    push 115
    store
    push 46123
    push 115
    store
    push 46130
    push 46
    store
    push 46131
    push 106
    store
    push 46132
    push 115
    store
    push 46140
    push 46
    store
    push 46141
    push 106
    store
    push 46142
    push 115
    store
    push 46143
    push 111
    store
    push 46144
    push 110
    store
    push 46150
    push 46
    store
    push 46151
    push 112
    store
    push 46152
    push 110
    store
    push 46153
    push 103
    store
    push 46160
    push 46
    store
    push 46161
    push 106
    store
    push 46162
    push 112
    store
    push 46163
    push 103
    store
    push 46170
    push 46
    store
    push 46171
    push 106
    store
    push 46172
    push 112
    store
    push 46173
    push 101
    store
    push 46174
    push 103
    store
    push 46180
    push 46
    store
    push 46181
    push 103
    store
    push 46182
    push 105
    store
    push 46183
    push 102
    store
    push 46190
    push 46
    store
    push 46191
    push 105
    store
    push 46192
    push 99
    store
    push 46193
    push 111
    store
    push 46195
    push 46
    store
    push 46196
    push 115
    store
    push 46197
    push 118
    store
    push 46198
    push 103
    store
    push 46205
    push 46
    store
    push 46206
    push 116
    store
    push 46207
    push 120
    store
    push 46208
    push 116
    store
    push 46215
    push 46
    store
    push 46216
    push 112
    store
    push 46217
    push 100
    store
    push 46218
    push 102
    store
    push 46225
    push 46
    store
    push 46226
    push 119
    store
    push 46227
    push 97
    store
    push 46228
    push 115
    store
    push 46229
    push 109
    store

    call init-gb
    sock-create
    store SERVER_FD
    load SERVER_FD
    push -1
    call eq
    if {
        halt
    } else {
        id
    }
    load SERVER_FD
    push 8000
    sock-bind
    push 0
    call eq
    if {
        id
    } else {
        halt
    }
    load SERVER_FD
    push 10
    sock-listen
    drop
    call accept-loop
    halt

; tiny comparisons (a b -- r)
word lt
    sub
    sign
    push 1
    add
    if {
        push 0
    } else {
        push 1
    }

word eq
    sub
    if {
        push 0
    } else {
        push 1
    }

word gt
    swap
    call lt

word le
    call gt
    if {
        push 0
    } else {
        push 1
    }

word ge
    call lt
    if {
        push 0
    } else {
        push 1
    }

; lowercase one ASCII char
word tolow
    dup
    push 65
    call lt
    if {
        id
    } else {
        dup
        push 90
        call gt
        if {
            id
        } else {
            push 32
            add
        }
    }

; hex digit value, -1 if not hex
word hexval
    dup
    push 48
    call lt
    if {
        drop
        push -1
    } else {
        dup
        push 57
        call gt
        if {
            dup
            push 65
            call lt
            if {
                drop
                push -1
            } else {
                dup
                push 70
                call gt
                if {
                    dup
                    push 97
                    call lt
                    if {
                        drop
                        push -1
                    } else {
                        dup
                        push 102
                        call gt
                        if {
                            drop
                            push -1
                        } else {
                            push 87
                            sub
                        }
                    }
                } else {
                    push 55
                    sub
                }
            }
        } else {
            push 48
            sub
        }
    }

; accept loop: one request per connection, like python HTTP/1.0
word accept-loop
    load SERVER_FD
    sock-accept
    store CLIENT_FD
    load CLIENT_FD
    push -1
    call eq
    if {
        push 100
        sleep
        call accept-loop
    } else {
        call handle-client
        load CLIENT_FD
        close
        drop
        call accept-loop
    }

word handle-client
    call recv-request
    load REQ_LEN
    push 0
    call gt
    if {
        call parse-request
        call route
    } else {
        id
    }

; read until end of headers (\r\n\r\n or \n\n), cap 8000
word recv-request
    push 0
    store REQ_LEN
    call recv-loop

word recv-loop
    load REQ_LEN
    push 8000
    call lt
    if {
        load CLIENT_FD
        load REQ_BUF
        load REQ_LEN
        add
        push 4096
        sock-recv
        store R_N
        load R_N
        push 0
        call gt
        if {
            load REQ_LEN
            load R_N
            add
            store REQ_LEN

            call make-check-hend
            load HEND
            push 0
            call ge
            if {
                ; headers complete: still want CL bytes of body
                load REQ_LEN
                load HEND
                load BODY_CL
                add
                call lt
                if { call recv-loop } else { id }
            } else {
                call recv-loop
            }
        } else {
            id
        }
    } else { id }
word make-check-hend
    push -1
    store HEND
    push 0
    store BODY_CL
    push 0
    store F_I
    call chen-loop
    load HEND
    push 0
    call ge
    if {
        call parse-cl
        load HEND
        push 1
        add
        store HEND
    } else {
        id
    }

word chen-loop
    load HEND
    push 0
    call ge
    if {
        id
    } else {
        load F_I
        load REQ_LEN
        call lt
        if {
            load REQ_BUF
            load F_I
            add
            load
            push 10
            call eq
            if {
                load F_I
                push 1
                call ge
                if {
                    load REQ_BUF
                    load F_I
                    push 1
                    sub
                    add
                    load
                    push 10
                    call eq
                    if {
                        load F_I
                        store HEND
                    } else {
                        load F_I
                        push 3
                        call ge
                        if {
                            load REQ_BUF
                            load F_I
                            push 3
                            sub
                            add
                            load
                            push 13
                            call eq
                            if {
                                load REQ_BUF
                                load F_I
                                push 2
                                sub
                                add
                                load
                                push 10
                                call eq
                                if {
                                    load REQ_BUF
                                    load F_I
                                    push 1
                                    sub
                                    add
                                    load
                                    push 13
                                    call eq
                                    if {
                                        load F_I
                                        store HEND
                                    } else {
                                        push 0
                                        call chen-next
                                    }
                                } else {
                                    push 0
                                    call chen-next
                                }
                            } else {
                                push 0
                                call chen-next
                            }
                        } else {
                            push 0
                            call chen-next
                        }
                    }
                } else {
                    push 0
                    call chen-next
                }
            } else {
                push 0
                call chen-next
            }
        } else {
            id
        }
    }

word chen-next
    drop
    load F_I
    push 1
    add
    store F_I
    call chen-loop

word parse-cl
    push 0
    store CL_P
    call pcl-loop

word pcl-loop
    load CL_P
    load HEND
    call lt
    if {
        load REQ_BUF
        load CL_P
        add
        load
        push 58
        call eq
        if {
            call is-length-colon
            if {
                load CL_P
                push 1
                add
                store CL_V
                call pcv-loop
                load CL_V
                store CL_P
                call pcl-loop
            } else {
                load CL_P
                push 1
                add
                store CL_P
                call pcl-loop
            }
        } else {
            load CL_P
            push 1
            add
            store CL_P
            call pcl-loop
        }
    } else {
        id
    }

word is-length-colon
    ; 1 iff the six chars ending at CL_P-1 are 'length' (case-insensitive)
    load CL_P
    push 6
    call ge
    if {
        load REQ_BUF
        load CL_P
        push 6
        sub
        add
        load
        call tolow
        push 108
        call eq
        if {
            load REQ_BUF
            load CL_P
            push 5
            sub
            add
            load
            call tolow
            push 101
            call eq
            if {
                load REQ_BUF
                load CL_P
                push 4
                sub
                add
                load
                call tolow
                push 110
                call eq
                if {
                    load REQ_BUF
                    load CL_P
                    push 3
                    sub
                    add
                    load
                    call tolow
                    push 103
                    call eq
                    if {
                        load REQ_BUF
                        load CL_P
                        push 2
                        sub
                        add
                        load
                        call tolow
                        push 116
                        call eq
                        if {
                            load REQ_BUF
                            load CL_P
                            push 1
                            sub
                            add
                            load
                            call tolow
                            push 104
                            call eq
                            if {
                                load CL_P
                                push 9
                                call ge
                                if {
                                    load REQ_BUF
                                    load CL_P
                                    push 8
                                    sub
                                    add
                                    load
                                    push 58
                                    call eq
                                    if { push 1 } else {
                                        load REQ_BUF
                                        load CL_P
                                        push 8
                                        sub
                                        add
                                        load
                                        push 45
                                        call eq
                                        if { push 1 } else {
                                            load REQ_BUF
                                            load CL_P
                                            push 8
                                            sub
                                            add
                                            load
                                            push 32
                                            call eq
                                            if { push 1 } else { push 0 }
                                        }
                                    }
                                } else {
                                    ; header started before byte 8: accept if line began at 0
                                    load CL_P
                                    push 7
                                    call eq
                                    if { push 1 } else { push 0 }
                                }
                            } else { push 0 }
                        } else { push 0 }
                    } else { push 0 }
                } else { push 0 }
            } else { push 0 }
        } else { push 0 }
    } else {
        push 0
    }

word pcv-loop
    load CL_V
    load HEND
    call lt
    if {
        load REQ_BUF
        load CL_V
        add
        load
        store CH
        load CH
        push 32
        call eq
        if {
            load CL_V
            push 1
            add
            store CL_V
            call pcv-loop
        } else {
            load CH
            push 48
            call ge
            if {
                load CH
                push 57
                call le
                if {
                    load BODY_CL
                    push 10
                    mul
                    load CH
                    push 48
                    sub
                    add
                    store BODY_CL
                    load CL_V
                    push 1
                    add
                    store CL_V
                    call pcv-loop
                } else {
                    id
                }
            } else {
                id
            }
        }
    } else {
        id
    }
word parse-request
    call find-spaces
    call parse-method
    call parse-path

word find-spaces
    push -1
    store SP1
    push -1
    store SP2
    push -1
    store LINE_END
    push 0
    store F_SCAN
    call find-spaces-loop
    ; fallback: no spaces at all -> whole buffer is garbage, SP1 stays -1
    ; fallback line end: if none found, use REQ_LEN
    load LINE_END
    push -1
    call eq
    if {
        load REQ_LEN
        store LINE_END
    } else {
        id
    }
    ; fallback second space: if none, use line end
    load SP2
    push -1
    call eq
    if {
        load LINE_END
        store SP2
    } else {
        id
    }

word find-spaces-loop
    load F_SCAN
    load REQ_LEN
    call lt
    if {
        load REQ_BUF
        load F_SCAN
        add
        load
        push 32
        call eq
        if {
            load SP1
            push -1
            call eq
            if {
                load F_SCAN
                store SP1
            } else {
                load SP2
                push -1
                call eq
                if {
                    load F_SCAN
                    store SP2
                } else {
                    id
                }
            }
            load F_SCAN
            push 1
            add
            store F_SCAN
            call find-spaces-loop
        } else {
            load REQ_BUF
            load F_SCAN
            add
            load
            push 13
            call eq
            if {
                load LINE_END
                push -1
                call eq
                if {
                    load F_SCAN
                    store LINE_END
                } else {
                    id
                }
                load F_SCAN
                push 1
                add
                store F_SCAN
                call find-spaces-loop
            } else {
                load REQ_BUF
                load F_SCAN
                add
                load
                push 10
                call eq
                if {
                    load LINE_END
                    push -1
                    call eq
                    if {
                        load F_SCAN
                        store LINE_END
                    } else {
                        id
                    }
                    load F_SCAN
                    push 1
                    add
                    store F_SCAN
                    call find-spaces-loop
                } else {
                    load F_SCAN
                    push 1
                    add
                    store F_SCAN
                    call find-spaces-loop
                }
            }
        }
    } else {
        id
    }

word parse-method
    push 2
    store METHOD
    load METHOD
    push 2
    call eq
    if {
    load SP1
    push 3
    call ge
    if {
    load REQ_BUF
    push 2
    add
    load
    push 84
    call eq
    if {
    load REQ_BUF
    push 1
    add
    load
    push 69
    call eq
    if {
    load REQ_BUF
    load
    push 71
    call eq
    if {
    push 0
    store METHOD
    } else {
    id
    }
    } else {
    id
    }
    } else {
    id
    }
    } else {
    id
    }
    } else {
    id
    }
    load METHOD
    push 2
    call eq
    if {
    load SP1
    push 4
    call ge
    if {
    load REQ_BUF
    push 3
    add
    load
    push 68
    call eq
    if {
    load REQ_BUF
    push 2
    add
    load
    push 65
    call eq
    if {
    load REQ_BUF
    push 1
    add
    load
    push 69
    call eq
    if {
    load REQ_BUF
    load
    push 72
    call eq
    if {
    push 1
    store METHOD
    } else {
    id
    }
    } else {
    id
    }
    } else {
    id
    }
    } else {
    id
    }
    } else {
    id
    }
    } else {
    id
    }
    load METHOD
    push 2
    call eq
    if {
    load SP1
    push 4
    call ge
    if {
    load REQ_BUF
    push 3
    add
    load
    push 84
    call eq
    if {
    load REQ_BUF
    push 2
    add
    load
    push 83
    call eq
    if {
    load REQ_BUF
    push 1
    add
    load
    push 79
    call eq
    if {
    load REQ_BUF
    load
    push 80
    call eq
    if {
    push 3
    store METHOD
    } else {
    id
    }
    } else {
    id
    }
    } else {
    id
    }
    } else {
    id
    }
    } else {
    id
    }
    } else {
    id
    }
word parse-path
    ; URL region = (SP1+1 .. SP2), but SP1 may be -1 for garbage
    load SP1
    push 0
    call ge
    if {
        load SP1
        push 1
        add
        store URL_START
    } else {
        push 0
        store URL_START
    }
    load SP2
    load URL_START
    call gt
    if {
        load SP2
        store URL_END
    } else {
        load URL_START
        store URL_END
    }
    ; clamp URL_END to REQ_LEN
    load URL_END
    load REQ_LEN
    call gt
    if {
        load REQ_LEN
        store URL_END
    } else {
        id
    }
    ; HAS_SLASH if last URL byte is '/'
    push 0
    store HAS_SLASH
    load URL_END
    load URL_START
    call gt
    if {
        load REQ_BUF
        load URL_END
        push 1
        sub
        add
        load
        push 47
        call eq
        if {
            push 1
            store HAS_SLASH
        } else {
            id
        }
    } else {
        id
    }
    call decode-path
    ; empty path (was "/" or empty) -> "." so syscalls work
    load PATH_LEN
    push 0
    call eq
    if {
        load PATH_BUF
        push 46
        store
        push 1
        store PATH_LEN
        push 1
        store HAS_SLASH
    } else {
        id
    }

word decode-path
    push 0
    store PATH_LEN
    load URL_START
    store D_SRC
    ; skip one leading slash
    load D_SRC
    load URL_END
    call lt
    if {
        load REQ_BUF
        load D_SRC
        add
        load
        push 47
        call eq
        if {
            load D_SRC
            push 1
            add
            store D_SRC
        } else {
            id
        }
    } else {
        id
    }
    call decode-loop

word decode-loop
    load D_SRC
    load URL_END
    call lt
    if {
        load REQ_BUF
        load D_SRC
        add
        load
        push 63
        call eq
        if {
            ; query starts: stop copying, skip to end
            load URL_END
            store D_SRC
            call decode-loop
        } else {
            load REQ_BUF
            load D_SRC
            add
            load
            push 37
            call eq
            if {
                ; '%' needs two more bytes
                load D_SRC
                push 2
                add
                load URL_END
                call lt
                if {
                    load REQ_BUF
                    load D_SRC
                    push 1
                    add
                    add
                    load
                    call hexval
                    store C_A
                    load REQ_BUF
                    load D_SRC
                    push 2
                    add
                    add
                    load
                    call hexval
                    store C_B
                    load C_A
                    push 0
                    call ge
                    if {
                        load C_B
                        push 0
                        call ge
                        if {
                            load C_A
                            push 16
                            mul
                            load C_B
                            add
                            store C_A
                            load PATH_BUF
                            load PATH_LEN
                            add
                            load C_A
                            swap
                            store
                            load PATH_LEN
                            push 1
                            add
                            store PATH_LEN
                            load D_SRC
                            push 3
                            add
                            store D_SRC
                            call decode-loop
                        } else {
                            call decode-copy-one
                        }
                    } else {
                        call decode-copy-one
                    }
                } else {
                    call decode-copy-one
                }
            } else {
                call decode-copy-one
            }
        }
    } else {
        id
    }

word decode-copy-one
    load REQ_BUF
    load D_SRC
    add
    load
    store C_A
    ; cap path at 2000 bytes
    load PATH_LEN
    push 2000
    call lt
    if {
        load PATH_BUF
        load PATH_LEN
        add
        load C_A
        store
        load PATH_LEN
        push 1
        add
        store PATH_LEN
    } else {
        id
    }
    load D_SRC
    push 1
    add
    store D_SRC
    call decode-loop

;  traversal guard: 1 if PATH_BUF contains ".." 
word check-traversal
    push 0
    store T_IDX
    call ct-loop

word ct-loop
    load T_IDX
    push 1
    add
    load PATH_LEN
    call le
    if {
        load PATH_BUF
        load T_IDX
        add
        load
        push 46
        call eq
        if {
            load PATH_BUF
            load T_IDX
            push 1
            add
            add
            load
            push 46
            call eq
            if {
                push 1
            } else {
                load T_IDX
                push 1
                add
                store T_IDX
                call ct-loop
            }
        } else {
            load T_IDX
            push 1
            add
            store T_IDX
            call ct-loop
        }
    } else {
        push 0
    }

;  output helpers: O_BASE/O_POS target, then send 
word out-reset
    load OUT_BUF
    store O_BASE
    push 0
    store O_POS

word body-reset
    load HTML_BUF
    store O_BASE
    push 0
    store O_POS

word out-byte
    load O_BASE
    load O_POS
    add
    swap
    store
    load O_POS
    push 1
    add
    store O_POS

word out-mem
    store O_LEN
    store O_SRC
    push 0
    store O_IDX
    call out-mem-loop

word out-mem-loop
    load O_IDX
    load O_LEN
    call lt
    if {
        load O_SRC
        load O_IDX
        add
        load
        call out-byte
        load O_IDX
        push 1
        add
        store O_IDX
        call out-mem-loop
    } else {
        id
    }

word out-num
    dup
    push 10
    div
    dup
    if {
        call out-num
        dup
        push 10
        mod
        push 48
        add
        call out-byte
        drop
    } else {
        drop
        push 48
        add
        call out-byte
    }

word out-crlf
    push 13
    call out-byte
    push 10
    call out-byte

word send-out
    load CLIENT_FD
    load OUT_BUF
    load HDR_LEN
    sock-send
    drop

word send-mem
    store S_LEN
    store S_SRC
    push 0
    store S_OFF
    call send-mem-loop

word send-mem-loop
    load S_OFF
    load S_LEN
    call lt
    if {
        load S_LEN
        load S_OFF
        sub
        dup
        push 4096
        call lt
        if {
            id
        } else {
            drop
            push 4096
        }
        store S_CHUNK
        load CLIENT_FD
        load S_SRC
        load S_OFF
        add
        load S_CHUNK
        sock-send
        dup
        push 0
        call gt
        if {
            load S_OFF
            add
            store S_OFF
            call send-mem-loop
        } else {
            drop
        }
    } else {
        id
    }

;  suffix match for MIME (ext_ptr ext_len -- r), case-insensitive 
word ends-with
    store E_LEN
    store E_PTR
    load E_LEN
    load PATH_LEN
    call gt
    if {
        push 0
    } else {
        push 0
        store E_IDX
        call ends-with-loop
    }

word ends-with-loop
    load E_IDX
    load E_LEN
    call lt
    if {
        load PATH_BUF
        load PATH_LEN
        load E_LEN
        sub
        load E_IDX
        add
        add
        load
        call tolow
        store E_PC
        load E_PTR
        load E_IDX
        add
        load
        call tolow
        store E_EC
        load E_PC
        load E_EC
        sub
        if {
            push 0
        } else {
            load E_IDX
            push 1
            add
            store E_IDX
            call ends-with-loop
        }
    } else {
        push 1
    }

;  Content-Type value (no CRLF) guessed from PATH suffix 
word mime-value
    push 46100
    push 5
    call ends-with
    if {
        push 116
        call out-byte
        push 101
        call out-byte
        push 120
        call out-byte
        push 116
        call out-byte
        push 47
        call out-byte
        push 104
        call out-byte
        push 116
        call out-byte
        push 109
        call out-byte
        push 108
        call out-byte
    } else {
        push 46110
        push 4
        call ends-with
        if {
            push 116
            call out-byte
            push 101
            call out-byte
            push 120
            call out-byte
            push 116
            call out-byte
            push 47
            call out-byte
            push 104
            call out-byte
            push 116
            call out-byte
            push 109
            call out-byte
            push 108
            call out-byte
        } else {
            push 46120
            push 4
            call ends-with
            if {
                push 116
                call out-byte
                push 101
                call out-byte
                push 120
                call out-byte
                push 116
                call out-byte
                push 47
                call out-byte
                push 99
                call out-byte
                push 115
                call out-byte
                push 115
                call out-byte
            } else {
                push 46130
                push 3
                call ends-with
                if {
                    push 116
                    call out-byte
                    push 101
                    call out-byte
                    push 120
                    call out-byte
                    push 116
                    call out-byte
                    push 47
                    call out-byte
                    push 106
                    call out-byte
                    push 97
                    call out-byte
                    push 118
                    call out-byte
                    push 97
                    call out-byte
                    push 115
                    call out-byte
                    push 99
                    call out-byte
                    push 114
                    call out-byte
                    push 105
                    call out-byte
                    push 112
                    call out-byte
                    push 116
                    call out-byte
                } else {
                    push 46140
                    push 5
                    call ends-with
                    if {
                        push 97
                        call out-byte
                        push 112
                        call out-byte
                        push 112
                        call out-byte
                        push 108
                        call out-byte
                        push 105
                        call out-byte
                        push 99
                        call out-byte
                        push 97
                        call out-byte
                        push 116
                        call out-byte
                        push 105
                        call out-byte
                        push 111
                        call out-byte
                        push 110
                        call out-byte
                        push 47
                        call out-byte
                        push 106
                        call out-byte
                        push 115
                        call out-byte
                        push 111
                        call out-byte
                        push 110
                        call out-byte
                    } else {
                        push 46150
                        push 4
                        call ends-with
                        if {
                            push 105
                            call out-byte
                            push 109
                            call out-byte
                            push 97
                            call out-byte
                            push 103
                            call out-byte
                            push 101
                            call out-byte
                            push 47
                            call out-byte
                            push 112
                            call out-byte
                            push 110
                            call out-byte
                            push 103
                            call out-byte
                        } else {
                            push 46160
                            push 4
                            call ends-with
                            if {
                                push 105
                                call out-byte
                                push 109
                                call out-byte
                                push 97
                                call out-byte
                                push 103
                                call out-byte
                                push 101
                                call out-byte
                                push 47
                                call out-byte
                                push 106
                                call out-byte
                                push 112
                                call out-byte
                                push 101
                           
                                call out-byte
                                push 103
                                call out-byte
                            } else {
                                push 46170
                                push 5
                                call ends-with
                                if {
                                    push 105
                                    call out-byte
                                    push 109
                                    call out-byte
                                    push 97
                                    call out-byte
                                    push 103
                                    call out-byte
                                    push 101
                                    call out-byte
                                    push 47
                                    call out-byte
                                    push 106
                                    call out-byte
                                    push 112
                                    call out-byte
                                    push 101
                                    call out-byte
                                    push 103
                                    call out-byte
                                } else {
                                    push 46180
                                    push 4
                                    call ends-with
                                    if {
                                        push 105
                                        call out-byte
                                        push 109
                                        call out-byte
                                        push 97
                                        call out-byte
                                        push 103
                                        call out-byte
                                        push 101
                                        call out-byte
                                        push 47
                                        call out-byte
                                        push 103
                                        call out-byte
                                        push 105
                                        call out-byte
                                        push 102
                                        call out-byte
                                    } else {
                                        push 46190
                                        push 4
                                        call ends-with
                                        if {
                                            push 105
                                            call out-byte
                                            push 109
                                            call out-byte
                                            push 97
                                            call out-byte
                                            push 103
                                            call out-byte
                                            push 101
                                            call out-byte
                                            push 47
                                            call out-byte
                                            push 120
                                            call out-byte
                                            push 45
                                            call out-byte
                                            push 105
                                            call out-byte
                                            push 99
                                            call out-byte
                                            push 111
                                            call out-byte
                                            push 110
                                            call out-byte
                                        } else {
                                            push 46195
                                            push 4
                                            call ends-with
                                            if {
                                                push 105
                                                call out-byte
                                                push 109
                                                call out-byte
                                                push 97
                                                call out-byte
                                                push 103
                                                call out-byte
                                                push 101
                                                call out-byte
                                                push 47
                                                call out-byte
                                                push 115
                                                call out-byte
                                                push 118
                                                call out-byte
                                                push 103
                                                call out-byte
                                                push 43
                                                call out-byte
                                                push 120
                                                call out-byte
                                                push 109
                                                call out-byte
                                                push 108
                                                call out-byte
                                            } else {
                                                push 46205
                                                push 4
                                                call ends-with
                                                if {
                                                    push 116
                                                    call out-byte
                                                    push 101
                                                    call out-byte
                                                    push 120
                                                    call out-byte
                                                    push 116
                                                    call out-byte
                                                    push 47
                                                    call out-byte
                                                    push 112
                                                    call out-byte
                                                    push 108
                                                    call out-byte
                                                    push 97
                                                    call out-byte
                                                    push 105
                                                    call out-byte
                                                    push 110
                                                    call out-byte
                                                } else {
                                                    push 46215
                                                    push 4
                                                    call ends-with
                                                    if {
                                                        push 97
                                                        call out-byte
                                                        push 112
                                                        call out-byte
                                                        push 112
                                                        call out-byte
                                                        push 108
                                                        call out-byte
                                                        push 105
                                                        call out-byte
                                                        push 99
                                                        call out-byte
                                                        push 97
                                                        call out-byte
                                                        push 116
                                                        call out-byte
                                                        push 105
                                                        call out-byte
                                                        push 111
                                                        call out-byte
                                                        push 110
                                                        call out-byte
                                                        push 47
                                                        call out-byte
                                                        push 112
                                                        call out-byte
                                                        push 100
                                                        call out-byte
                                                        push 102
                                                        call out-byte
                                                    } else {
                                                        push 46225
                                                        push 5
                                                        call ends-with
                                                        if {
                                                            push 97
                                                            call out-byte
                                                            push 112
                                                            call out-byte
                                                            push 112
                                                            call out-byte
                                                            push 108
                                                            call out-byte
                                                            push 105
                                                            call out-byte
                                                            push 99
                                                            call out-byte
                                                            push 97
                                                            call out-byte
                                                            push 116
                                                            call out-byte
                                                            push 105
                                                            call out-byte
                                                            push 111
                                                            call out-byte
                                                            push 110
                                                            call out-byte
                                                            push 47
                                                            call out-byte
                                                            push 119
                                                            call out-byte
                                                            push 97
                                                            call out-byte
                                                            push 115
                                                            call out-byte
                                                            push 109
                                                            call out-byte
                                                        } else {
                                                            push 97
                                                            call out-byte
                                                            push 112
                                                            call out-byte
                                                            push 112
                                                            call out-byte
                                                            push 108
                                                            call out-byte
                                                            push 105
                                                            call out-byte
                                                            push 99
                                                            call out-byte
                                                            push 97
                                                            call out-byte
                                                            push 116
                                                            call out-byte
                                                            push 105
                                                            call out-byte
                                                            push 111
                                                            call out-byte
                                                            push 110
                                                            call out-byte
                                                            push 47
                                                            call out-byte
                                                            push 111
                                                            call out-byte
                                                            push 99
                                                            call out-byte
                                                            push 116
                                                            call out-byte
                                                            push 101
                                                            call out-byte
                                                            push 116
                                                            call out-byte
                                                            push 45
                                                            call out-byte
                                                            push 115
                                                            call out-byte
                                                            push 116
                                                            call out-byte
                                                            push 114
                                                            call out-byte
                                                            push 101
                                                            call out-byte
                                                            push 97
                                                            call out-byte
                                                            push 109
                                                            call out-byte
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

;  status line + fixed headers (Content-Type/Length appended by caller) 
word send-status-line
    push 72
    call out-byte
    push 84
    call out-byte
    push 84
    call out-byte
    push 80
    call out-byte
    push 47
    call out-byte
    push 49
    call out-byte
    push 46
    call out-byte
    push 48
    call out-byte
    push 32
    call out-byte
    load STATUS
    call out-num
    push 32
    call out-byte
    load STATUS
    push 200
    call eq
    if {
    push 79
    call out-byte
    push 75
    call out-byte
    } else {
    load STATUS
    push 301
    call eq
    if {
    push 77
    call out-byte
    push 111
    call out-byte
    push 118
    call out-byte
    push 101
    call out-byte
    push 100
    call out-byte
    push 32
    call out-byte
    push 80
    call out-byte
    push 101
    call out-byte
    push 114
    call out-byte
    push 109
    call out-byte
    push 97
    call out-byte
    push 110
    call out-byte
    push 101
    call out-byte
    push 110
    call out-byte
    push 116
    call out-byte
    push 108
    call out-byte
    push 121
    call out-byte
    } else {
    load STATUS
    push 302
    call eq
    if {
    push 70
    call out-byte
    push 111
    call out-byte
    push 117
    call out-byte
    push 110
    call out-byte
    push 100
    call out-byte
    } else {
    load STATUS
    push 403
    call eq
    if {
    push 70
    call out-byte
    push 111
    call out-byte
    push 114
    call out-byte
    push 98
    call out-byte
    push 105
    call out-byte
    push 100
    call out-byte
    push 100
    call out-byte
    push 101
    call out-byte
    push 110
    call out-byte
    } else {
    load STATUS
    push 404
    call eq
    if {
    push 70
    call out-byte
    push 105
    call out-byte
    push 108
    call out-byte
    push 101
    call out-byte
    push 32
    call out-byte
    push 110
    call out-byte
    push 111
    call out-byte
    push 116
    call out-byte
    push 32
    call out-byte
    push 102
    call out-byte
    push 111
    call out-byte
    push 117
    call out-byte
    push 110
    call out-byte
    push 100
    call out-byte
    } else {
    push 85
    call out-byte
    push 110
    call out-byte
    push 115
    call out-byte
    push 117
    call out-byte
    push 112
    call out-byte
    push 112
    call out-byte
    push 111
    call out-byte
    push 114
    call out-byte
    push 116
    call out-byte
    push 101
    call out-byte
    push 100
    call out-byte
    push 32
    call out-byte
    push 109
    call out-byte
    push 101
    call out-byte
    push 116
    call out-byte
    push 104
    call out-byte
    push 111
    call out-byte
    push 100
    call out-byte
    }
    }
    }
    }
    }
    call out-crlf
word send-server-headers
    push 83
    call out-byte
    push 101
    call out-byte
    push 114
    call out-byte
    push 118
    call out-byte
    push 101
    call out-byte
    push 114
    call out-byte
    push 58
    call out-byte
    push 32
    call out-byte
    push 73
    call out-byte
    push 110
    call out-byte
    push 118
    call out-byte
    push 105
    call out-byte
    push 115
    call out-byte
    push 105
    call out-byte
    push 98
    call out-byte
    push 108
    call out-byte
    push 101
    call out-byte
    push 73
    call out-byte
    push 110
    call out-byte
    push 107
    call out-byte
    push 47
    call out-byte
    push 49
    call out-byte
    push 46
    call out-byte
    push 50
    call out-byte
    push 46
    call out-byte
    push 48
    call out-byte
    call out-crlf
    push 68
    call out-byte
    push 97
    call out-byte
    push 116
    call out-byte
    push 101
    call out-byte
    push 58
    call out-byte
    push 32
    call out-byte
    push 84
    call out-byte
    push 104
    call out-byte
    push 117
    call out-byte
    push 44
    call out-byte
    push 32
    call out-byte
    push 48
    call out-byte
    push 49
    call out-byte
    push 32
    call out-byte
    push 74
    call out-byte
    push 97
    call out-byte
    push 110
    call out-byte
    push 32
    call out-byte
    push 49
    call out-byte
    push 57
    call out-byte
    push 55
    call out-byte
    push 48
    call out-byte
    push 32
    call out-byte
    push 48
    call out-byte
    push 48
    call out-byte
    push 58
    call out-byte
    push 48
    call out-byte
    push 48
    call out-byte
    push 58
    call out-byte
    push 48
    call out-byte
    push 48
    call out-byte
    push 32
    call out-byte
    push 71
    call out-byte
    push 77
    call out-byte
    push 84
    call out-byte
    call out-crlf
    push 67
    call out-byte
    push 111
    call out-byte
    push 110
    call out-byte
    push 110
    call out-byte
    push 101
    call out-byte
    push 99
    call out-byte
    push 116
    call out-byte
    push 105
    call out-byte
    push 111
    call out-byte
    push 110
    call out-byte
    push 58
    call out-byte
    push 32
    call out-byte
    push 99
    call out-byte
    push 108
    call out-byte
    push 111
    call out-byte
    push 115
    call out-byte
    push 101
    call out-byte
    call out-crlf

word send-content-type
    push 67
    call out-byte
    push 111
    call out-byte
    push 110
    call out-byte
    push 116
    call out-byte
    push 101
    call out-byte
    push 110
    call out-byte
    push 116
    call out-byte
    push 45
    call out-byte
    push 116
    call out-byte
    push 121
    call out-byte
    push 112
    call out-byte
    push 101
    call out-byte
    push 58
    call out-byte
    push 32
    call out-byte
    call mime-value
    call out-crlf

word send-html-content-type
    push 67
    call out-byte
    push 111
    call out-byte
    push 110
    call out-byte
    push 116
    call out-byte
    push 101
    call out-byte
    push 110
    call out-byte
    push 116
    call out-byte
    push 45
    call out-byte
    push 116
    call out-byte
    push 121
    call out-byte
    push 112
    call out-byte
    push 101
    call out-byte
    push 58
    call out-byte
    push 32
    call out-byte
    push 116
    call out-byte
    push 101
    call out-byte
    push 120
    call out-byte
    push 116
    call out-byte
    push 47
    call out-byte
    push 104
    call out-byte
    push 116
    call out-byte
    push 109
    call out-byte
    push 108
    call out-byte
    push 59
    call out-byte
    push 32
    call out-byte
    push 99
    call out-byte
    push 104
    call out-byte
    push 97
    call out-byte
    push 114
    call out-byte
    push 115
    call out-byte
    push 101
    call out-byte
    push 116
    call out-byte
    push 61
    call out-byte
    push 117
    call out-byte
    push 116
    call out-byte
    push 102
    call out-byte
    push 45
    call out-byte
    push 56
    call out-byte
    call out-crlf

word send-content-length
    push 67
    call out-byte
    push 111
    call out-byte
    push 110
    call out-byte
    push 116
    call out-byte
    push 101
    call out-byte
    push 110
    call out-byte
    push 116
    call out-byte
    push 45
    call out-byte
    push 76
    call out-byte
    push 101
    call out-byte
    push 110
    call out-byte
    push 103
    call out-byte
    push 116
    call out-byte
    push 104
    call out-byte
    push 58
    call out-byte
    push 32
    call out-byte
    load BODY_LEN
    call out-num
    call out-crlf

;  error pages: STATUS selects 403/404/501 body+headers 
word send-error
    call body-reset
    push 60
    call out-byte
    push 104
    call out-byte
    push 116
    call out-byte
    push 109
    call out-byte
    push 108
    call out-byte
    push 62
    call out-byte
    push 60
    call out-byte
    push 104
    call out-byte
    push 49
    call out-byte
    push 62
    call out-byte
    load STATUS
    call out-num
    push 32
    call out-byte
    load STATUS
    push 403
    call eq
    if {
        push 70
        call out-byte
        push 111
        call out-byte
        push 114
        call out-byte
        push 98
        call out-byte
        push 105
        call out-byte
        push 100
        call out-byte
        push 100
        call out-byte
        push 101
        call out-byte
        push 110
        call out-byte
    } else {
        load STATUS
        push 404
        call eq
        if {
            push 70
            call out-byte
            push 105
            call out-byte
            push 108
            call out-byte
            push 101
            call out-byte
            push 32
            call out-byte
            push 110
            call out-byte
            push 111
            call out-byte
            push 116
            call out-byte
            push 32
            call out-byte
            push 102
            call out-byte
            push 111
            call out-byte
            push 117
            call out-byte
            push 110
            call out-byte
            push 100
            call out-byte
        } else {
            push 85
            call out-byte
            push 110
            call out-byte
            push 115
            call out-byte
            push 117
            call out-byte
            push 112
            call out-byte
            push 112
            call out-byte
            push 111
            call out-byte
            push 114
            call out-byte
            push 116
            call out-byte
            push 101
            call out-byte
            push 100
            call out-byte
            push 32
            call out-byte
            push 109
            call out-byte
            push 101
            call out-byte
            push 116
            call out-byte
            push 104
            call out-byte
            push 111
            call out-byte
            push 100
            call out-byte
        }
    }
    push 60
    call out-byte
    push 47
    call out-byte
    push 104
    call out-byte
    push 49
    call out-byte
    push 62
    call out-byte
    push 60
    call out-byte
    push 112
    call out-byte
    push 62
    call out-byte
    load STATUS
    push 403
    call eq
    if {
        push 80
        call out-byte
        push 97
        call out-byte
        push 116
        call out-byte
        push 104
        call out-byte
        push 32
        call out-byte
        push 102
        call out-byte
        push 111
        call out-byte
        push 114
        call out-byte
        push 98
        call out-byte
        push 105
        call out-byte
        push 100
        call out-byte
        push 100
        call out-byte
        push 101
        call out-byte
        push 110
        call out-byte
        push 46
        call out-byte
    } else {
        load STATUS
        push 404
        call eq
        if {
            push 70
            call out-byte
            push 105
            call out-byte
            push 108
            call out-byte
            push 101
            call out-byte
            push 32
            call out-byte
            push 110
            call out-byte
            push 111
            call out-byte
            push 116
            call out-byte
            push 32
            call out-byte
            push 102
            call out-byte
            push 111
            call out-byte
            push 117
            call out-byte
            push 110
            call out-byte
            push 100
            call out-byte
            push 46
            call out-byte
        } else {
            push 79
            call out-byte
            push 110
            call out-byte
            push 108
            call out-byte
            push 121
            call out-byte
            push 32
            call out-byte
            push 71
            call out-byte
            push 69
            call out-byte
            push 84
            call out-byte
            push 32
            call out-byte
            push 97
            call out-byte
            push 110
            call out-byte
            push 100
            call out-byte
            push 32
            call out-byte
            push 72
            call out-byte
            push 69
            call out-byte
            push 65
            call out-byte
            push 68
            call out-byte
            push 32
            call out-byte
            push 97
            call out-byte
            push 114
            call out-byte
            push 101
            call out-byte
            push 32
            call out-byte
            push 115
            call out-byte
            push 117
            call out-byte
            push 112
            call out-byte
            push 112
            call out-byte
            push 111
            call out-byte
            push 114
            call out-byte
            push 116
            call out-byte
            push 101
            call out-byte
            push 100
            call out-byte
            push 46
            call out-byte
        }
    }
    push 60
    call out-byte
    push 47
    call out-byte
    push 112
    call out-byte
    push 62
    call out-byte
    push 60
    call out-byte
    push 47
    call out-byte
    push 104
    call out-byte
    push 116
    call out-byte
    push 109
    call out-byte
    push 108
    call out-byte
    push 62
    call out-byte
    push 10
    call out-byte
    load O_POS
    store BODY_LEN
    call out-reset
    call send-status-line
    call send-server-headers
    call send-html-content-type
    call send-content-length
    call out-crlf
    load O_POS
    store HDR_LEN
    call send-out
    load HTML_BUF
    load BODY_LEN
    call send-mem
    load STATUS
    call log-request

word send-301
    call out-reset
    call send-status-line
    push 76
    call out-byte
    push 111
    call out-byte
    push 99
    call out-byte
    push 97
    call out-byte
    push 116
    call out-byte
    push 105
    call out-byte
    push 111
    call out-byte
    push 110
    call out-byte
    push 58
    call out-byte
    push 32
    call out-byte
    push 47
    call out-byte
    load PATH_BUF
    load PATH_LEN
    call out-mem
    push 47
    call out-byte
    call out-crlf
    call send-server-headers
    push 67
    call out-byte
    push 111
    call out-byte
    push 110
    call out-byte
    push 116
    call out-byte
    push 101
    call out-byte
    push 110
    call out-byte
    push 116
    call out-byte
    push 45
    call out-byte
    push 76
    call out-byte
    push 101
    call out-byte
    push 110
    call out-byte
    push 103
    call out-byte
    push 116
    call out-byte
    push 104
    call out-byte
    push 58
    call out-byte
    push 32
    call out-byte
    push 48
    call out-byte
    call out-crlf
    call out-crlf
    load O_POS
    store HDR_LEN
    call send-out
    load STATUS
    call log-request

;  routing 
word route
    load METHOD
    push 2
    call eq
    if {
        push 501
        store STATUS
        call send-error
    } else {
        load METHOD
        push 3
        call eq
        if {
            call is-sign-path
            if {
                call handle-sign
            } else {
                push 404
                store STATUS
                call send-error
            }
        } else {
            call is-root
            if {
                call serve-gb
            } else {
                call check-traversal
                if {
                    push 403
                    store STATUS
                    call send-error
                } else {
                    load PATH_BUF
                    load PATH_LEN
                    file-stat
                    store FS_STAT
                    load FS_STAT
                    push -1
                    call eq
                    if {
                        push 404
                        store STATUS
                        call send-error
                    } else {
                        load FS_STAT
                        push -2
                        call eq
                        if {
                            call handle-dir
                        } else {
                            load FS_STAT
                            store FILE_SIZE
                            call handle-file
                        }
                    }
                }
            }
        }
    }
word handle-file
    load PATH_BUF
    load PATH_LEN
    push 0
    file-open
    store FILE_FD
    load FILE_FD
    push -1
    call eq
    if {
        push 404
        store STATUS
        call send-error
    } else {
        push 200
        store STATUS
        load FILE_SIZE
        store BODY_LEN
        call out-reset
        call send-status-line
        call send-server-headers
        call send-content-type
        call send-content-length
        call out-crlf
        load O_POS
        store HDR_LEN
        call send-out
        load METHOD
        push 0
        call eq
        if {
            call send-file-loop
        } else {
            id
        }
        load FILE_FD
        close
        drop
        load STATUS
        call log-request
    }

word send-file-loop
    load FILE_FD
    load FILE_BUF
    push 4096
    file-read
    store F_N
    load F_N
    push 0
    call gt
    if {
        load CLIENT_FD
        load FILE_BUF
        load F_N
        sock-send
        drop
        call send-file-loop
    } else {
        id
    }

word handle-dir
    load HAS_SLASH
    push 0
    call eq
    if {
        push 301
        store STATUS
        call send-301
    } else {
        call try-index
        if {
            id
        } else {
            call serve-dir-listing
        }
    }

word build-index-path
    push 0
    store I_LEN
    call build-index-copy
    load PATH_BUF
    load
    push 46
    call eq
    if {
        load PATH_LEN
        push 1
        call eq
        if {
            push 0
            store I_LEN
        } else {
            push 47
            call index-byte
        }
    } else {
        push 47
        call index-byte
    }
    push 105
    call index-byte
    push 110
    call index-byte
    push 100
    call index-byte
    push 101
    call index-byte
    push 120
    call index-byte
    push 46
    call index-byte
    push 104
    call index-byte
    push 116
    call index-byte
    push 109
    call index-byte
    push 108
    call index-byte

word build-index-copy
    push 0
    store T_IDX
    call build-index-copy-loop

word build-index-copy-loop
    load T_IDX
    load PATH_LEN
    call lt
    if {
        load PATH_BUF
        load T_IDX
        add
        load
        call index-byte
        load T_IDX
        push 1
        add
        store T_IDX
        call build-index-copy-loop
    } else {
        id
    }

word index-byte
    load INDEX_BUF
    load I_LEN
    add
    swap
    store
    load I_LEN
    push 1
    add
    store I_LEN

word try-index
    call build-index-path
    call serve-index-stat
    if {
        push 1
    } else {
        call build-htm-path
        call serve-index-stat
    }

word serve-index-stat
    load INDEX_BUF
    load I_LEN
    file-stat
    store FS_STAT
    load FS_STAT
    push -1
    call eq
    if {
        push 0
    } else {
        load FS_STAT
        push -2
        call eq
        if {
            push 0
        } else {
            load FS_STAT
            store FILE_SIZE
            load INDEX_BUF
            load I_LEN
            push 0
            file-open
            store FILE_FD
            load FILE_FD
            push -1
            call eq
            if {
                push 0
            } else {
                push 200
                store STATUS
                load FILE_SIZE
                store BODY_LEN
                call out-reset
                call send-status-line
                call send-server-headers
                call send-html-content-type
                call send-content-length
                call out-crlf
                load O_POS
                store HDR_LEN
                call send-out
                load METHOD
                push 0
                call eq
                if {
                    call send-file-loop
                } else {
                    id
                }
                load FILE_FD
                close
                drop
                load STATUS
                call log-request
                push 1
            }
        }
    }

word build-htm-path
    push 0
    store I_LEN
    call build-index-copy
    load PATH_BUF
    load
    push 46
    call eq
    if {
        load PATH_LEN
        push 1
        call eq
        if {
            push 0
            store I_LEN
        } else {
            push 47
            call index-byte
        }
    } else {
        push 47
        call index-byte
    }
    push 105
    call index-byte
    push 110
    call index-byte
    push 100
    call index-byte
    push 101
    call index-byte
    push 120
    call index-byte
    push 46
    call index-byte
    push 104
    call index-byte
    push 116
    call index-byte
    push 109
    call index-byte

word serve-dir-listing
    load PATH_BUF
    load PATH_LEN
    load DIR_BUF
    push 8000
    dir-list
    store DIR_LEN
    load DIR_LEN
    push 0
    call ge
    if {
        id
    } else {
        push 0
        store DIR_LEN
    }
    call body-reset
    push 60
    call out-byte
    push 104
    call out-byte
    push 116
    call out-byte
    push 109
    call out-byte
    push 108
    call out-byte
    push 62
    call out-byte
    push 10
    call out-byte
    push 60
    call out-byte
    push 104
    call out-byte
    push 101
    call out-byte
    push 97
    call out-byte
    push 100
    call out-byte
    push 62
    call out-byte
    push 60
    call out-byte
    push 116
    call out-byte
    push 105
    call out-byte
    push 116
    call out-byte
    push 108
    call out-byte
    push 101
    call out-byte
    push 62
    call out-byte
    push 68
    call out-byte
    push 105
    call out-byte
    push 114
    call out-byte
    push 101
    call out-byte
    push 99
    call out-byte
    push 116
    call out-byte
    push 111
    call out-byte
    push 114
    call out-byte
    push 121
    call out-byte
    push 32
    call out-byte
    push 108
    call out-byte
    push 105
    call out-byte
    push 115
    call out-byte
    push 116
    call out-byte
    push 105
    call out-byte
    push 110
    call out-byte
    push 103
    call out-byte
    push 32
    call out-byte
    push 102
    call out-byte
    push 111
    call out-byte
    push 114
    call out-byte
    push 32
    call out-byte
    push 47
    call out-byte
    load PATH_BUF
    load
    push 46
    call eq
    if {
        load PATH_LEN
        push 1
        call eq
        if {
            id
        } else {
            load PATH_BUF
            load PATH_LEN
            call out-mem
        }
    } else {
        load PATH_BUF
        load PATH_LEN
        call out-mem
    }
    push 60
    call out-byte
    push 47
    call out-byte
    push 116
    call out-byte
    push 105
    call out-byte
    push 116
    call out-byte
    push 108
    call out-byte
    push 101
    call out-byte
    push 62
    call out-byte
    push 60
    call out-byte
    push 47
    call out-byte
    push 104
    call out-byte
    push 101
    call out-byte
    push 97
    call out-byte
    push 100
    call out-byte
    push 62
    call out-byte
    push 10
    call out-byte
    push 60
    call out-byte
    push 98
    call out-byte
    push 111
    call out-byte
    push 100
    call out-byte
    push 121
    call out-byte
    push 62
    call out-byte
    push 10
    call out-byte
    push 60
    call out-byte
    push 104
    call out-byte
    push 49
    call out-byte
    push 62
    call out-byte
    push 68
    call out-byte
    push 105
    call out-byte
    push 114
    call out-byte
    push 101
    call out-byte
    push 99
    call out-byte
    push 116
    call out-byte
    push 111
    call out-byte
    push 114
    call out-byte
    push 121
    call out-byte
    push 32
    call out-byte
    push 108
    call out-byte
    push 105
    call out-byte
    push 115
    call out-byte
    push 116
    call out-byte
    push 105
    call out-byte
    push 110
    call out-byte
    push 103
    call out-byte
    push 32
    call out-byte
    push 102
    call out-byte
    push 111
    call out-byte
    push 114
    call out-byte
    push 32
    call out-byte
    push 47
    call out-byte
    load PATH_BUF
    load
    push 46
    call eq
    if {
        load PATH_LEN
        push 1
        call eq
        if {
            id
        } else {
            load PATH_BUF
            load PATH_LEN
            call out-mem
        }
    } else {
        load PATH_BUF
        load PATH_LEN
        call out-mem
    }
    push 60
    call out-byte
    push 47
    call out-byte
    push 104
    call out-byte
    push 49
    call out-byte
    push 62
    call out-byte
    push 10
    call out-byte
    push 60
    call out-byte
    push 104
    call out-byte
    push 114
    call out-byte
    push 62
    call out-byte
    push 10
    call out-byte
    push 60
    call out-byte
    push 117
    call out-byte
    push 108
    call out-byte
    push 62
    call out-byte
    push 10
    call out-byte
    push 0
    store G_IDX
    call dir-entries-loop
    push 60
    call out-byte
    push 47
    call out-byte
    push 117
    call out-byte
    push 108
    call out-byte
    push 62
    call out-byte
    push 10
    call out-byte
    push 60
    call out-byte
    push 104
    call out-byte
    push 114
    call out-byte
    push 62
    call out-byte
    push 10
    call out-byte
    push 60
    call out-byte
    push 47
    call out-byte
    push 98
    call out-byte
    push 111
    call out-byte
    push 100
    call out-byte
    push 121
    call out-byte
    push 62
    call out-byte
    push 10
    call out-byte
    push 60
    call out-byte
    push 47
    call out-byte
    push 104
    call out-byte
    push 116
    call out-byte
    push 109
    call out-byte
    push 108
    call out-byte
    push 62
    call out-byte
    push 10
    call out-byte
    load O_POS
    store BODY_LEN
    push 200
    store STATUS
    call out-reset
    call send-status-line
    call send-server-headers
    call send-html-content-type
    call send-content-length
    call out-crlf
    load O_POS
    store HDR_LEN
    call send-out
    load METHOD
    push 0
    call eq
    if {
        load HTML_BUF
        load BODY_LEN
        call send-mem
    } else {
        id
    }
    load STATUS
    call log-request

word dir-entries-loop
    load G_IDX
    load DIR_LEN
    call lt
    if {
        load DIR_BUF
        load G_IDX
        add
        load
        push 10
        call eq
        if {
            load G_IDX
            push 1
            add
            store G_IDX
            call dir-entries-loop
        } else {
            load G_IDX
            store G_OFF
            call entry-scan
            load G_OFF
            load G_END
            call lt
            if {
                call emit-entry
            } else {
                id
            }
            load G_END
            store G_IDX
            call dir-entries-loop
        }
    } else {
        id
    }

word entry-scan
    load G_OFF
    store G_END
    call entry-scan-loop

word entry-scan-loop
    load G_END
    load DIR_LEN
    call lt
    if {
        load DIR_BUF
        load G_END
        add
        load
        push 10
        call eq
        if {
            id
        } else {
            load G_END
            push 1
            add
            store G_END
            call entry-scan-loop
        }
    } else {
        id
    }

word emit-entry
    push 60
    call out-byte
    push 108
    call out-byte
    push 105
    call out-byte
    push 62
    call out-byte
    push 60
    call out-byte
    push 97
    call out-byte
    push 32
    call out-byte
    push 104
    call out-byte
    push 114
    call out-byte
    push 101
    call out-byte
    push 102
    call out-byte
    push 61
    call out-byte
    push 34
    call out-byte
    load G_OFF
    store T_IDX
    call emit-href-loop
    push 34
    call out-byte
    push 62
    call out-byte
    load G_OFF
    store T_IDX
    call emit-text-loop
    push 60
    call out-byte
    push 47
    call out-byte
    push 97
    call out-byte
    push 62
    call out-byte
    push 60
    call out-byte
    push 47
    call out-byte
    push 108
    call out-byte
    push 105
    call out-byte
    push 62
    call out-byte
    push 10
    call out-byte

word emit-href-loop
    load T_IDX
    load G_END
    call lt
    if {
        load DIR_BUF
        load T_IDX
        add
        load
        push 32
        call eq
        if {
            push 37
            call out-byte
            push 50
            call out-byte
            push 48
            call out-byte
        } else {
            load DIR_BUF
            load T_IDX
            add
            load
            push 38
            call eq
            if {
                push 37
                call out-byte
                push 50
                call out-byte
                push 54
                call out-byte
            } else {
                load DIR_BUF
                load T_IDX
                add
                load
                call out-byte
            }
        }
        load T_IDX
        push 1
        add
        store T_IDX
        call emit-href-loop
    } else {
        id
    }

word emit-text-loop
    load T_IDX
    load G_END
    call lt
    if {
        load DIR_BUF
        load T_IDX
        add
        load
        push 38
        call eq
        if {
            push 38
            call out-byte
            push 97
            call out-byte
            push 109
            call out-byte
            push 112
            call out-byte
            push 59
            call out-byte
        } else {
            load DIR_BUF
            load T_IDX
            add
            load
            push 60
            call eq
            if {
                push 38
                call out-byte
                push 108
                call out-byte
                push 116
                call out-byte
                push 59
                call out-byte
            } else {
                load DIR_BUF
                load T_IDX
                add
                load
                push 62
                call eq
                if {
                    push 38
                    call out-byte
                    push 103
                    call out-byte
                    push 116
                    call out-byte
                    push 59
                    call out-byte
                } else {
                    load DIR_BUF
                    load T_IDX
                    add
                    load
                    call out-byte
                }
            }
        }
        load T_IDX
        push 1
        add
        store T_IDX
        call emit-text-loop
    } else {
        id
    }

;  access log to stdout: METHOD SP PATH SP CODE LF 
word log-request
    store STATUS
    load METHOD
    push 0
    call eq
    if {
    push 71
    char-out
    push 69
    char-out
    push 84
    char-out
    } else {
    load METHOD
    push 1
    call eq
    if {
    push 72
    char-out
    push 69
    char-out
    push 65
    char-out
    push 68
    char-out
    } else {
    load METHOD
    push 3
    call eq
    if {
    push 80
    char-out
    push 79
    char-out
    push 83
    char-out
    push 84
    char-out
    } else {
    push 63
    char-out
    }
    }
    }
    push 32
    char-out
    push 47
    char-out

    load PATH_BUF
    load
    push 46
    call eq
    if {
        load PATH_LEN
        push 1
        call eq
        if {
            id
        } else {
            push 0
            store L_IDX
            call log-path-loop
        }
    } else {
        push 0
        store L_IDX
        call log-path-loop
    }
    push 32
    char-out
    load STATUS
    num-out
    push 10
    char-out
word log-path-loop
    load L_IDX
    load PATH_LEN
    call lt
    if {
        load PATH_BUF
        load L_IDX
        add
        load
        char-out
        load L_IDX
        push 1
        add
        store L_IDX
        call log-path-loop
    } else {
        id
    }
word is-root
    load PATH_LEN
    push 1
    call eq
    if {
        load PATH_BUF
        load
        push 46
        call eq
        if { push 1 } else { push 0 }
    } else {
        push 0
    }

word is-sign-path
    load PATH_LEN
    push 4
    call eq
    if {
        load PATH_BUF
        load
        push 115
        call eq
        if {
            load PATH_BUF
            push 1
            add
            load
            push 105
            call eq
            if {
                load PATH_BUF
                push 2
                add
                load
                push 103
                call eq
                if {
                    load PATH_BUF
                    push 3
                    add
                    load
                    push 110
                    call eq
                    if { push 1 } else { push 0 }
                } else { push 0 }
            } else { push 0 }
        } else { push 0 }
    } else {
        push 0
    }

word byte
    ; ( c -- ) store char at mem[WPTR], WPTR++
    load WPTR
    swap
    store
    load WPTR
    push 1
    add
    store WPTR

word init-gb
    ; guestbook.txt
    push 103
    call byte
    push 117
    call byte
    push 101
    call byte
    push 115
    call byte
    push 116
    call byte
    push 98
    call byte
    push 111
    call byte
    push 111
    call byte
    push 107
    call byte
    push 46
    call byte
    push 116
    call byte
    push 120
    call byte
    push 116
    call byte

word handle-sign
    call find-msg-param
    load MSG_EQ
    push 0
    call ge
    if {
        call decode-msg
        call build-entry
        call append-entry
    } else {
        id
    }
    push 302
    store STATUS
    call send-302

word send-302
    push 0
    store BODY_LEN
    call out-reset
    call send-status-line
    push 76
    call out-byte
    push 111
    call out-byte
    push 99
    call out-byte
    push 97
    call out-byte
    push 116
    call out-byte
    push 105
    call out-byte
    push 111
    call out-byte
    push 110
    call out-byte
    push 58
    call out-byte
    push 32
    call out-byte
    push 47
    call out-byte
    call out-crlf
    call send-server-headers
    call send-content-length
    call out-crlf
    load O_POS
    store HDR_LEN
    call send-out
    load STATUS
    call log-request

word find-msg-param
    push -1
    store MSG_EQ
    load HEND
    store F_P
    call fmp-loop

word fmp-loop
    load MSG_EQ
    push 0
    call ge
    if {
        id
    } else {
        load F_P
        push 4
        add
        load REQ_LEN
        call le
        if {
            load REQ_BUF
            load F_P
            add
            load
            push 109
            call eq
            if {
                load REQ_BUF
                load F_P
                push 1
                add
                load
                push 115
                call eq
                if {
                    load REQ_BUF
                    load F_P
                    push 2
                    add
                    load
                    push 103
                    call eq
                    if {
                        load REQ_BUF
                        load F_P
                        push 3
                        add
                        load
                        push 61
                        call eq
                        if {
                            load F_P
                            push 3
                            add
                            store MSG_EQ
                        } else {
                            call fmp-adv
                        }
                    } else {
                        call fmp-adv
                    }
                } else {
                    call fmp-adv
                }
            } else {
                call fmp-adv
            }
        } else {
            id
        }
    }

word fmp-adv
    load F_P
    push 1
    add
    store F_P
    call fmp-loop

word decode-msg
    push 0
    store MSG_LEN
    load MSG_EQ
    push 1
    add
    store D_SRC
    load REQ_LEN
    store D_END
    call dm-loop

word dm-loop
    load D_SRC
    load D_END
    call lt
    if {
        load MSG_LEN
        push 1024
        call lt
        if {
            load REQ_BUF
            load D_SRC
            add
            load
            store CH
            load CH
            push 38
            call eq
            if {
                ; '&' ends the field value
                load D_END
                store D_SRC
                call dm-loop
            } else {
                load CH
                push 43
                call eq
                if {
                    push 32
                    call dm-push
                } else {
                    load CH
                    push 37
                    call eq
                    if {
                        call dm-decode-hex
                    } else {
                        call dm-copy
                    }
                }
            }
        } else {
            load D_SRC
            push 1
            add
            store D_SRC
            call dm-loop
        }
    } else {
        id
    }

word dm-decode-hex
    ; CH == '%': needs two more bytes within D_END
    load D_SRC
    push 2
    add
    load D_END
    call lt
    if {
        load REQ_BUF
        load D_SRC
        push 1
        add
        add
        load
        call hexval
        store C_A
        load REQ_BUF
        load D_SRC
        push 2
        add
        add
        load
        call hexval
        store C_B
        load C_A
        push 0
        call ge
        if {
            load C_B
            push 0
            call ge
            if {
                load C_A
                push 16
                mul
                load C_B
                add
                call dm-push3
            } else {
                call dm-copy
            }
        } else {
            call dm-copy
        }
    } else {
        call dm-copy
    }

word dm-push
    ; ( val -- ): append, advance 1, continue
    load GB_MSG
    load MSG_LEN
    add
    swap
    store
    load MSG_LEN
    push 1
    add
    store MSG_LEN
    load D_SRC
    push 1
    add
    store D_SRC
    call dm-loop

word dm-push3
    ; ( val -- ): append, advance 3, continue
    load GB_MSG
    load MSG_LEN
    add
    swap
    store
    load MSG_LEN
    push 1
    add
    store MSG_LEN
    load D_SRC
    push 3
    add
    store D_SRC
    call dm-loop

word dm-copy
    load CH
    call dm-push

word eput
    ; ( c -- ): mem[EPTR] = c; EPTR++
    load EPTR
    swap
    store
    load EPTR
    push 1
    add
    store EPTR

word build-entry
    push 62000
    store EPTR
    ; "<li>"
    push 60
    call eput
    push 108
    call eput
    push 105
    call eput
    push 62
    call eput
    push 0
    store E_I
    call be-msg-loop
    ; "</li>\n"
    push 60
    call eput
    push 47
    call eput
    push 108
    call eput
    push 105
    call eput
    push 62
    call eput
    push 10
    call eput
    load EPTR
    push 62000
    sub
    store ELEN

word be-msg-loop
    load E_I
    load MSG_LEN
    call lt
    if {
        load GB_MSG
        load E_I
        add
        load
        store CH
        load CH
        push 38
        call eq
        if {
            push 38
            call eput
            push 97
            call eput
            push 109
            call eput
            push 112
            call eput
            push 59
            call eput
        } else {
            load CH
            push 60
            call eq
            if {
                push 38
                call eput
                push 108
                call eput
                push 116
                call eput
                push 59
                call eput
            } else {
                load CH
                push 62
                call eq
                if {
                    push 38
                    call eput
                    push 103
                    call eput
                    push 116
                    call eput
                    push 59
                    call eput
                } else {
                    load CH
                    push 10
                    call eq
                    if {
                        push 32
                        call eput
                    } else {
                        load CH
                        push 13
                        call eq
                        if {
                            push 32
                            call eput
                        } else {
                            load CH
                            call eput
                        }
                    }
                }
            }
        }
        load E_I
        push 1
        add
        store E_I
        call be-msg-loop
    } else {
        id
    }

word append-entry
    load GPATH
    push 13
    file-append
    store GFD
    load GFD
    push 0
    call lt
    if {
        id
    } else {
        load GFD
        push 62000
        load ELEN
        file-write
        drop
        load GFD
        close
        drop
    }

word serve-gb
    call body-reset
    call gb-prefix
    call gb-entries
    call gb-suffix
    load O_POS
    store BODY_LEN
    push 200
    store STATUS
    call out-reset
    call send-status-line
    call send-server-headers
    call send-html-content-type
    call send-content-length
    call out-crlf
    load O_POS
    store HDR_LEN
    call send-out
    load METHOD
    push 0
    call eq
    if {
        load HTML_BUF
        load BODY_LEN
        call send-mem
    } else {
        id
    }
    load STATUS
    call log-request

word gb-prefix
            push 60
        call out-byte
        push 104
        call out-byte
        push 116
        call out-byte
        push 109
        call out-byte
        push 108
        call out-byte
        push 62
        call out-byte
        push 10
        call out-byte
        push 60
        call out-byte
        push 104
        call out-byte
        push 101
        call out-byte
        push 97
        call out-byte
        push 100
        call out-byte
        push 62
        call out-byte
        push 60
        call out-byte
        push 116
        call out-byte
        push 105
        call out-byte
        push 116
        call out-byte
        push 108
        call out-byte
        push 101
        call out-byte
        push 62
        call out-byte
        push 73
        call out-byte
        push 110
        call out-byte
        push 118
        call out-byte
        push 105
        call out-byte
        push 115
        call out-byte
        push 105
        call out-byte
        push 98
        call out-byte
        push 108
        call out-byte
        push 101
        call out-byte
        push 32
        call out-byte
        push 73
        call out-byte
        push 110
        call out-byte
        push 107
        call out-byte
        push 32
        call out-byte
        push 71
        call out-byte
        push 117
        call out-byte
        push 101
        call out-byte
        push 115
        call out-byte
        push 116
        call out-byte
        push 98
        call out-byte
        push 111
        call out-byte
        push 111
        call out-byte
        push 107
        call out-byte
        push 60
        call out-byte
        push 47
        call out-byte
        push 116
        call out-byte
        push 105
        call out-byte
        push 116
        call out-byte
        push 108
        call out-byte
        push 101
        call out-byte
        push 62
        call out-byte
        push 60
        call out-byte
        push 47
        call out-byte
        push 104
        call out-byte
        push 101
        call out-byte
        push 97
        call out-byte
        push 100
        call out-byte
        push 62
        call out-byte
        push 10
        call out-byte
        push 60
        call out-byte
        push 98
        call out-byte
        push 111
        call out-byte
        push 100
        call out-byte
        push 121
        call out-byte
        push 62
        call out-byte
        push 10
        call out-byte
        push 60
        call out-byte
        push 104
        call out-byte
        push 49
        call out-byte
        push 62
        call out-byte
        push 73
        call out-byte
        push 110
        call out-byte
        push 118
        call out-byte
        push 105
        call out-byte
        push 115
        call out-byte
        push 105
        call out-byte
        push 98
        call out-byte
        push 108
        call out-byte
        push 101
        call out-byte
        push 32
        call out-byte
        push 73
        call out-byte
        push 110
        call out-byte
        push 107
        call out-byte
        push 32
        call out-byte
        push 71
        call out-byte
        push 117
        call out-byte
        push 101
        call out-byte
        push 115
        call out-byte
        push 116
        call out-byte
        push 98
        call out-byte
        push 111
        call out-byte
        push 111
        call out-byte
        push 107
        call out-byte
        push 60
        call out-byte
        push 47
        call out-byte
        push 104
        call out-byte
        push 49
        call out-byte
        push 62
        call out-byte
        push 10
        call out-byte
        push 60
        call out-byte
        push 112
        call out-byte
        push 62
        call out-byte
        push 84
        call out-byte
        push 104
        call out-byte
        push 105
        call out-byte
        push 115
        call out-byte
        push 32
        call out-byte
        push 112
        call out-byte
        push 97
        call out-byte
        push 103
        call out-byte
        push 101
        call out-byte
        push 32
        call out-byte
        push 105
        call out-byte
        push 115
        call out-byte
        push 32
        call out-byte
        push 115
        call out-byte
        push 101
        call out-byte
        push 114
        call out-byte
        push 118
        call out-byte
        push 101
        call out-byte
        push 100
        call out-byte
        push 32
        call out-byte
        push 98
        call out-byte
        push 121
        call out-byte
        push 32
        call out-byte
        push 97
        call out-byte
        push 32
        call out-byte
        push 112
        call out-byte
        push 114
        call out-byte
        push 111
        call out-byte
        push 103
        call out-byte
        push 114
        call out-byte
        push 97
        call out-byte
        push 109
        call out-byte
        push 32
        call out-byte
        push 119
        call out-byte
        push 104
        call out-byte
        push 111
        call out-byte
        push 115
        call out-byte
        push 101
        call out-byte
        push 32
        call out-byte
        push 101
        call out-byte
        push 118
        call out-byte
        push 101
        call out-byte
        push 114
        call out-byte
        push 121
        call out-byte
        push 32
        call out-byte
        push 105
        call out-byte
        push 110
        call out-byte
        push 115
        call out-byte
        push 116
        call out-byte
        push 114
        call out-byte
        push 117
        call out-byte
        push 99
        call out-byte
        push 116
        call out-byte
        push 105
        call out-byte
        push 111
        call out-byte
        push 110
        call out-byte
        push 32
        call out-byte
        push 105
        call out-byte
        push 115
        call out-byte
        push 32
        call out-byte
        push 97
        call out-byte
        push 110
        call out-byte
        push 32
        call out-byte
        push 105
        call out-byte
        push 110
        call out-byte
        push 118
        call out-byte
        push 105
        call out-byte
        push 115
        call out-byte
        push 105
        call out-byte
        push 98
        call out-byte
        push 108
        call out-byte
        push 101
        call out-byte
        push 32
        call out-byte
        push 111
        call out-byte
        push 114
        call out-byte
        push 32
        call out-byte
        push 122
        call out-byte
        push 101
        call out-byte
        push 114
        call out-byte
        push 111
        call out-byte
        push 45
        call out-byte
        push 119
        call out-byte
        push 105
        call out-byte
        push 100
        call out-byte
        push 116
        call out-byte
        push 104
        call out-byte
        push 32
        call out-byte
        push 85
        call out-byte
        push 110
        call out-byte
        push 105
        call out-byte
        push 99
        call out-byte
        push 111
        call out-byte
        push 100
        call out-byte
        push 101
        call out-byte
        push 32
        call out-byte
        push 99
        call out-byte
        push 111
        call out-byte
        push 100
        call out-byte
        push 101
        call out-byte
        push 112
        call out-byte
        push 111
        call out-byte
        push 105
        call out-byte
        push 110
        call out-byte
        push 116
        call out-byte
        push 46
        call out-byte
        push 32
        call out-byte
        push 76
        call out-byte
        push 101
        call out-byte
        push 97
        call out-byte
        push 118
        call out-byte
        push 101
        call out-byte
        push 32
        call out-byte
        push 97
        call out-byte
        push 32
        call out-byte
        push 110
        call out-byte
        push 111
        call out-byte
        push 116
        call out-byte
        push 101
        call out-byte
        push 58
        call out-byte
        push 60
        call out-byte
        push 47
        call out-byte
        push 112
        call out-byte
        push 62
        call out-byte
        push 10
        call out-byte
        push 60
        call out-byte
        push 102
        call out-byte
        push 111
        call out-byte
        push 114
        call out-byte
        push 109
        call out-byte
        push 32
        call out-byte
        push 109
        call out-byte
        push 101
        call out-byte
        push 116
        call out-byte
        push 104
        call out-byte
        push 111
        call out-byte
        push 100
        call out-byte
        push 61
        call out-byte
        push 34
        call out-byte
        push 112
        call out-byte
        push 111
        call out-byte
        push 115
        call out-byte
        push 116
        call out-byte
        push 34
        call out-byte
        push 32
        call out-byte
        push 97
        call out-byte
        push 99
        call out-byte
        push 116
        call out-byte
        push 105
        call out-byte
        push 111
        call out-byte
        push 110
        call out-byte
        push 61
        call out-byte
        push 34
        call out-byte
        push 47
        call out-byte
        push 115
        call out-byte
        push 105
        call out-byte
        push 103
        call out-byte
        push 110
        call out-byte
        push 34
        call out-byte
        push 62
        call out-byte
        push 10
        call out-byte
        push 60
        call out-byte
        push 112
        call out-byte
        push 62
        call out-byte
        push 60
        call out-byte
        push 105
        call out-byte
        push 110
        call out-byte
        push 112
        call out-byte
        push 117
        call out-byte
        push 116
        call out-byte
        push 32
        call out-byte
        push 110
        call out-byte
        push 97
        call out-byte
        push 109
        call out-byte
        push 101
        call out-byte
        push 61
        call out-byte
        push 34
        call out-byte
        push 109
        call out-byte
        push 115
        call out-byte
        push 103
        call out-byte
        push 34
        call out-byte
        push 32
        call out-byte
        push 115
        call out-byte
        push 105
        call out-byte
        push 122
        call out-byte
        push 101
        call out-byte
        push 61
        call out-byte
        push 34
        call out-byte
        push 54
        call out-byte
        push 48
        call out-byte
        push 34
        call out-byte
        push 62
        call out-byte
        push 32
        call out-byte
        push 60
        call out-byte
        push 105
        call out-byte
        push 110
        call out-byte
        push 112
        call out-byte
        push 117
        call out-byte
        push 116
        call out-byte
        push 32
        call out-byte
        push 116
        call out-byte
        push 121
        call out-byte
        push 112
        call out-byte
        push 101
        call out-byte
        push 61
        call out-byte
        push 34
        call out-byte
        push 115
        call out-byte
        push 117
        call out-byte
        push 98
        call out-byte
        push 109
        call out-byte
        push 105
        call out-byte
        push 116
        call out-byte
        push 34
        call out-byte
        push 32
        call out-byte
        push 118
        call out-byte
        push 97
        call out-byte
        push 108
        call out-byte
        push 117
        call out-byte
        push 101
        call out-byte
        push 61
        call out-byte
        push 34
        call out-byte
        push 83
        call out-byte
        push 105
        call out-byte
        push 103
        call out-byte
        push 110
        call out-byte
        push 34
        call out-byte
        push 62
        call out-byte
        push 60
        call out-byte
        push 47
        call out-byte
        push 112
        call out-byte
        push 62
        call out-byte
        push 10
        call out-byte
        push 60
        call out-byte
        push 47
        call out-byte
        push 102
        call out-byte
        push 111
        call out-byte
        push 114
        call out-byte
        push 109
        call out-byte
        push 62
        call out-byte
        push 10
        call out-byte
        push 60
        call out-byte
        push 104
        call out-byte
        push 114
        call out-byte
        push 62
        call out-byte
        push 10
        call out-byte
        push 60
        call out-byte
        push 117
        call out-byte
        push 108
        call out-byte
        push 62
        call out-byte
        push 10
        call out-byte

word gb-suffix
            push 60
        call out-byte
        push 47
        call out-byte
        push 117
        call out-byte
        push 108
        call out-byte
        push 62
        call out-byte
        push 10
        call out-byte
        push 60
        call out-byte
        push 104
        call out-byte
        push 114
        call out-byte
        push 62
        call out-byte
        push 10
        call out-byte
        push 60
        call out-byte
        push 112
        call out-byte
        push 62
        call out-byte
        push 71
        call out-byte
        push 117
        call out-byte
        push 101
        call out-byte
        push 115
        call out-byte
        push 116
        call out-byte
        push 98
        call out-byte
        push 111
        call out-byte
        push 111
        call out-byte
        push 107
        call out-byte
        push 32
        call out-byte
        push 109
        call out-byte
        push 97
        call out-byte
        push 105
        call out-byte
        push 110
        call out-byte
        push 116
        call out-byte
        push 97
        call out-byte
        push 105
        call out-byte
        push 110
        call out-byte
        push 101
        call out-byte
        push 100
        call out-byte
        push 32
        call out-byte
        push 118
        call out-byte
        push 105
        call out-byte
        push 97
        call out-byte
        push 32
        call out-byte
        push 60
        call out-byte
        push 116
        call out-byte
        push 116
        call out-byte
        push 62
        call out-byte
        push 102
        call out-byte
        push 105
        call out-byte
        push 108
        call out-byte
        push 101
        call out-byte
        push 45
        call out-byte
        push 97
        call out-byte
        push 112
        call out-byte
        push 112
        call out-byte
        push 101
        call out-byte
        push 110
        call out-byte
        push 100
        call out-byte
        push 60
        call out-byte
        push 47
        call out-byte
        push 116
        call out-byte
        push 116
        call out-byte
        push 62
        call out-byte
        push 32
        call out-byte
        push 43
        call out-byte
        push 32
        call out-byte
        push 60
        call out-byte
        push 116
        call out-byte
        push 116
        call out-byte
        push 62
        call out-byte
        push 102
        call out-byte
        push 105
        call out-byte
        push 108
        call out-byte
        push 101
        call out-byte
        push 45
        call out-byte
        push 119
        call out-byte
        push 114
        call out-byte
        push 105
        call out-byte
        push 116
        call out-byte
        push 101
        call out-byte
        push 60
        call out-byte
        push 47
        call out-byte
        push 116
        call out-byte
        push 116
        call out-byte
        push 62
        call out-byte
        push 32
        call out-byte
        push 40
        call out-byte
        push 85
        call out-byte
        push 43
        call out-byte
        push 70
        call out-byte
        push 70
        call out-byte
        push 70
        call out-byte
        push 66
        call out-byte
        push 44
        call out-byte
        push 32
        call out-byte
        push 85
        call out-byte
        push 43
        call out-byte
        push 70
        call out-byte
        push 70
        call out-byte
        push 70
        call out-byte
        push 65
        call out-byte
        push 41
        call out-byte
        push 46
        call out-byte
        push 32
        call out-byte
        push 69
        call out-byte
        push 118
        call out-byte
        push 101
        call out-byte
        push 114
        call out-byte
        push 121
        call out-byte
        push 32
        call out-byte
        push 105
        call out-byte
        push 110
        call out-byte
        push 115
        call out-byte
        push 116
        call out-byte
        push 114
        call out-byte
        push 117
        call out-byte
        push 99
        call out-byte
        push 116
        call out-byte
        push 105
        call out-byte
        push 111
        call out-byte
        push 110
        call out-byte
        push 32
        call out-byte
        push 105
        call out-byte
        push 110
        call out-byte
        push 32
        call out-byte
        push 116
        call out-byte
        push 104
        call out-byte
        push 105
        call out-byte
        push 115
        call out-byte
        push 32
        call out-byte
        push 115
        call out-byte
        push 101
        call out-byte
        push 114
        call out-byte
        push 118
        call out-byte
        push 101
        call out-byte
        push 114
        call out-byte
        push 32
        call out-byte
        push 105
        call out-byte
        push 115
        call out-byte
        push 32
        call out-byte
        push 105
        call out-byte
        push 110
        call out-byte
        push 118
        call out-byte
        push 105
        call out-byte
        push 115
        call out-byte
        push 105
        call out-byte
        push 98
        call out-byte
        push 108
        call out-byte
        push 101
        call out-byte
        push 46
        call out-byte
        push 60
        call out-byte
        push 47
        call out-byte
        push 112
        call out-byte
        push 62
        call out-byte
        push 10
        call out-byte
        push 60
        call out-byte
        push 47
        call out-byte
        push 98
        call out-byte
        push 111
        call out-byte
        push 100
        call out-byte
        push 121
        call out-byte
        push 62
        call out-byte
        push 10
        call out-byte
        push 60
        call out-byte
        push 47
        call out-byte
        push 104
        call out-byte
        push 116
        call out-byte
        push 109
        call out-byte
        push 108
        call out-byte
        push 62
        call out-byte
        push 10
        call out-byte

word gb-entries
    load GPATH
    push 13
    push 0
    file-open
    store GFD
    load GFD
    push 0
    call lt
    if {
        id
    } else {
        call gbe-loop
        load GFD
        close
        drop
    }

word gbe-loop
    load O_POS
    push 11000
    call lt
    if {
        load GFD
        load FILE_BUF
        push 4096
        file-read
        store GB_N
        load GB_N
        push 0
        call gt
        if {
            load FILE_BUF
            load GB_N
            call out-mem
            call gbe-loop
        } else {
            id
        }
    } else {
        id
    }

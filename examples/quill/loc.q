; loc.q -- recursively count lines of code under a directory.
;
; The interpreter has no argv access, so the directory path is read from
; stdin (one line; EOF also terminates it). Empty input defaults to ".".
;
;   printf '%s\n' src | ink run --steps 0 examples/quill/loc.q
;
; Output, one line per regular file, then a grand total:
;
;   src/main.c: 42
;   src/parse.c: 185
;   total: 227
;
; Directories are walked with an explicit worklist in memory (dir-list is
; called for each queued directory, subdirectories are queued), so the
; traversal itself does not consume interpreter call depth.
;
; Run with --steps 0: counting every byte of a large tree blows past the
; default 10,000,000-step limit.

word main
    ; ---- variables (one flat int64 memory cell each) ----
    let TOTAL = 0
    let WL_COUNT = 0
    let CUR = 0
    let EB = 0
    let DIR_BASE = 0
    let DIR_LEN = 0
    let ENT_N = 0
    let ENT_IDX = 0
    let LE = 0
    let CHILD_LEN = 0
    let STAT = 0
    let FD = 0
    let NR = 0
    let BYTES = 0
    let LINES = 0
    let LAST = 0
    let CH = 0
    let ROOT_LEN = 0
    let I = 0
    let C_SRC = 0
    let C_DST = 0
    let C_LEN = 0

    ; ---- addresses / constants ----
    let FILE_BUF = 1000        ; 8192-byte read chunk
    let FILE_CHUNK = 8192
    let CHILD_BUF = 10000      ; built child path
    let LIST_BUF = 20000       ; dir-list output
    let LIST_CAP = 262144
    let WL_BASE = 300000       ; worklist: WL_MAX entries of WL_PATHSZ bytes
    let WL_PATHSZ = 1024
    let WL_MAX = 1024
    let ROOT_PTR = 300001      ; entry 0's path bytes (WL_BASE + 1)

    ; read the directory path from stdin, normalize, seed the worklist
    call read-root
    load WL_BASE
    load ROOT_LEN
    store
    push 1
    store WL_COUNT

    call process-all

    push 116 char-out   ; 't'
    push 111 char-out   ; 'o'
    push 116 char-out   ; 't'
    push 97  char-out   ; 'a'
    push 108 char-out   ; 'l'
    push 58  char-out   ; ':'
    push 32  char-out   ; ' '
    load TOTAL
    call out-num
    push 10
    char-out
    halt

; ---------------------------------------------------------------------------
; stdin -> root path, ensure it ends in '/'
; ---------------------------------------------------------------------------
word read-root
    push 0
    store ROOT_LEN
    call read-root-loop

    ; empty input -> "."
    load ROOT_LEN
    push 0
    call eq
    if {
        load ROOT_PTR
        push 46
        store
        load ROOT_LEN
        push 1
        add
        store ROOT_LEN
    } else {
        id
    }

    ; append '/' if missing
    load ROOT_PTR
    load ROOT_LEN
    add
    push 1
    sub
    load
    push 47
    call eq
    if {
        id
    } else {
        load ROOT_PTR
        load ROOT_LEN
        add
        push 47
        store
        load ROOT_LEN
        push 1
        add
        store ROOT_LEN
    }

word read-root-loop
    char-in
    store CH
    load CH
    push -1
    call eq
    if {
        id
    } else {
        load CH
        push 10
        call eq
        if {
            id
        } else {
            load ROOT_PTR
            load ROOT_LEN
            add
            load CH
            store
            load ROOT_LEN
            push 1
            add
            store ROOT_LEN
            call read-root-loop
        }
    }

; ---------------------------------------------------------------------------
; walk the worklist
; ---------------------------------------------------------------------------
word process-all
    load CUR
    load WL_COUNT
    call lt
    if {
        call process-dir
        load CUR
        push 1
        add
        store CUR
        call process-all
    } else {
        id
    }

word process-dir
    ; entry base = WL_BASE + CUR * WL_PATHSZ
    load CUR
    load WL_PATHSZ
    mul
    load WL_BASE
    add
    store DIR_BASE
    load DIR_BASE
    load
    store DIR_LEN

    load DIR_BASE
    push 1
    add
    load DIR_LEN
    load LIST_BUF
    load LIST_CAP
    dir-list
    store ENT_N

    push 0
    store ENT_IDX
    load ENT_N
    push 0
    call gt
    if {
        call scan-entries
    } else {
        id
    }

word scan-entries
    load ENT_IDX
    load ENT_N
    call lt
    if {
        call find-eol
        call build-child

        load CHILD_BUF
        load CHILD_LEN
        file-stat
        store STAT

        load STAT
        push -2
        call eq
        if {
            call push-dir
        } else {
            load STAT
            push 0
            call ge
            if {
                call count-file
            } else {
                id
            }
        }

        load LE
        push 1
        add
        store ENT_IDX
        call scan-entries
    } else {
        id
    }

; LE = index of newline at/after ENT_IDX, or ENT_N
word find-eol
    load ENT_IDX
    store LE
    call find-eol-loop

word find-eol-loop
    load LE
    load ENT_N
    call lt
    if {
        load LIST_BUF
        load LE
        add
        load
        push 10
        call eq
        if {
            id
        } else {
            load LE
            push 1
            add
            store LE
            call find-eol-loop
        }
    } else {
        id
    }

; child path = current path + listing name; current path ends in '/'
word build-child
    load DIR_BASE
    push 1
    add
    load CHILD_BUF
    load DIR_LEN
    call copy-bytes

    load LIST_BUF
    load ENT_IDX
    add
    load CHILD_BUF
    load DIR_LEN
    add
    load LE
    load ENT_IDX
    sub
    call copy-bytes

    load LE
    load ENT_IDX
    sub
    load DIR_LEN
    add
    store CHILD_LEN

; queue a subdirectory (child path already ends in '/')
word push-dir
    load WL_COUNT
    load WL_MAX
    call lt
    if {
        load WL_COUNT
        load WL_PATHSZ
        mul
        load WL_BASE
        add
        store EB

        load EB
        load CHILD_LEN
        store

        load CHILD_BUF
        load EB
        push 1
        add
        load CHILD_LEN
        call copy-bytes

        load WL_COUNT
        push 1
        add
        store WL_COUNT
    } else {
        id
    }

; ---------------------------------------------------------------------------
; count lines in the file at CHILD_BUF
; ---------------------------------------------------------------------------
word count-file
    load CHILD_BUF
    load CHILD_LEN
    push 0
    file-open
    store FD
    load FD
    push 0
    call ge
    if {
        call print-child-path

        push 0
        store BYTES
        push 0
        store LINES
        call count-lines

        ; a trailing chunk without a final newline is still a line
        load BYTES
        push 0
        call gt
        if {
            load LAST
            push 10
            call eq
            if {
                id
            } else {
                load LINES
                push 1
                add
                store LINES
            }
        } else {
            id
        }

        load LINES
        call out-num
        push 10
        char-out

        load TOTAL
        load LINES
        add
        store TOTAL

        load FD
        close
        drop
    } else {
        id
    }

word count-lines
    load FD
    load FILE_BUF
    load FILE_CHUNK
    file-read
    store NR
    load NR
    push 0
    call gt
    if {
        push 0
        store I
        call count-chunk

        load BYTES
        load NR
        add
        store BYTES

        load FILE_BUF
        load NR
        add
        push 1
        sub
        load
        store LAST

        call count-lines
    } else {
        id
    }

word count-chunk
    load I
    load NR
    call lt
    if {
        load FILE_BUF
        load I
        add
        load
        push 10
        call eq
        if {
            load LINES
            push 1
            add
            store LINES
        } else {
            id
        }
        load I
        push 1
        add
        store I
        call count-chunk
    } else {
        id
    }

word print-child-path
    push 0
    store I
    call print-child-loop

word print-child-loop
    load I
    load CHILD_LEN
    call lt
    if {
        load CHILD_BUF
        load I
        add
        load
        char-out
        load I
        push 1
        add
        store I
        call print-child-loop
    } else {
        push 58
        char-out
        push 32
        char-out
    }

; ---------------------------------------------------------------------------
; helpers
; ---------------------------------------------------------------------------
word copy-bytes      ; ( src dst len -- )
    store C_LEN
    store C_DST
    store C_SRC
    call copy-loop

word copy-loop
    load C_LEN
    push 0
    call gt
    if {
        load C_DST
        load C_SRC
        load
        store
        load C_SRC
        push 1
        add
        store C_SRC
        load C_DST
        push 1
        add
        store C_DST
        load C_LEN
        push 1
        sub
        store C_LEN
        call copy-loop
    } else {
        id
    }

word out-num          ; ( n -- )
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
        char-out
        drop
    } else {
        drop
        push 48
        add
        char-out
    }

word lt               ; ( a b -- a<b )
    sub
    sign
    push 1
    add
    if {
        push 0
    } else {
        push 1
    }

word eq               ; ( a b -- a==b )
    sub
    if {
        push 0
    } else {
        push 1
    }

word gt               ; ( a b -- a>b )
    swap
    call lt

word ge               ; ( a b -- a>=b )
    call lt
    if {
        push 0
    } else {
        push 1
    }

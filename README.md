# Invisible Ink

Whitespace, except worse. Every instruction is an invisible or zero-width Unicode
codepoint, no two of which look alike, and none of which you can see anyway. Several of
these are one editor-setting away from being silently destroyed. 

A program is a sequence of invisible codepoints, plus as much visible "cover text"
as you like: **any character not defined below is ignored entirely.** This allows you to splice a `.txt` document with Invisible Ink, and that's the language's entire purpose. Critically, `U+0020 SPACE` itself is *not* one of the spec codepoints, so ordinary prose (which is mostly made of spaces) is always safe cover text: a program can be laced anywhere in a document, including in the middle of a sentence, not just at paragraph breaks.

Writing this language is genuinely awful, so I've made its own text editor. Just to let you get a sense of how painful this language is to write, I've added the raw inputs I wrote in the IDE for the blackjack example, you can view the raw here: [view the blackjack raw](blackjack-raw.md)

Just for completeness, I figured I'd also disassemble the blackjack example too. You can view it here: [view the blackjack disassembled quill source](blackjack-disassemble.q)

---

## Installation

```sh
make              # builds src/ink
./install.sh      # installs ink to /usr/local/bin/ink
```

---

## `ink`

Everything is now under one command. `quill` still works as a shim but is deprecated.

```sh
ink <command> [options] [args]
ink -h | --help
ink -V | --version
ink help <command>
```

### Commands and args

| Command | Aliases | Args / Options | Description |
|---|---|---|---|
| `run <file>` | `r` | `<file>` `--steps <N>` / `--max-steps <N>` `--depth <N>` / `--max-depth <N>` `--stats` `-h/--help` | Run an Invisible Ink program. `<file>` is UTF-8 (cover text allowed, `FEFF`-embedded-aware). `--steps` overrides step limit (default 10M), `--depth` overrides call-depth (default 20k), `--stats` prints `steps/words/stack/mem/elapsed` to stderr. Legacy `ink <file>` and `ink <file> --stats` still work. |
| `dump <file>` | — | `<file>` | Dump parsed AST in internal style (`word 0:` / `then:` / `else:`). Legacy `ink -d <file>` maps here. |
| `disassemble <input> [-o <output>]` | `disasm`, `decompile`, `d` | `<input>` `-o`/`--output <file>` `-h/--help` | Decompile Invisible Ink → Quill (`word word_0` …). Writes to stdout if `-o` omitted. Replaces `quill disassemble`. |
| `assemble <input> [-o <output>]` | `asm`, `compile`, `build`, `c` | `<input>` `-o`/`--output <file>` `-h/--help` | Assemble Quill `.q` → Invisible Ink. Writes to stdout if `-o` omitted. Replaces `quill assemble`. |
| `check <file>` | — | `<file>` `-h/--help` | Parse + validate without executing. Prints word count / spec codepoints / embedded status. |
| `edit [file]` | `ide`, `ed` | `[file]` `--selftest` `-h/--help` | Launch InkIDE visual editor. No file → new buffer. `--selftest` runs internal tests. Also available as `ink ide`. |
| `help [command]` | `-h`, `--help` | `[command]` | Show global or per-command help. |

Global help and per-command help are available:

```sh
ink help              # global usage
ink help run          # run options
ink help disassemble  # disassemble options
ink help assemble
ink help edit
ink run --help        # same as above
ink disasm --help
```

**Legacy compatibility**

* `ink program.txt` → `ink run program.txt`
* `ink -d program.txt` → `ink dump program.txt`
* `quill assemble/disassemble …` → shim forwards to `ink assemble/disassemble`

---

## Program structure

An Invisible Ink source consists of one or more **words**, separated by
`U+2001 EM QUAD`:

- Everything before the first `¶` is **word 0**, the entry point, and runs
  automatically.
- Each subsequent `¶`-delimited chunk is word 1, 2, 3, ... in definition order.
- A word returns implicitly when it hits the next `¶` or the program terminator.
- Within a word, `U+2000 EN QUAD` marks statement boundaries (and also
  closes literals).

Recursion plus `if/then/else` plus the native `while` (see *Control flow*) are
the control-flow primitives; combined with word calls (below) that's enough
for arbitrary iteration.

---

## Operator reference

### Stack manipulation

| Codepoint | Name | Op | Stack effect |
|---|---|---|---|
| U+2062 | INVISIBLE TIMES | `dup` | `a -- a a` |
| U+00A0 | NO-BREAK SPACE | `swap` | `a b -- b a` |
| U+2007 | FIGURE SPACE | `drop` | `a --` |
| U+2009 | THIN SPACE | `rot` | `a b c -- b c a` |

### Arithmetic, arity-dispatched

Three codepoints, each doing a different job depending on how many times it
repeats in a row (1×, 2×, or 3×). Each row is a coherent little family:

| Codepoint | ×1 | ×2 | ×3 |
|---|---|---|---|
| U+2002 EN SPACE | negate | subtract | modulo |
| U+2003 EM SPACE | identity | add | divide |
| U+2004 THREE-PER-EM SPACE | sign | multiply | exponent |

Negative literals aren't a separate syntax: push the unsigned value, then apply
EN SPACE at arity 1 (`negate`).

### Bitwise and shifts (ISA v2)

Five plain bitwise ops, all two-operand, word-wise on the unsigned 64-bit
pattern. Shift counts are taken mod 64 (`count & 63`), and `shr` is a **logical**
(shift-in-zero) right shift, never sign-extending:

| Codepoint | Name | Op | Stack effect |
|---|---|---|---|
| U+202A | LEFT-TO-RIGHT EMBEDDING | `and` | `a b -- a&b` |
| U+202B | RIGHT-TO-LEFT EMBEDDING | `or` | `a b -- a\|b` |
| U+202C | POP DIRECTIONAL FORMATTING | `xor` | `a b -- a^b` |
| U+202D | LEFT-TO-RIGHT OVERRIDE | `shl` | `a b -- a << (b & 63)` |
| U+202E | RIGHT-TO-LEFT OVERRIDE | `shr` | `a b -- logical (a >> b) on uint64` |

### Literals

`U+200A HAIR SPACE` opens a literal. A run of binary-digit codepoints follows,
most-significant bit first, and the literal closes at the next terminator
(which also still ends the statement; no separate terminator needed):

| Codepoint | Name | Meaning |
|---|---|---|
| U+200A | HAIR SPACE | begin literal |
| U+2005 | FOUR-PER-EM SPACE | binary digit `0` |
| U+2006 | SIX-PER-EM SPACE | binary digit `1` |
| U+2000 | EN QUAD | close literal / end statement |
| U+00AD | SOFT HYPHEN | close literal + `char-out` |

A literal must contain at least one binary digit, and must be terminated by
either `EN QUAD` or `SOFT HYPHEN`. `SOFT HYPHEN` is a fused shorthand:
`HAIR SPACE <bits> SOFT HYPHEN` is exactly equivalent to
`HAIR SPACE <bits> EN QUAD` + `PUNCTUATION SPACE` (`char-out`). it pushes the
value and immediately prints it as a Unicode codepoint. Outside a literal,
a stray `SOFT HYPHEN` is ignored (like any non-spec cover text would be, but
explicitly consumed so it never leaks into parsing).

Example: pushing `5` (`101`): `HAIR-SPACE SIX-PER-EM FOUR-PER-EM SIX-PER-EM EN-QUAD`.

To print `A` (65 = `1000001`) without a separate `char-out` op:
`HAIR-SPACE SIX-PER-EM FOUR-PER-EM ×5 SIX-PER-EM SOFT-HYPHEN`.

### I/O

| Codepoint | Name | Op | Stack effect |
|---|---|---|---|
| U+3000 | IDEOGRAPHIC SPACE | `num-out` | `a --` (pop, print as decimal, **no** newline) |
| U+2008 | PUNCTUATION SPACE | `char-out` | `a --` (pop, print as Unicode codepoint, no newline) |
| U+205F | MEDIUM MATHEMATICAL SPACE | `char-in` | `-- a` (read one codepoint, push its code point; pushes `-1` on EOF) |

Numeric input isn't a dedicated op. Do `char-in` in a loop plus your own parsing.

`char-in` flushes stdout before blocking, so prompts printed with `char-out`/`num-out` are visible before input is requested. `num-out` no longer appends a newline (earlier builds did). Emit `10` via `char-out` explicitly if you want a line break.

### Memory (random-access store)

Sparse, unbounded word-addressed memory, zero-initialised. Addresses are signed 64-bit integers; negative addresses halt. Reading an address that has never been written returns `0`. Memory grows on demand.

| Codepoint | Name | Op | Stack effect |
|---|---|---|---|
| U+2064 | INVISIBLE PLUS | `store` | `addr value --` (pop `value`, pop `addr`, `mem[addr] = value`) |
| U+200E | LEFT-TO-RIGHT MARK | `load` | `addr -- value` (push `mem[addr]`) |

Mnemonic order for `store` is address on the bottom, value on top, so a typical sequence is `push addr; push value; store`. Both ops halt on negative address (and `store` will grow the backing store as needed).

To bump a counter at address `0`:

```
push 0; push 0; store   // mem[0] = 0
push 0; load            // push mem[0]
push 1; add             // inc
push 0; swap; store     // mem[0] = mem[0]+1  (re-arrange to addr/value order)
```

### Random

| Codepoint | Name | Op | Stack effect |
|---|---|---|---|
| U+2063 | INVISIBLE SEPARATOR | `rand` | `n -- r` where `0 <= r < n`, uniform, unbiased via rejection sampling |

Pops `n`, pushes a uniform random integer in `[0, n)`. Halts if `n <= 0`. Seeded on first use from `/dev/urandom` when available, otherwise `clock_gettime(CLOCK_MONOTONIC)` mixed with `getpid()` and stack address (xorshift64*). Used by `blackjack.txt` for shuffling/dealing.

Example: roll a d6 (1–6): `push 6; rand; push 1; add`.

### Timing

| Codepoint | Name | Op | Stack effect |
|---|---|---|---|
| U+200F | RIGHT-TO-LEFT MARK | `sleep` | `ms --` (pop milliseconds, sleep that long) |

Pops `ms` and sleeps for `ms` milliseconds via `nanosleep`. No-op if `ms <= 0`. Flushes stdout before sleeping, so prior output is visible during the pause. Useful for animations, pacing card reveals in `blackjack.txt`, etc.

### Networking

Minimal blocking TCP server primitives. File descriptors are small
integers pushed on the stack; on failure an op pushes `-1` instead of
halting (so Quill programs can branch on errors). `close`/`sock-close`
closes either a socket or a file fd. A client disconnect mid-`sock-send`
reports a short write rather than killing the server (`SIGPIPE` is ignored).

| Codepoint | Quill | Stack effect |
|---|---|---|
| U+2065 | `sock-create` | `-- fd` (push listening socket, `-1` on failure) |
| U+2066 | `sock-bind` | `fd port -- rc` (`0` ok, `-1` fail) |
| U+2067 | `sock-listen` | `fd backlog -- rc` (`0` ok, `-1` fail) |
| U+2068 | `sock-accept` | `fd -- cfd` (blocks; `-1` on failure) |
| U+2069 | `sock-recv` | `fd buf len -- n` (bytes read into `mem[buf..]`, `0` = EOF, `-1` = error) |
| U+206A | `sock-send` | `fd buf len -- n` (bytes sent from `mem[buf..]`) |
| U+206B | `sock-close` / `close` / `file-close` | `fd -- rc` (`0` ok, `-1` fail) |

### Filesystem

Paths live in memory as byte arrays: `path_ptr` points at `mem[path_ptr..path_ptr+path_len)`, one byte per cell. `file-open` is read-only.

| Codepoint | Quill | Stack effect |
|---|---|---|
| U+206C | `file-open` | `path_ptr path_len flags -- fd` (`flags` ignored, `-1` if missing) |
| U+206D | `file-read` | `fd buf len -- n` (`0` = EOF, `-1` = error) |
| U+206E | `file-stat` | `path_ptr path_len -- size` (`-1` missing, `-2` directory) |
| U+206F | `dir-list` | `path_ptr path_len out_buf out_cap -- n` (newline-separated names, `/` suffix for dirs, sorted case-insensitively; `-1` on error) |

### Control flow

| Codepoint | Name | Role |
|---|---|---|
| U+200C | ZERO WIDTH NON-JOINER | block open |
| U+200D | ZERO WIDTH JOINER | block close |
| U+2060 | WORD JOINER | `if` / `then` / `else` (see below) |
| U+2061 | FUNCTION APPLICATION | `call` - pop `n`, run word `n`, return here when it finishes |
| U+061C | ARABIC LETTER MARK | `while` (see below) |
| U+2000 | EN QUAD | statement boundary |
| U+2001 | EM QUAD | word definition boundary |
| U+200B | ZERO WIDTH SPACE | program terminator |
| U+FEFF | ZERO WIDTH NO-BREAK SPACE | begin embedded program (see *Embedding*) |

**`while` grammar:** `ARABIC LETTER MARK` is followed by two
`ZWNJ ... ZWJ` blocks: a *condition* block and a *body* block. Repeat:
execute the condition block, pop its result, run the body block while the
result is truthy; a falsy result exits the loop. The condition block must
leave exactly one value on the stack (an empty condition is a stack
underflow error). Unlike `call`-based looping, a `while` loop **consumes
no call depth** — iterations don't stack frames, so you can loop
indefinitely under the 20,000-deep limit. The step limit still bounds
runaway loops as always.

Quill syntax (identical shape, readable):
`while { <cond> } { <body> }`.

**`if`/`then`/`else` grammar:** `WORD JOINER` pops `cond`, then runs the next
`ZWNJ ... ZWJ` block if `cond` is truthy. If that block is *immediately*
followed by a second `ZWNJ ... ZWJ` block, that second block runs instead when
`cond` is falsy (and is skipped otherwise). The second block is optional. You can omit it for plain if-with-no-else.

**`call`:** recursion is just a word calling its own index; combined with
`if/then` as a base-case guard, this is the language's only loop mechanism.

---

## Comments

No dedicated codepoint, meaning any character outside this spec is simply ignored, so
all visible text in a file is automatically a comment. This is
also what makes embedding (below) work: the surrounding prose has to be inert,
and it already is.

---

## Errors

**Halt-on-error**, matching original Whitespace. Any of the following halts the program immediately with a runtime error (exit 1, message on stderr):

- stack underflow (`dup`/`swap`/`drop`/`rot`/arity ops/`call`/`if`/`while`/`num-out`/`char-out`/`rand`/`store`/`load`/`sleep` needing a value that isn't there)
- divide-by-zero or modulo-by-zero
- malformed literal (empty, or unterminated; not closed by `EN QUAD` or `SOFT HYPHEN`)
- `rand` with `n <= 0`
- `store`/`load` with negative address
- call to undefined word (index `< 0` or `>= word count`)
- a `while` condition block that doesn't leave a value (stack underflow)
- call depth exceeded (recursion > 20,000)
- step limit exceeded (10,000,000 instructions, guards against infinite loops)

`char-in` at EOF is *not* an error. It pushes `-1`.

---

## Runtime limits

- **Steps:** 10,000,000 ops per run (`g_max_steps` in `src/vm.c` / `src/ink.h`, default `DEFAULT_MAX_STEPS`). Override at runtime: `ink run --steps 5000000 program.txt` or `--max-steps`. `--steps 0` disables the limit (needed for servers like `examples/quill/httpd.q`).
- **Call depth:** 20,000 nested `call`s (`DEFAULT_MAX_DEPTH`). Override: `ink run --depth 1000 program.txt`.
- **Memory:** heap-grown (`g_mem`), doubled as needed, zero-filled.
- **I/O:** `char-in` reads UTF-8 codepoints from stdin; `char-out`/`num-out`/`sleep`/`char-in` all `fflush(stdout)` before blocking/sleeping.

Validate without running: `ink check program.txt`
Dump parsed words/blocks: `ink dump program.txt` (internal) or `ink disassemble program.txt` (to Quill).

## Quill (mnemonic assembly)

**Quill is the mnemonic assembly language for Invisible Ink.** You write Quill
(`*.q`), the assembler inks it. Every mnemonic compiles to a specific run of
invisible Unicode codepoints, and a disassembler can recover a structurally
equivalent Quill program from any Invisible Ink source. 

### Usage

```sh
ink assemble program.q -o program.txt
ink disassemble program.txt
ink disassemble program.txt -o program.q
# aliases:
ink asm program.q -o program.txt
ink disasm program.txt
```

Legacy `quill` binary still works as a shim:
```sh
quill assemble program.q -o program.txt   # deprecated, forwards to ink
quill disassemble program.txt
```

Examples in `examples/quill/` (`add.q`, `fib.q`, `countdown.q`,
`countdown-while.q`, `echo.q`, `bitops.q`).

### Lexical structure

- Tokens are separated by any whitespace (spaces, tabs, newlines).
- `;` starts a comment that runs to end of line. Comments are stripped
  entirely and never affect emitted output.
- Identifiers (word names, memory slot names): `[a-zA-Z_][a-zA-Z0-9_-]*`.
- Integer literals: an optional `-`, then one or more decimal digits.
- Keywords: `word`, `if`, `else`, `while`, `let`, `call`, `halt`, and the op
  mnemonics in the tables below, are reserved and cannot be used as identifiers.
- `{` and `}` delimit `if`/`else` blocks only.

### Program structure

A Quill source file is a sequence of **word definitions**:

```
word <name>
    <statement>
    <statement>
    ...
```

- The **first** `word` block in the file is the entry point, regardless of
  its name, compiling to Ink word 0.
- Every subsequent `word` block is assigned the next index in definition
  order (1, 2, 3, ...).
- A word body ends at the next `word` keyword or end of file. There is no
  explicit `end`.
- Forward references to words and slots are allowed. `call` may reference
  a word defined later in the file.

### Stack operations

| Mnemonic | Stack effect | Emits |
|---|---|---|
| `dup`  | `a -- a a`   | `U+2062` ×1 |
| `swap` | `a b -- b a` | `U+00A0` ×1 |
| `drop` | `a --`       | `U+2007` ×1 |
| `rot`  | `a b c -- b c a` | `U+2009` ×1 |

### Arithmetic

Invisible Ink packs three operations onto each of three codepoints via
run-length (arity 1/2/3). Quill unpacks this into nine flat, unambiguous
mnemonics:

| Mnemonic | Stack effect | Underlying codepoint × arity |
|---|---|---|
| `neg`  | `a -- -a`     | `U+2002` ×1 |
| `sub`  | `a b -- a-b`  | `U+2002` ×2 |
| `mod`  | `a b -- a%b`  | `U+2002` ×3 |
| `id`   | `a -- a`      | `U+2003` ×1 |
| `add`  | `a b -- a+b`  | `U+2003` ×2 |
| `div`  | `a b -- a/b`  | `U+2003` ×3 |
| `sign` | `a -- sgn(a)` | `U+2004` ×1 |
| `mul`  | `a b -- a*b`  | `U+2004` ×2 |
| `pow`  | `a b -- a^b`  | `U+2004` ×3 |
| `and`  | `a b -- a&b`  | `U+202A` ×1 |
| `or`   | `a b -- a\|b` | `U+202B` ×1 |
| `xor`  | `a b -- a^b`  | `U+202C` ×1 |
| `shl`  | `a b -- a << (b & 63)` | `U+202D` ×1 |
| `shr`  | `a b -- (uint64)logical a >> (b & 63)` | `U+202E` ×1 |

**Negative literals** are not a separate literal form: `push -5`
assembles to `push 5` immediately followed by `neg`.

### Literals

```
push <integer>
```

Compiles to `U+200A HAIR SPACE`, the binary digits of the integer's absolute
value (most-significant bit first, via `U+2005 FOUR-PER-EM`/`U+2006 SIX-PER-EM
SPACE`), then `U+2000 EN QUAD`. A negative operand triggers the `neg`
expansion described above.

### I/O

| Mnemonic | Effect |
|---|---|
| `num-out`  | pop, print as decimal number (no newline; emit `10` via `char-out` for line break) |
| `char-out` | pop, print as one UTF-8 codepoint, no newline |
| `char-in`  | read one codepoint from stdin, push its code point (`-1` on EOF) |

### Control flow

#### `if` / `else`

```
if {
    <statements run when top-of-stack is truthy, popped first>
} else {
    <statements run when falsy>
}
```

The `else` block is optional:

```
if {
    <statements>
}
```

Compiles directly to `U+2060 WORD JOINER`, `U+200C ZWNJ ... U+200D ZWJ`,
optionally followed by a second `U+200C ZWNJ ... U+200D ZWJ`.

#### `while`

```
while {
    <condition statements, must leave one value on the stack>
} {
    <body statements>
}
```

Compiles to `U+061C ARABIC LETTER MARK`, `U+200C ZWNJ ... U+200D ZWJ`
(condition), then `U+200C ZWNJ ... U+200D ZWJ` (body). Infinite loops are legal (`while { push 1 } { ... }`) but bounded by the
step limit unless the limit is disabled with `--steps 0`.

The two-block form maps one-to-one onto the `if`/`else` block pair grammar:
`while` is `if` where the first block is re-evaluated every round and the
second block repeats instead of running once.

```
word main
    let i = 0
    push 5
    store i
    while {
        load i
    } {
        load i
        num-out
        push 10
        char-out
        load i
        push 1
        sub
        store i
    }
```

#### `call`

```
call <word-name>
```

Resolves `<word-name>` to its word index at assemble time and emits
`push <index>` followed by `U+2061 FUNCTION APPLICATION`. A raw numeric form,
`call 3`, is also legal for calling a word with no convenient name (or one
produced by disassembly).

#### `halt`

```
halt
```

Emits `U+200B ZERO WIDTH SPACE`, terminating the program immediately.

#### Recursion

The classic pre-`while` iteration mechanism still works: a word
that `call`s itself, guarded by an `if`. Prefer `while` for plain loops —
it uses no call depth; heavy recursion hits the 20,000-deep limit and halts.

```
word countdown          ; ( n -- )
    dup
    if {
        dup
        num-out
        push 1
        sub
        call countdown
    } else {
        drop
    }
```

### Memory

Invisible Ink's `store`/`load` address a flat integer-indexed memory tape
directly. Quill adds named slots as pure sugar over that same tape:

```
let <name> = <integer>
```

Declares a slot, assigns it the next free address at assemble time, and
emits the initializer as `push <addr>`, `push <integer>`, `store` at the
point where the `let` appears. `let` may only appear inside a word body.
There is no separate top-of-file data section, since Invisible Ink has no
such concept either.

```
load <name>     ; pushes the slot's current value
store <name>    ; pops a value and writes it to the slot
```

`load`/`store` on a bare address are also legal (`push 4`, then `load` /
`store`, exactly as in raw Invisible Ink) for cases with no natural name.

```
word main
    let counter = 0
    push 3
    store counter
    load counter
    num-out         ; prints 3
```

### Ops I just added to Ink

The ops I just added to the readme after I wrote them into the interpreter
exist in Quill too:

| Mnemonic | Stack effect | Codepoint |
|---|---|---|
| `rand`  | `n -- r` (uniform `0 <= r < n`) | `U+2063` |
| `sleep` | `ms --` (pause) | `U+200F` |
| `sock-create` | `-- fd` | `U+2065` |
| `sock-bind`   | `fd port -- rc` | `U+2066` |
| `sock-listen` | `fd backlog -- rc` | `U+2067` |
| `sock-accept` | `fd -- cfd` | `U+2068` |
| `sock-recv`   | `fd buf len -- n` | `U+2069` |
| `sock-send`   | `fd buf len -- n` | `U+206A` |
| `sock-close` / `close` / `file-close` | `fd -- rc` | `U+206B` |
| `file-open` | `path_ptr path_len flags -- fd` | `U+206C` |
| `file-read` | `fd buf len -- n` | `U+206D` |
| `file-stat` | `path_ptr path_len -- size \| -1 \| -2` | `U+206E` |
| `dir-list`  | `path_ptr path_len out_buf out_cap -- n` | `U+206F` |

`store`/`load` above already cover `U+2064` / `U+200E`.

### Round-tripping and disassembly

Disassembling raw Invisible Ink back to Quill is a near-exact inverse of
emission, with one caveat: word and slot names are assembler-side only 
and are not encoded in the codepoint stream.

A disassembler can recover correct structure but not original names. 
Round-tripped Quill is guaranteed to be *semantically* identical to the input;
it is not guaranteed to look like whatever Quill source (if any) originally 
produced that program.

Disassembly via `ink disassemble program.txt` or `ink dump program.txt` (or legacy `ink -d` / `quill disassemble`) dumps parsed words/blocks with placeholder names.

### Example

```
; Fibonacci, printed up to n terms.
word main
    let n = 10
    let a = 0
    let b = 1
    load n
    call loop
    halt

word loop               ; ( count -- )
    dup
    if {
        load a
        num-out
        load a
        load b
        add             ; a + b
        load b
        store a         ; a <- old b
        store b         ; b <- a + b
        push 1
        sub
        call loop
    } else {
        drop
    }
```

---

## Scribe

**Scribe is a high level language that compiles to Quill.** 
you can think of it like this: If Quill is assembly, Scribe is its C counterpart. 

```sh
ink scc program.s -o program.q        # Scribe -> Quill
ink asm program.q -o program.txt      # Quill -> Invisible Ink
ink run program.txt                   # run it
```

### Program shape and grammar

A program is a sequence of declarations, in any order (functions may call
functions defined later):

```
<global-decl>  := "let" NAME ["[" <int> "]"] ["=" <expr>] ";"
<fn-decl>      := "fn" NAME "(" [ NAME ["," NAME]* ] ")" "{" <stmt>* "}"
<stmt>         := "let" NAME [ "[" <int> "]" ] ["=" <expr>] ";"   ; local
               |  NAME "=" <expr> ";" |  NAME "[" <expr> "]" "=" <expr> ";"
               |  "if" "(" <expr> ")" "{" <stmt>* "}"
               |    [ "else" "{" <stmt>* "}" ]
               |  "while" "(" <expr> ")" "{" <stmt>* "}"
               |  "for" "(" <stmt> ";" <expr> ";" <stmt> ")" "{" <stmt>* "}"
               |  "return" [<expr>] ";"
               |  <builtin-call> ";"
               |  <expr> ";"                     ; e.g. a call for side effects
<expr>         := || / && / | ^ & / == != / < > <= >= / << >> / + - / * / %
                  unary - and ! / calls / peek(...) / literals / names / a[i]
```

`main` is required and takes zero arguments (arity is checked after the whole
file parses, so definition order doesn't matter).

### Builtins

| Scribe call | Meaning |
|---|---|
| `putc(n);` | print codepoint `n` |
| `putn(n);` | print `n` in decimal |
| `getc()` | read one codepoint, `-1` on EOF |
| `peek(a)` | read `mem[a]` |
| `poke(a, v);` | write `mem[a] = v` |
| `rnd(n)` | random integer in `[0, n)` |
| `sleep(ms);` | pause `ms` milliseconds |
| `exit();` | halt the program |

### Memory model

A Scribe program compiles to this Quill memory layout:

```
mem[0]      SP   dynamic stack pointer (top = end of the current frame)
mem[1]      FP   base of the current frame; frames are laid:
                 B+0        saved caller FP
                 B+1..N     args (slot 1 = first arg)
                 B+N..      locals (arrays take one cell per element)
                 B+ROFF     result slot (value rides out on the Quill stack)
                 B+DROFF    done flag (early-return control flow)
mem[2..]    globals, in declaration order; each array element = one cell
```

Function arguments live at `B+1` onward and locals right after them, so
addresses of locals are `FP + off` at runtime, while global addresses compile
to constants.

### Loops:

`while`/`for` compile to the ISA's native `while` op (`while { cond } {
body }`, two structured blocks). The condition block re-evaluates every
iteration, **costing zero call depth and emitting no hidden words**. A
million-iteration loop runs under the same 20,000-deep limit as any
straight-line program. In functions with early returns, the `done` flag is
checked inside the condition block, so a `return` fired mid-body exits the
loop on the next condition check. Visible when you `ink disassemble` a
compiled program: loops appear plainly as `while { ... } { ... }`.

Functions with early returns get a hidden `done` flag in their frame plus a
guard: when a `return` fires mid-body, any remaining statements in the block
(including nested blocks) are skipped. In such functions, `while` conditions
also test `done`, so a `return` from inside a loop body leaves the loop
cleanly on the next condition check.

### limitations

- `int64` only: no narrower/unsigned types, no casts, no floats.
- Arrays: one flat block, `base + i` addressing, length must be a literal,
  no initializers, no bounds checks at runtime.
- `&&`/`||` eager; no `break`/`continue` (restructure with flags or returns);
  no strings, no structs, no `~`, no `++`/`--`, no function pointers, no macros.
- Value builtins (`getc`, `peek`, `rnd`) work in expressions only; void
  builtins (`putc`, `putn`, `poke`, `sleep`, `exit`) work as statements only.

---

## Normalization policy

Several codepoints in this spec, `NBSP`, `EN/EM/THREE-PER-EM/FOUR-PER-EM/
SIX-PER-EM SPACE`, `IDEOGRAPHIC SPACE`, `FIGURE SPACE`, have compatibility
decompositions to plain `U+0020` under **NFKC**. Any pipeline that
NFKC-normalizes text will silently collapse `dup`/`swap`/`drop`/literal digits/etc.
down to indistinguishable `SPACE`s and destroy the program.

**Invisible Ink programs are NFC-only.** NFKC-normalizing a program is a
destructive, lossy transcoding, exactly like running any other non-ASCII
source file through the wrong encoding, and is simply out of spec, not
undefined behavior.

---

## Embedding (the `U+FEFF` marker)

A real UTF-8 BOM is only meaningful as the literal first three bytes of a
file. A `U+FEFF` appearing anywhere *else* was never a BOM to begin with,
which is what makes it safe to repurpose:

> An interpreter scans the input for `U+FEFF`. On finding one, it begins
> interpreting from the next codepoint and continues until `U+200B`
> (terminator) or EOF. Everything before the `FEFF` and after the `200B` is
> inert cover text.

One edge case worth knowing: a file whose first three bytes genuinely *are* a
UTF-8 BOM will have that codepoint consumed by whatever's reading the file as
"this is UTF-8," so it never reaches the interpreter as data. A program
relying on `FEFF`-at-position-0 specifically should account for that. in
practice, this just means the program lives spliced into the *middle* of the
cover text, never at absolute file-start, which is the natural way to embed
it anyway.

---

## IDE

The bundled editor (`src/editor.c`, built into the single `ink` binary) renders each spec codepoint as a 2-letter glyph (`it`/`nb`/`fg`/`th`/`ha`/`d0`/`d1`/`id`/`pu`/`mm`/`en`/`em`/`tp`/`nj`/`zj`/`wj`/`fa`/`lm`/`ls`/`ps`/`zs`/`bo`/`is`/`ip`/`lr`/`rl`/`sh`/`sc`/`sb`/`sl`/`sa`/`sr`/`ss`/`sx`/`fo`/`fr`/`fs`/`dl`/`an`/`or`/`xo`/`hl`/`hr`) so invisible ops are visible. `Ctrl-K` + two letters inserts the corresponding codepoint; `Ctrl-G` shows the full legend. All 43 current spec codepoints are covered, including `is` (rand), `ip` (store), `lr` (load), `rl` (sleep), `sh` (soft hyphen), `lm` (while), plus `sc`/`sb`/`sl`/`sa`/`sr`/`ss`/`sx` (sockets), `fo`/`fr`/`fs`/`dl` (files), and `an`/`or`/`xo`/`hl`/`hr` (ISA v2 bitwise: and/or/xor/shl/shr).

Launch via the unified CLI:

```sh
ink edit myfile.txt      # or ink ide myfile.txt
ink edit --selftest      # run editor self-tests
# legacy compatibility shims (forward to `ink edit`):
inkide myfile.txt
InkIDE myfile.txt
```

Build is modular: `make` compiles `src/*.c` → `src/ink` (still a single binary).

---

## Full codepoint table

| Codepoint | Name | Op |
|---|---|---|
| U+2062 | INVISIBLE TIMES | `dup` |
| U+00A0 | NO-BREAK SPACE | `swap` |
| U+2007 | FIGURE SPACE | `drop` |
| U+2009 | THIN SPACE | `rot` |
| U+200A | HAIR SPACE | push literal |
| U+2005 | FOUR-PER-EM SPACE | literal digit `0` |
| U+2006 | SIX-PER-EM SPACE | literal digit `1` |
| U+3000 | IDEOGRAPHIC SPACE | numeric print (no newline) |
| U+2008 | PUNCTUATION SPACE | char-out |
| U+205F | MEDIUM MATHEMATICAL SPACE | char-in (`-1` on EOF) |
| U+2063 | INVISIBLE SEPARATOR | `rand` (`n -- r`, `0 <= r < n`) |
| U+2064 | INVISIBLE PLUS | `store` (`addr value --`) |
| U+200E | LEFT-TO-RIGHT MARK | `load` (`addr -- value`) |
| U+200F | RIGHT-TO-LEFT MARK | `sleep` (`ms --`) |
| U+2065 | NEXT LINE | `sock-create` (`-- fd`) |
| U+2066 | LEFT-TO-RIGHT ISOLATE | `sock-bind` (`fd port -- rc`) |
| U+2067 | RIGHT-TO-LEFT ISOLATE | `sock-listen` (`fd backlog -- rc`) |
| U+2068 | FIRST STRONG ISOLATE | `sock-accept` (`fd -- cfd`) |
| U+2069 | POP DIRECTIONAL ISOLATE | `sock-recv` (`fd buf len -- n`) |
| U+206A | INHIBIT SYMMETRIC SWAPPING | `sock-send` (`fd buf len -- n`) |
| U+206B | ACTIVATE SYMMETRIC SWAPPING | `sock-close` (`fd -- rc`) |
| U+206C | INHIBIT ARABIC FORM SHAPING | `file-open` (`path_ptr path_len flags -- fd`) |
| U+206D | ACTIVATE ARABIC FORM SHAPING | `file-read` (`fd buf len -- n`) |
| U+206E | NATIONAL DIGIT SHAPES | `file-stat` (`path_ptr path_len -- size/-1/-2`) |
| U+206F | NOMINAL DIGIT SHAPES | `dir-list` (`path_ptr path_len out_buf out_cap -- n`) |
| U+061C | ARABIC LETTER MARK | `while` (`cond` block + `body` block, see Control flow) |
| U+00AD | SOFT HYPHEN | literal terminator + `char-out` |
| U+2002 | EN SPACE | arity: negate / subtract / modulo |
| U+2003 | EM SPACE | arity: identity / add / divide |
| U+2004 | THREE-PER-EM SPACE | arity: sign / multiply / exponent |
| U+200C | ZERO WIDTH NON-JOINER | block open |
| U+200D | ZERO WIDTH JOINER | block close |
| U+2060 | WORD JOINER | if / then / else |
| U+2061 | FUNCTION APPLICATION | call word `n` |
| U+2000 | EN QUAD | statement boundary / literal terminator |
| U+2001 | EM QUAD | word definition boundary |
| U+200B | ZERO WIDTH SPACE | program terminator |
| U+FEFF | ZERO WIDTH NO-BREAK SPACE | begin embedded program |
| U+202A | LEFT-TO-RIGHT EMBEDDING | `and` (`a b -- a&b`) |
| U+202B | RIGHT-TO-LEFT EMBEDDING | `or` (`a b -- a\|b`) |
| U+202C | POP DIRECTIONAL FORMATTING | `xor` (`a b -- a^b`) |
| U+202D | LEFT-TO-RIGHT OVERRIDE | `shl` (`a b -- a << (b&63)`) |
| U+202E | RIGHT-TO-LEFT OVERRIDE | `shr` (`a b -- (uint64)a >> (b&63)`) |

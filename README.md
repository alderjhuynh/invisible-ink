# Invisible Ink

Whitespace, except worse. Every instruction is an invisible or zero-width Unicode
codepoint, no two of which look alike, and none of which you can see anyway. Several of
these are one editor-setting away from being silently destroyed. 

A program is a sequence of invisible codepoints, plus as much visible "cover text"
as you like: **any character not defined below is ignored entirely.** This allows you to splice a `.txt` document with Invisible Ink, and that's the language's entire purpose. Critically, `U+0020 SPACE` itself is *not* one of the spec codepoints, so ordinary prose (which is mostly made of spaces) is always safe cover text: a program can be laced anywhere in a document, including in the middle of a sentence, not just at paragraph breaks.

Writing this language is genuinely awful, so I've made its own text editor. Just to let you get a sense of how painful this language is to write, I've added the raw inputs I wrote in the IDE for the blackjack example, you can view the raw here: [view the blackjack raw](blackjack-raw.md)

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

Recursion plus `if/then/else` is the language's only control-flow primitive;
combined with word calls (below), that's enough for arbitrary loops without a
dedicated loop construct.

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

### Control flow

| Codepoint | Name | Role |
|---|---|---|
| U+200C | ZERO WIDTH NON-JOINER | block open |
| U+200D | ZERO WIDTH JOINER | block close |
| U+2060 | WORD JOINER | `if` / `then` / `else` (see below) |
| U+2061 | FUNCTION APPLICATION | `call` - pop `n`, run word `n`, return here when it finishes |
| U+2000 | EN QUAD | statement boundary |
| U+2001 | EM QUAD | word definition boundary |
| U+200B | ZERO WIDTH SPACE | program terminator |
| U+FEFF | ZERO WIDTH NO-BREAK SPACE | begin embedded program (see *Embedding*) |

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

- stack underflow (`dup`/`swap`/`drop`/`rot`/arity ops/`call`/`if`/`num-out`/`char-out`/`rand`/`store`/`load`/`sleep` needing a value that isn't there)
- divide-by-zero or modulo-by-zero
- malformed literal (empty, or unterminated; not closed by `EN QUAD` or `SOFT HYPHEN`)
- `rand` with `n <= 0`
- `store`/`load` with negative address
- `call` to undefined word (index `< 0` or `>= word count`)
- call depth exceeded (recursion > 20,000)
- step limit exceeded (10,000,000 instructions, guards against infinite loops)

`char-in` at EOF is *not* an error. It pushes `-1`.

---

## Runtime limits

- **Steps:** 10,000,000 ops per run (`MAX_STEPS` in `src/invisible-ink.c:314`). Override by editing the source.
- **Call depth:** 20,000 nested `call`s.
- **Memory:** heap-grown (`g_mem`), doubled as needed, zero-filled.
- **I/O:** `char-in` reads UTF-8 codepoints from stdin; `char-out`/`num-out`/`sleep`/`char-in` all `fflush(stdout)` before blocking/sleeping.

Disassemble without running: `./ink -d program.txt` dumps parsed words/blocks.

## Quill (mnemonic assembly)

**Quill is the mnemonic assembly language for Invisible Ink.** You write Quill
(`*.q`), the assembler inks it. Every mnemonic compiles to a specific run of
invisible Unicode codepoints, and a disassembler can recover a structurally
equivalent Quill program from any Invisible Ink source. 

### Usage

```sh
./quill assemble program.q -o program.txt
./quill disassemble program.txt
./quill disassemble program.txt -o program.q
```

Examples in `examples/quill/` (`add.q`, `fib.q`, `countdown.q`, `echo.q`).

### Lexical structure

- Tokens are separated by any whitespace (spaces, tabs, newlines).
- `;` starts a comment that runs to end of line. Comments are stripped
  entirely and never affect emitted output.
- Identifiers (word names, memory slot names): `[a-zA-Z_][a-zA-Z0-9_-]*`.
- Integer literals: an optional `-`, then one or more decimal digits.
- Keywords: `word`, `if`, `else`, `let`, `call`, `halt`, and the op
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

Like in Ink, there is no dedicated loop construct: a word
that `call`s itself, guarded by an `if`, is the only iteration mechanism.

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

`store`/`load` above already cover `U+2064` / `U+200E`.

### Round-tripping and disassembly

Disassembling raw Invisible Ink back to Quill is a near-exact inverse of
emission, with one caveat: word and slot names are assembler-side only 
and are not encoded in the codepoint stream.

A disassembler can recover correct structure but not original names. 
Round-tripped Quill is guaranteed to be *semantically* identical to the input;
it is not guaranteed to look like whatever Quill source (if any) originally 
produced that program.

Disassembly via `./ink -d program.txt` or `./quill disassemble program.txt`
dumps parsed words/blocks with these placeholder names.

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

The bundled editor (`ide/ink-ide.c`, built as `InkIDE`) renders each spec codepoint as a 2-letter glyph (`it`/`nb`/`fg`/`th`/`ha`/`d0`/`d1`/`id`/`pu`/`mm`/`en`/`em`/`tp`/`nj`/`zj`/`wj`/`fa`/`ls`/`ps`/`zs`/`bo`/`is`/`ip`/`lr`/`rl`/`sh`) so invisible ops are visible. `Ctrl-K` + two letters inserts the corresponding codepoint; `Ctrl-G` shows the full legend. All 26 current spec codepoints are covered, including `is` (rand), `ip` (store), `lr` (load), `rl` (sleep), `sh` (soft hyphen).

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

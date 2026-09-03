# Invisible Ink

Whitespace, except worse. Every instruction is an invisible or zero-width Unicode
codepoint, no two of which look alike, and none of which you can see anyway. Several of
these are one editor-setting away from being silently destroyed. 

A program is a sequence of invisible codepoints, plus as much visible "cover text"
as you like: **any character not defined below is ignored entirely.** This allows you to splice a `.txt` document with Invisible Ink, and that's the language's entire purpose.

Writing this language is genuinely awful, so I've made its own text editor.

---

## Program structure

An Invisible Ink source consists of one or more **words**, separated by
`U+2029 PARAGRAPH SEPARATOR`:

- Everything before the first `¶` is **word 0**, the entry point, and runs
  automatically.
- Each subsequent `¶`-delimited chunk is word 1, 2, 3, ... in definition order.
- A word returns implicitly when it hits the next `¶` or the program terminator.
- Within a word, `U+2028 LINE SEPARATOR` marks statement boundaries (and also
  closes literals).

Recursion plus `if/then/else` is the language's only control-flow primitive;
combined with word calls (below), that's enough for arbitrary loops without a
dedicated loop construct.

---

## Operator reference

### Stack manipulation

| Codepoint | Name | Op | Stack effect |
|---|---|---|---|
| U+0020 | SPACE | `dup` | `a -- a a` |
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
most-significant bit first, and the literal closes at the next `LINE SEPARATOR`
(which also still ends the statement; no separate terminator needed):

| Codepoint | Name | Meaning |
|---|---|---|
| U+200A | HAIR SPACE | begin literal |
| U+2005 | FOUR-PER-EM SPACE | binary digit `0` |
| U+2006 | SIX-PER-EM SPACE | binary digit `1` |
| U+2028 | LINE SEPARATOR | close literal / end statement |

Example: pushing `5` (`101`): `HAIR-SPACE SIX-PER-EM FOUR-PER-EM SIX-PER-EM LINE-SEPARATOR`.

### I/O

| Codepoint | Name | Op |
|---|---|---|
| U+3000 | IDEOGRAPHIC SPACE | `num-out` - pop, print as a number, newline |
| U+2008 | PUNCTUATION SPACE | `char-out` - pop, print as a Unicode codepoint (no newline) |
| U+205F | MEDIUM MATHEMATICAL SPACE | `char-in` - read one codepoint, push its code point; pushes `-1` on EOF |

Numeric input isn't a dedicated op. Do `char-in` in a loop plus your own parsing.

### Control flow

| Codepoint | Name | Role |
|---|---|---|
| U+200C | ZERO WIDTH NON-JOINER | block open |
| U+200D | ZERO WIDTH JOINER | block close |
| U+2060 | WORD JOINER | `if` / `then` / `else` (see below) |
| U+2061 | FUNCTION APPLICATION | `call` - pop `n`, run word `n`, return here when it finishes |
| U+2028 | LINE SEPARATOR | statement boundary |
| U+2029 | PARAGRAPH SEPARATOR | word definition boundary |
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

**Halt-on-error**, matching original Whitespace: stack underflow,
divide-by-zero, or a malformed literal all halt the program immediately.
`char-in` at EOF is *not* an error.

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

## Full codepoint table

| Codepoint | Name | Op |
|---|---|---|
| U+0020 | SPACE | `dup` |
| U+00A0 | NO-BREAK SPACE | `swap` |
| U+2007 | FIGURE SPACE | `drop` |
| U+2009 | THIN SPACE | `rot` |
| U+200A | HAIR SPACE | push literal |
| U+2005 | FOUR-PER-EM SPACE | literal digit `0` |
| U+2006 | SIX-PER-EM SPACE | literal digit `1` |
| U+3000 | IDEOGRAPHIC SPACE | numeric print + newline |
| U+2008 | PUNCTUATION SPACE | char-out |
| U+205F | MEDIUM MATHEMATICAL SPACE | char-in (`-1` on EOF) |
| U+2002 | EN SPACE | arity: negate / subtract / modulo |
| U+2003 | EM SPACE | arity: identity / add / divide |
| U+2004 | THREE-PER-EM SPACE | arity: sign / multiply / exponent |
| U+200C | ZERO WIDTH NON-JOINER | block open |
| U+200D | ZERO WIDTH JOINER | block close |
| U+2060 | WORD JOINER | if / then / else |
| U+2061 | FUNCTION APPLICATION | call word `n` |
| U+2028 | LINE SEPARATOR | statement boundary / literal terminator |
| U+2029 | PARAGRAPH SEPARATOR | word definition boundary |
| U+200B | ZERO WIDTH SPACE | program terminator |
| U+FEFF | ZERO WIDTH NO-BREAK SPACE | begin embedded program |
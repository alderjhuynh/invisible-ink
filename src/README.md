# Invisible Ink interpreter

A single-file, dependency-free C11 interpreter for the Invisible Ink
esolang.

## Build

```sh
make
```

Produces an `ink` binary. No external dependencies.

## Run

```sh
./ink program.txt          # run
./ink -d program.txt       # disassemble (dumps parsed AST)
```

Input file is read as UTF-8. Program output goes to stdout;
`char-in` reads UTF-8 codepoints from stdin, one at a time, as the
program asks for them, so interactive programs (echo loops, etc.)
work at a terminal as well as with piped input.

Some examples are included.

## Quill (mnemonic assembly)

Quill is a readable assembly over Invisible Ink. See `../README.md` (Quill section) for the full spec.

```sh
python3 ../quill.py assemble program.q -o program.txt   # assemble
python3 ../quill.py disassemble program.txt -o program.q # disassemble
../quill assemble program.q -o program.txt               # via wrapper
```

- `word <name>` defines words; first is entry (word 0).
- `push <int>`, `dup`/`swap`/`drop`/`rot`, `neg`/`sub`/`mod`/`id`/`add`/`div`/`sign`/`mul`/`pow`, `num-out`/`char-out`/`char-in`, `rand`/`sleep`, `load`/`store`, `call <word>`/`halt`, `if { } else { }`, `let <slot> = <int>`.
- Word order defines indices; `call` resolves names. `let` assigns sequential memory addresses; `load`/`store <name>` sugar emits `push <addr>` + `load` / `push <addr>` + `swap` + `store`.
- `push -5` is sugar for `push 5` + `neg`.
- Tokens are whitespace-separated; `;` starts comment to end of line.
```


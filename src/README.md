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


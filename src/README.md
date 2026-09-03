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

Two examples are included:

```sh
./ink examples/add.txt        # prints 7
./ink examples/countdown.txt  # prints 5 4 3 2 1, spliced into a page of cover text
```

`countdown.txt` demonstrates embedding: open it in a text editor and
you'll see only prose (and a large amount of whitespace) the program lives in invisible codepoints
between a `U+FEFF` marker and a `U+200B` terminator inside that text.


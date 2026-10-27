fn main() {
    putn(~12);      /* -13: bitwise complement is push -1; xor */
    putc(10);
    putn(~0);       /* -1 */
    putc(10);
    putn(~-1);      /* 0 */
    putc(10);
    buf_ops();
    return 0;
}

let buf = "AB\n";     /* 2 byte cells per character + one 0 terminator */

fn buf_ops() {
    /* string globals are addressable byte arrays */
    let i = 0;
    while (peek(&buf + i) != 0) {
        putc(peek(&buf + i));
        i = i + 1;
    }
    poke(&buf + 0, 97);       /* rewrite the first byte */
    putc(peek(&buf + 0));
    putc(10);
    putn(peek(&buf + 3));     /* terminator cell = 0 */
    putc(10);
    return 0;
}

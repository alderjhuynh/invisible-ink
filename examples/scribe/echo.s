fn main() {
    let c = getc();
    while (c >= 0) {
        putc(c);
        c = getc();
    }
    return 0;
}

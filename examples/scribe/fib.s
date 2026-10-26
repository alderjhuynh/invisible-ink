fn fib(n) {
    if (n < 2) { return n; }
    return fib(n - 1) + fib(n - 2);
}

fn main() {
    let i = 0;
    while (i < 11) {
        putn(fib(i));
        putc(32);
        i = i + 1;
    }
    putc(10);
    return 0;
}

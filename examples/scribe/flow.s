fn first_factor(x) {
    let i = 2;
    while (i * i <= x) {
        if (x % i == 0) { return i; }
        i = i + 1;
    }
    return x;
}

fn main() {
    for (let k = 2; k <= 10; k = k + 1) {
        putn(first_factor(k));
        putc(32);
    }
    putc(10);
    return 0;
}

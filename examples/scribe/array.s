let squares[8];
let count = 0;

fn main() {
    for (let i = 0; i < 8; i = i + 1) {
        squares[i] = i * i;
        count = count + 1;
    }
    let j = 0;
    while (j < 8) {
        putn(squares[j]);
        putc(32);
        j = j + 1;
    }
    putc(10);
    putn(count);
    putc(10);
    return 0;
}

fn main() {
    putn(12 & 10);           // 8
    putc(32);
    putn(12 | 10);           // 14
    putc(32);
    putn(12 ^ 10);           // 6
    putc(32);
    putn(1 << 4);            // 16
    putc(32);
    putn(64 >> 3);           // 8
    putc(32);
    let n = -1;
    putn(n >> 8);            // logical shift of 0xFFFF..FF >> 8
    putc(10);
    return 0;
}

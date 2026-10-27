let sintab[33];     // sin(i * pi/64) * 1024, i = 0..32 (one quadrant)
let vert[24];       // 8 vertices, x/y/z per vertex, fixed point +/-512
let ed[24];         // 12 edges as vertex-index pairs
let buf[1920];      // framebuffer, 80x24
let px8[8];         // projected screen x per vertex
let py8[8];         // projected screen y per vertex
let ya = 0;         // yaw angle index   (0..127)
let xa = 0;         // pitch angle index (0..127)
let ysa = 0;        // yaw sin,   fixed point
let yca = 0;        // yaw cos,   fixed point
let xsa = 0;        // pitch sin, fixed point
let xca = 0;        // pitch cos, fixed point

fn tsin(a) {
    let q = a / 32;
    let r = a - q * 32;
    let v = 0;
    if (q == 0) { v = sintab[r]; }
    if (q == 1) { v = sintab[32 - r]; }
    if (q == 2) { v = 0 - sintab[r]; }
    if (q == 3) { v = 0 - sintab[32 - r]; }
    return v;
}

fn tcos(a) {
    let q = a / 32;
    let r = a - q * 32;
    let v = 0;
    if (q == 0) { v = sintab[32 - r]; }
    if (q == 1) { v = 0 - sintab[r]; }
    if (q == 2) { v = 0 - sintab[32 - r]; }
    if (q == 3) { v = sintab[r]; }
    return v;
}

fn rotproj(i) { 
    let x = vert[i * 3];
    let y = vert[i * 3 + 1];
    let z = vert[i * 3 + 2];
    let x2 = (x * yca + z * ysa) / 1024;
    let z1 = (z * yca - x * ysa) / 1024;
    let y2 = (y * xca - z1 * xsa) / 1024;
    let z2 = (y * xsa + z1 * xca) / 1024;
    let zz = z2 + 4096;
    px8[i] = 40 + (x2 * 80) / zz;
    py8[i] = 12 + ((y2 * 80) / zz) / 2;
    return 0;
}

fn line(x0, y0, x1, y1) {
    let dx = x1 - x0;
    let sx = 1;
    if (dx < 0) { sx = 0 - 1; dx = 0 - dx; }
    let dy = y1 - y0;
    let sy = 1;
    if (dy < 0) { sy = 0 - 1; dy = 0 - dy; }
    let err = dx - dy;
    let px = x0;
    let py = y0;
    while (px != x1 || py != y1) {
        if (px >= 0) { if (px < 80) { if (py >= 0) { if (py < 24) {
            buf[py * 80 + px] = 35;
        } } } }
        let e2 = err * 2;
        if (e2 > 0 - dy) { err = err - dy; px = px + sx; }
        if (e2 < dx)     { err = err + dx; py = py + sy; }
    }
    if (x1 >= 0) { if (x1 < 80) { if (y1 >= 0) { if (y1 < 24) {
        buf[y1 * 80 + x1] = 35;
    } } } }
    return 0;
}

fn main() {
    let i = 0;
    let k = 0;

    sintab[0] = 0;     sintab[1] = 50;    sintab[2] = 100;   sintab[3] = 150;
    sintab[4] = 200;   sintab[5] = 249;   sintab[6] = 297;   sintab[7] = 345;
    sintab[8] = 392;   sintab[9] = 438;   sintab[10] = 483;  sintab[11] = 526;
    sintab[12] = 569;  sintab[13] = 610;  sintab[14] = 650;  sintab[15] = 688;
    sintab[16] = 724;  sintab[17] = 759;  sintab[18] = 792;  sintab[19] = 822;
    sintab[20] = 851;  sintab[21] = 878;  sintab[22] = 903;  sintab[23] = 926;
    sintab[24] = 946;  sintab[25] = 964;  sintab[26] = 980;  sintab[27] = 993;
    sintab[28] = 1004; sintab[29] = 1013; sintab[30] = 1019; sintab[31] = 1023;
    sintab[32] = 1024;

    for (i = 0; i < 8; i = i + 1) {
        vert[i * 3]     = (i % 2) * 1024 - 512;
        vert[i * 3 + 1] = ((i / 2) % 2) * 1024 - 512;
        vert[i * 3 + 2] = (i / 4) * 1024 - 512;
    }

    k = 0;
    for (i = 0; i < 8; i = i + 1) {
        let b = 1;
        while (b < 8) {
            let j = i ^ b;
            if (j > i) {
                ed[k * 2] = i;
                ed[k * 2 + 1] = j;
                k = k + 1;
            }
            b = b * 2;
        }
    }

    let t = 0;
    while (1) {
        ya = t & 127;
        xa = (t * 3) & 127;
        ysa = tsin(ya); yca = tcos(ya);
        xsa = tsin(xa); xca = tcos(xa);

        for (i = 0; i < 1920; i = i + 1) { buf[i] = 32; }

        for (i = 0; i < 8; i = i + 1) { rotproj(i); }

        for (k = 0; k < 12; k = k + 1) {
            line(px8[ed[k * 2]], py8[ed[k * 2]], px8[ed[k * 2 + 1]], py8[ed[k * 2 + 1]]);
        }

        for (let r = 0; r < 24; r = r + 1) {
            for (let c = 0; c < 80; c = c + 1) { putc(buf[r * 80 + c]); }
            putc(10);
        }

        sleep(50);
        t = t + 1;
    }
    return 0;
}

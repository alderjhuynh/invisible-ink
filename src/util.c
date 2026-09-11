#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

bool is_spec_cp(cp_t c) {
    switch (c) {
        case CP_ITIMES: case CP_NBSP: case CP_FIGURE: case CP_THIN:
        case CP_HAIR: case CP_DIG0: case CP_DIG1:
        case CP_IDEO: case CP_PUNCT: case CP_MEDMATH:
        case CP_EN: case CP_EM: case CP_THREE:
        case CP_ZWNJ: case CP_ZWJ: case CP_WJ: case CP_FA:
        case CP_LS: case CP_PS: case CP_ZWSP: case CP_BOM:
        case CP_INV_SEP: case CP_INV_PLUS: case CP_LRM: case CP_RLM: case CP_SOFT:
            return true;
        default:
            return false;
    }
}

cp_t *utf8_decode(const unsigned char *buf, size_t len, size_t *out_count) {
    cp_t *out = malloc(len * sizeof(cp_t));
    if (!out) { perror("malloc"); exit(1); }
    size_t n = 0, i = 0;
    while (i < len) {
        unsigned char b0 = buf[i];
        uint32_t cp; int extra;
        if (b0 < 0x80)              { cp = b0; extra = 0; }
        else if ((b0 & 0xE0) == 0xC0){ cp = b0 & 0x1F; extra = 1; }
        else if ((b0 & 0xF0) == 0xE0){ cp = b0 & 0x0F; extra = 2; }
        else if ((b0 & 0xF8) == 0xF0){ cp = b0 & 0x07; extra = 3; }
        else { out[n++] = 0xFFFD; i++; continue; }

        if (i + (size_t)extra >= len) { out[n++] = 0xFFFD; i++; continue; }
        bool ok = true;
        uint32_t val = cp;
        for (int k = 1; k <= extra; k++) {
            unsigned char bk = buf[i + k];
            if ((bk & 0xC0) != 0x80) { ok = false; break; }
            val = (val << 6) | (bk & 0x3F);
        }
        if (!ok) { out[n++] = 0xFFFD; i++; continue; }
        out[n++] = val;
        i += (size_t)extra + 1;
    }
    *out_count = n;
    return out;
}

size_t utf8_encode_cp(cp_t cp, unsigned char *out){
    if(cp<0x80){ out[0]=(unsigned char)cp; return 1; }
    else if(cp<0x800){ out[0]=(unsigned char)(0xC0|(cp>>6)); out[1]=(unsigned char)(0x80|(cp&0x3F)); return 2; }
    else if(cp<0x10000){ out[0]=(unsigned char)(0xE0|(cp>>12)); out[1]=(unsigned char)(0x80|((cp>>6)&0x3F)); out[2]=(unsigned char)(0x80|(cp&0x3F)); return 3; }
    else { out[0]=(unsigned char)(0xF0|(cp>>18)); out[1]=(unsigned char)(0x80|((cp>>12)&0x3F)); out[2]=(unsigned char)(0x80|((cp>>6)&0x3F)); out[3]=(unsigned char)(0x80|(cp&0x3F)); return 4; }
}

void utf8_put(int64_t code) {
    uint32_t c;
    if (code < 0 || code > 0x10FFFF || (code >= 0xD800 && code <= 0xDFFF)) {
        c = 0xFFFD;
    } else {
        c = (uint32_t)code;
    }
    if (c < 0x80) {
        putchar((int)c);
    } else if (c < 0x800) {
        putchar((int)(0xC0 | (c >> 6)));
        putchar((int)(0x80 | (c & 0x3F)));
    } else if (c < 0x10000) {
        putchar((int)(0xE0 | (c >> 12)));
        putchar((int)(0x80 | ((c >> 6) & 0x3F)));
        putchar((int)(0x80 | (c & 0x3F)));
    } else {
        putchar((int)(0xF0 | (c >> 18)));
        putchar((int)(0x80 | ((c >> 12) & 0x3F)));
        putchar((int)(0x80 | ((c >> 6) & 0x3F)));
        putchar((int)(0x80 | (c & 0x3F)));
    }
}

int64_t utf8_get_stdin(void) {
    int b0 = getchar();
    if (b0 == EOF) return -1;
    uint32_t cp; int extra;
    if (b0 < 0x80)               { cp = (uint32_t)b0; extra = 0; }
    else if ((b0 & 0xE0) == 0xC0){ cp = (uint32_t)(b0 & 0x1F); extra = 1; }
    else if ((b0 & 0xF0) == 0xE0){ cp = (uint32_t)(b0 & 0x0F); extra = 2; }
    else if ((b0 & 0xF8) == 0xF0){ cp = (uint32_t)(b0 & 0x07); extra = 3; }
    else return 0xFFFD;
    for (int k = 0; k < extra; k++) {
        int bk = getchar();
        if (bk == EOF) return 0xFFFD;
        if ((bk & 0xC0) != 0x80) return 0xFFFD;
        cp = (cp << 6) | (uint32_t)(bk & 0x3F);
    }
    return (int64_t)cp;
}

unsigned char *read_file_raw(const char *path, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "%s: %s\n", path, strerror(errno)); return NULL; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    if (sz<0) sz=0;
    fseek(f, 0, SEEK_SET);
    unsigned char *buf = malloc((size_t)sz+1);
    size_t got = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    *out_len = got;
    return buf;
}

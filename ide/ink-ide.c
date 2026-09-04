#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <stdarg.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <signal.h>

typedef uint32_t cp_t;

typedef enum {
    CAT_STACK = 1,   /* dup/swap/drop/rot                            -> cyan   */
    CAT_LITERAL,     /* literal open + binary digits                 -> yellow */
    CAT_IO,          /* num-out/char-out/char-in                     -> green  */
    CAT_ARITH,       /* arity-dispatched EN/EM/THREE                 -> magenta*/
    CAT_BLOCK,       /* ZWNJ/ZWJ/WJ/FA (blocks, if, call)            -> blue */
    CAT_STRUCT,      /* LS/PS/ZWSP/BOM (document structure)          -> red */
    CAT_DIM,         /* tab / CR / raw control bytes                 -> dim grey */
    CAT_SUSPECT      /* non-spec but easily-confused invisible cp    -> dim yellow */
} Category;

typedef struct {
    cp_t cp;
    const char label[3];
    const char *name;
    const char *op;
    Category cat;
} SpecEntry;

static const SpecEntry SPEC_TABLE[] = {
    { 0x2062, "it", "INVISIBLE TIMES",           "dup",                  CAT_STACK   },
    { 0x00A0, "nb", "NO-BREAK SPACE",            "swap",                 CAT_STACK   },
    { 0x2007, "fg", "FIGURE SPACE",              "drop",                 CAT_STACK   },
    { 0x2009, "th", "THIN SPACE",                "rot",                  CAT_STACK   },
    { 0x200A, "ha", "HAIR SPACE",                "begin literal",        CAT_LITERAL },
    { 0x2005, "d0", "FOUR-PER-EM SPACE",         "literal digit 0",      CAT_LITERAL },
    { 0x2006, "d1", "SIX-PER-EM SPACE",          "literal digit 1",      CAT_LITERAL },
    { 0x3000, "id", "IDEOGRAPHIC SPACE",         "num-out",              CAT_IO      },
    { 0x2008, "pu", "PUNCTUATION SPACE",         "char-out",             CAT_IO      },
    { 0x205F, "mm", "MEDIUM MATHEMATICAL SPACE", "char-in",              CAT_IO      },
    { 0x2002, "en", "EN SPACE",                  "arity: neg/sub/mod",   CAT_ARITH   },
    { 0x2003, "em", "EM SPACE",                  "arity: id/add/div",    CAT_ARITH   },
    { 0x2004, "tp", "THREE-PER-EM SPACE",        "arity: sign/mul/pow",  CAT_ARITH   },
    { 0x200C, "nj", "ZERO WIDTH NON-JOINER",     "block open",           CAT_BLOCK   },
    { 0x200D, "zj", "ZERO WIDTH JOINER",         "block close",          CAT_BLOCK   },
    { 0x2060, "wj", "WORD JOINER",               "if / then / else",     CAT_BLOCK   },
    { 0x2061, "fa", "FUNCTION APPLICATION",      "call",                 CAT_BLOCK   },
    { 0x2000, "ls", "EN QUAD",                   "statement boundary",   CAT_STRUCT  },
    { 0x2001, "ps", "EM QUAD",                   "word boundary",        CAT_STRUCT  },
    { 0x200B, "zs", "ZERO WIDTH SPACE",          "program terminator",   CAT_STRUCT  },
    { 0xFEFF, "bo", "ZERO WIDTH NO-BREAK SPACE", "begin embedded pgm",   CAT_STRUCT  },
    { 0x2063, "is", "INVISIBLE SEPARATOR",       "rand",                 CAT_IO      },
    { 0x2064, "ip", "INVISIBLE PLUS",            "store",                CAT_STACK   },
    { 0x200E, "lr", "LEFT-TO-RIGHT MARK",        "load",                 CAT_STACK   },
    { 0x200F, "rl", "RIGHT-TO-LEFT MARK",        "sleep",                CAT_IO      },
    { 0x00AD, "sh", "SOFT HYPHEN",               "lit+char-out",         CAT_LITERAL },
};
#define SPEC_COUNT (sizeof(SPEC_TABLE) / sizeof(SPEC_TABLE[0]))

typedef struct { cp_t lo, hi; const char *name; } ConfusableRange;
static const ConfusableRange CONFUSABLES[] = {
    { 0x180E, 0x180E, "MONGOLIAN VOWEL SEPARATOR"},
    { 0x061C, 0x061C, "ARABIC LETTER MARK" },
    { 0x202F, 0x202F, "NARROW NO-BREAK SPACE" },
    { 0xFE00, 0xFE0F, "VARIATION SELECTOR" },
    { 0xFFF9, 0xFFFB, "INTERLINEAR ANNOTATION" },
};
#define CONFUSABLE_COUNT (sizeof(CONFUSABLES) / sizeof(CONFUSABLES[0]))

static const SpecEntry *spec_by_cp(cp_t cp) {
    for (size_t i = 0; i < SPEC_COUNT; i++)
        if (SPEC_TABLE[i].cp == cp) return &SPEC_TABLE[i];
    return NULL;
}

static const SpecEntry *spec_by_mnemonic(int c1, int c2) {
    char a = (char)tolower(c1), b = (char)tolower(c2);
    for (size_t i = 0; i < SPEC_COUNT; i++)
        if (SPEC_TABLE[i].label[0] == a && SPEC_TABLE[i].label[1] == b)
            return &SPEC_TABLE[i];
    return NULL;
}

static bool is_confusable(cp_t cp, const char **name_out) {
    for (size_t i = 0; i < CONFUSABLE_COUNT; i++)
        if (cp >= CONFUSABLES[i].lo && cp <= CONFUSABLES[i].hi) {
            if (name_out) *name_out = CONFUSABLES[i].name;
            return true;
        }
    return false;
}

static cp_t *utf8_decode_buf(const unsigned char *buf, size_t len, size_t *out_count) {
    cp_t *out = malloc((len ? len : 1) * sizeof(cp_t));
    if (!out) { perror("malloc"); exit(1); }
    size_t n = 0, i = 0;
    while (i < len) {
        unsigned char b0 = buf[i];
        uint32_t cp; int extra;
        if (b0 < 0x80)                { cp = b0; extra = 0; }
        else if ((b0 & 0xE0) == 0xC0) { cp = b0 & 0x1F; extra = 1; }
        else if ((b0 & 0xF0) == 0xE0) { cp = b0 & 0x0F; extra = 2; }
        else if ((b0 & 0xF8) == 0xF0) { cp = b0 & 0x07; extra = 3; }
        else { out[n++] = 0xFFFD; i++; continue; }

        if (i + (size_t)extra >= len) { out[n++] = 0xFFFD; i++; continue; }
        bool ok = true;
        uint32_t val = cp;
        for (int k = 1; k <= extra; k++) {
            unsigned char bk = buf[i + (size_t)k];
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

typedef struct { unsigned char *data; size_t len, cap; } ByteBuf;

static void bb_ensure(ByteBuf *b, size_t more) {
    if (b->len + more <= b->cap) return;
    size_t newcap = b->cap ? b->cap * 2 : 256;
    while (newcap < b->len + more) newcap *= 2;
    b->data = realloc(b->data, newcap);
    if (!b->data) { perror("realloc"); exit(1); }
    b->cap = newcap;
}

static void bb_put_byte(ByteBuf *b, unsigned char c) {
    bb_ensure(b, 1);
    b->data[b->len++] = c;
}

static void utf8_encode_append(ByteBuf *b, cp_t code) {
    uint32_t c = code;
    if (code > 0x10FFFF || (code >= 0xD800 && code <= 0xDFFF)) c = 0xFFFD;
    if (c < 0x80) {
        bb_put_byte(b, (unsigned char)c);
    } else if (c < 0x800) {
        bb_ensure(b, 2);
        bb_put_byte(b, (unsigned char)(0xC0 | (c >> 6)));
        bb_put_byte(b, (unsigned char)(0x80 | (c & 0x3F)));
    } else if (c < 0x10000) {
        bb_ensure(b, 3);
        bb_put_byte(b, (unsigned char)(0xE0 | (c >> 12)));
        bb_put_byte(b, (unsigned char)(0x80 | ((c >> 6) & 0x3F)));
        bb_put_byte(b, (unsigned char)(0x80 | (c & 0x3F)));
    } else {
        bb_ensure(b, 4);
        bb_put_byte(b, (unsigned char)(0xF0 | (c >> 18)));
        bb_put_byte(b, (unsigned char)(0x80 | ((c >> 12) & 0x3F)));
        bb_put_byte(b, (unsigned char)(0x80 | ((c >> 6) & 0x3F)));
        bb_put_byte(b, (unsigned char)(0x80 | (c & 0x3F)));
    }
}

typedef struct {
    cp_t *chars;
    size_t len, cap;
} Line;

static void line_ensure(Line *ln, size_t more) {
    if (ln->len + more <= ln->cap) return;
    size_t newcap = ln->cap ? ln->cap * 2 : 16;
    while (newcap < ln->len + more) newcap *= 2;
    ln->chars = realloc(ln->chars, newcap * sizeof(cp_t));
    if (!ln->chars) { perror("realloc"); exit(1); }
    ln->cap = newcap;
}

static void line_insert_cp(Line *ln, size_t idx, cp_t cp) {
    line_ensure(ln, 1);
    if (idx > ln->len) idx = ln->len;
    memmove(&ln->chars[idx + 1], &ln->chars[idx], (ln->len - idx) * sizeof(cp_t));
    ln->chars[idx] = cp;
    ln->len++;
}

static void line_delete_cp(Line *ln, size_t idx) {
    if (idx >= ln->len) return;
    memmove(&ln->chars[idx], &ln->chars[idx + 1], (ln->len - idx - 1) * sizeof(cp_t));
    ln->len--;
}

typedef struct {
    Line *lines;
    size_t numlines, linecap;
    size_t cy, cx;
    size_t rowoff;
    int coloff;
    int screenrows, screencols;
    int textrows;
    int gutter_w;
    char *filename;
    bool dirty;
    bool had_trailing_newline;
    char statusmsg[160];
    time_t statusmsg_time;
    int pending_digraph;
    int pend_c1;
    bool show_legend;
    int quit_confirm;
    struct termios orig_termios;
    bool raw_enabled;
} EditorState;

static EditorState E;
static volatile sig_atomic_t g_resized = 0;

static void lines_ensure(size_t more) {
    if (E.numlines + more <= E.linecap) return;
    size_t newcap = E.linecap ? E.linecap * 2 : 16;
    while (newcap < E.numlines + more) newcap *= 2;
    E.lines = realloc(E.lines, newcap * sizeof(Line));
    if (!E.lines) { perror("realloc"); exit(1); }
    E.linecap = newcap;
}

static void lines_insert_blank(size_t at) {
    lines_ensure(1);
    if (at > E.numlines) at = E.numlines;
    memmove(&E.lines[at + 1], &E.lines[at], (E.numlines - at) * sizeof(Line));
    E.lines[at].chars = NULL;
    E.lines[at].len = 0;
    E.lines[at].cap = 0;
    E.numlines++;
}

static void lines_delete(size_t at) {
    if (at >= E.numlines) return;
    free(E.lines[at].chars);
    memmove(&E.lines[at], &E.lines[at + 1], (E.numlines - at - 1) * sizeof(Line));
    E.numlines--;
}

static void set_status(const char *fmt, ...);

static void add_line_from_range(const cp_t *cps, size_t start, size_t end) {
    if (end > start && cps[end - 1] == 0x0D) end--;
    lines_insert_blank(E.numlines);
    Line *ln = &E.lines[E.numlines - 1];
    for (size_t k = start; k < end; k++) line_insert_cp(ln, ln->len, cps[k]);
}

static void buffer_from_codepoints(const cp_t *cps, size_t n, bool *had_trailing_nl) {
    E.numlines = 0;
    size_t seg_start = 0;
    for (size_t i = 0; i < n; i++) {
        if (cps[i] == 0x0A) {
            add_line_from_range(cps, seg_start, i);
            seg_start = i + 1;
        }
    }
    if (seg_start < n || E.numlines == 0)
        add_line_from_range(cps, seg_start, n);
    *had_trailing_nl = (n > 0 && cps[n - 1] == 0x0A);
}

static bool load_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        if (errno == ENOENT) {
            lines_insert_blank(0);
            E.had_trailing_newline = true;
            set_status("(new file)");
            return true;
        }
        return false;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0) { fclose(f); return false; }
    unsigned char *raw = malloc((size_t)sz ? (size_t)sz : 1);
    if ((size_t)sz > 0 && fread(raw, 1, (size_t)sz, f) != (size_t)sz) {
        free(raw); fclose(f); return false;
    }
    fclose(f);

    size_t n;
    cp_t *cps = utf8_decode_buf(raw, (size_t)sz, &n);
    free(raw);

    buffer_from_codepoints(cps, n, &E.had_trailing_newline);
    free(cps);
    return true;
}

static bool save_file_to(const char *path) {
    ByteBuf bb = {0};
    for (size_t i = 0; i < E.numlines; i++) {
        Line *ln = &E.lines[i];
        for (size_t k = 0; k < ln->len; k++) utf8_encode_append(&bb, ln->chars[k]);
        if (i + 1 < E.numlines) bb_put_byte(&bb, 0x0A);
    }
    if (E.numlines > 0 && E.had_trailing_newline) bb_put_byte(&bb, 0x0A);

    FILE *f = fopen(path, "wb");
    if (!f) { free(bb.data); return false; }
    size_t written = fwrite(bb.data, 1, bb.len, f);
    fclose(f);
    bool ok = (written == bb.len);
    free(bb.data);
    return ok;
}

typedef struct {
    bool is_token;
    int width;
    char label[3];
    Category cat;
} RenderInfo;

static RenderInfo classify_cp(cp_t cp) {
    RenderInfo r = {0};
    const SpecEntry *s = spec_by_cp(cp);
    if (s) {
        r.is_token = true; r.width = 2; r.cat = s->cat;
        r.label[0] = s->label[0]; r.label[1] = s->label[1]; r.label[2] = 0;
        return r;
    }
    if (cp == 0x09) {
        r.is_token = true; r.width = 2; r.cat = CAT_DIM;
        r.label[0] = '-'; r.label[1] = '>';
        return r;
    }
    if (cp == 0x0D) {
        r.is_token = true; r.width = 2; r.cat = CAT_DIM;
        r.label[0] = 'C'; r.label[1] = 'R';
        return r;
    }
    if (cp < 0x20 || cp == 0x7F) {
        r.is_token = true; r.width = 2; r.cat = CAT_DIM;
        r.label[0] = '^';
        r.label[1] = (cp == 0x7F) ? '?' : (char)('@' + cp);
        return r;
    }
    const char *cname;
    if (is_confusable(cp, &cname)) {
        r.is_token = true; r.width = 2; r.cat = CAT_SUSPECT;
        r.label[0] = '?'; r.label[1] = '?';
        return r;
    }
    r.is_token = false;
    r.width = 1; /* this is probably right :sob: */
    return r;
}

static void disable_raw_mode(void) {
    if (E.raw_enabled) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios);
        /* leave alternate screen, restore cursor */
        ssize_t _w = write(STDOUT_FILENO, "\x1b[?1049l\x1b[?25h", 14); (void)_w;
    }
}

static void die(const char *s) {
    disable_raw_mode();
    perror(s);
    exit(1);
}

static void enable_raw_mode(void) {
    if (!isatty(STDIN_FILENO)) {
        fprintf(stderr, "ink-edit: stdin is not a terminal\n");
        exit(1);
    }
    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
    atexit(disable_raw_mode);

    struct termios raw = E.orig_termios;
    raw.c_iflag &= (tcflag_t)~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= (tcflag_t)~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= (tcflag_t)~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
    E.raw_enabled = true;

    ssize_t _w2 = write(STDOUT_FILENO, "\x1b[?1049h", 8); (void)_w2;
}

static void handle_sigwinch(int sig) { (void)sig; g_resized = 1; }

static void get_window_size(void) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        E.screenrows = 24; E.screencols = 80;
    } else {
        E.screenrows = ws.ws_row;
        E.screencols = ws.ws_col;
    }
    E.textrows = E.screenrows - 2;
    if (E.textrows < 1) E.textrows = 1;
    int digits = 2;
    size_t n = E.numlines ? E.numlines : 1;
    while (n >= 10) { n /= 10; digits++; }
    if (digits < 3) digits = 3;
    E.gutter_w = digits + 1;
}

enum {
    KEY_ARROW_LEFT = 0x110000,
    KEY_ARROW_RIGHT,
    KEY_ARROW_UP,
    KEY_ARROW_DOWN,
    KEY_HOME,
    KEY_END,
    KEY_PAGE_UP,
    KEY_PAGE_DOWN,
    KEY_DEL,
    KEY_BACKSPACE,
    KEY_ENTER,
    KEY_ESC,
    KEY_CTRL_S,
    KEY_CTRL_Q,
    KEY_CTRL_K,
    KEY_CTRL_G,
    KEY_NONE,
};

static int read_byte_blocking(void) {
    unsigned char c;
    ssize_t n;
    while ((n = read(STDIN_FILENO, &c, 1)) != 1) {
        if (n == -1 && errno != EAGAIN && errno != EINTR) die("read");
        if (g_resized) return -1;
    }
    return c;
}

static int read_byte_timeout(int ms) {
    fd_set fds;
    struct timeval tv;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    tv.tv_sec = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;
    int r = select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
    if (r <= 0) return -1;
    unsigned char c;
    if (read(STDIN_FILENO, &c, 1) != 1) return -1;
    return c;
}

static int decode_utf8_continuation(int b0) {
    uint32_t cp; int extra;
    if ((b0 & 0xE0) == 0xC0)      { cp = (uint32_t)(b0 & 0x1F); extra = 1; }
    else if ((b0 & 0xF0) == 0xE0) { cp = (uint32_t)(b0 & 0x0F); extra = 2; }
    else if ((b0 & 0xF8) == 0xF0) { cp = (uint32_t)(b0 & 0x07); extra = 3; }
    else return 0xFFFD;
    for (int k = 0; k < extra; k++) {
        int bk = read_byte_blocking();
        if (bk < 0 || (bk & 0xC0) != 0x80) return 0xFFFD;
        cp = (cp << 6) | (uint32_t)(bk & 0x3F);
    }
    return (int)cp;
}

static int parse_escape_sequence(void) {
    int b1 = read_byte_timeout(50);
    if (b1 < 0) return KEY_ESC;

    if (b1 == '[' || b1 == 'O') {
        int b2 = read_byte_timeout(50);
        if (b2 < 0) return KEY_ESC;
        if (b2 >= '0' && b2 <= '9') {
            int b3 = read_byte_timeout(50);
            if (b3 == '~') {
                switch (b2) {
                    case '1': case '7': return KEY_HOME;
                    case '3': return KEY_DEL;
                    case '4': case '8': return KEY_END;
                    case '5': return KEY_PAGE_UP;
                    case '6': return KEY_PAGE_DOWN;
                    default: return KEY_ESC;
                }
            }
            return KEY_ESC;
        }
        switch (b2) {
            case 'A': return KEY_ARROW_UP;
            case 'B': return KEY_ARROW_DOWN;
            case 'C': return KEY_ARROW_RIGHT;
            case 'D': return KEY_ARROW_LEFT;
            case 'H': return KEY_HOME;
            case 'F': return KEY_END;
            default:  return KEY_ESC;
        }
    }
    return KEY_ESC;
}

static int read_key(void) {
    int b0 = read_byte_blocking();
    if (b0 < 0) return KEY_NONE;

    if (b0 == 0x1B) return parse_escape_sequence();
    if (b0 == 0x0D || b0 == 0x0A) return KEY_ENTER;
    if (b0 == 0x7F || b0 == 0x08) return KEY_BACKSPACE;
    if (b0 == 0x13) return KEY_CTRL_S;
    if (b0 == 0x11) return KEY_CTRL_Q;
    if (b0 == 0x0B) return KEY_CTRL_K;
    if (b0 == 0x07) return KEY_CTRL_G;
    if (b0 == 0x03) return KEY_ESC;
    if (b0 < 0x80) return b0;
    return decode_utf8_continuation(b0);
}

static void editor_insert_cp(cp_t cp) {
    line_insert_cp(&E.lines[E.cy], E.cx, cp);
    E.cx++;
    E.dirty = true;
}

static void editor_insert_newline(void) {
    Line *cur = &E.lines[E.cy];
    lines_insert_blank(E.cy + 1);
    cur = &E.lines[E.cy];
    Line *next = &E.lines[E.cy + 1];
    size_t tail = cur->len - E.cx;
    if (tail > 0) {
        line_ensure(next, tail);
        memcpy(next->chars, &cur->chars[E.cx], tail * sizeof(cp_t));
        next->len = tail;
        cur->len = E.cx;
    }
    E.cy++;
    E.cx = 0;
    E.dirty = true;
}

static void editor_backspace(void) {
    if (E.cx > 0) {
        line_delete_cp(&E.lines[E.cy], E.cx - 1);
        E.cx--;
        E.dirty = true;
    } else if (E.cy > 0) {
        Line *prev = &E.lines[E.cy - 1];
        Line *cur = &E.lines[E.cy];
        size_t newcx = prev->len;
        if (cur->len > 0) {
            line_ensure(prev, cur->len);
            memcpy(&prev->chars[prev->len], cur->chars, cur->len * sizeof(cp_t));
            prev->len += cur->len;
        }
        lines_delete(E.cy);
        E.cy--;
        E.cx = newcx;
        E.dirty = true;
    }
}

static void editor_delete_forward(void) {
    Line *cur = &E.lines[E.cy];
    if (E.cx < cur->len) {
        line_delete_cp(cur, E.cx);
        E.dirty = true;
    } else if (E.cy + 1 < E.numlines) {
        Line *next = &E.lines[E.cy + 1];
        if (next->len > 0) {
            line_ensure(cur, next->len);
            memcpy(&cur->chars[cur->len], next->chars, next->len * sizeof(cp_t));
            cur->len += next->len;
        }
        lines_delete(E.cy + 1);
        E.dirty = true;
    }
}

static void move_cursor(int key) {
    Line *ln = &E.lines[E.cy];
    switch (key) {
        case KEY_ARROW_LEFT:
            if (E.cx > 0) E.cx--;
            else if (E.cy > 0) { E.cy--; E.cx = E.lines[E.cy].len; }
            break;
        case KEY_ARROW_RIGHT:
            if (E.cx < ln->len) E.cx++;
            else if (E.cy + 1 < E.numlines) { E.cy++; E.cx = 0; }
            break;
        case KEY_ARROW_UP:
            if (E.cy > 0) E.cy--;
            break;
        case KEY_ARROW_DOWN:
            if (E.cy + 1 < E.numlines) E.cy++;
            break;
        case KEY_HOME:
            E.cx = 0;
            break;
        case KEY_END:
            E.cx = E.lines[E.cy].len;
            break;
    }
    ln = &E.lines[E.cy];
    if (E.cx > ln->len) E.cx = ln->len;
}

typedef struct { char *b; size_t len, cap; } AppendBuf;

static void ab_append(AppendBuf *ab, const char *s, size_t n) {
    if (ab->len + n > ab->cap) {
        size_t newcap = ab->cap ? ab->cap * 2 : 4096;
        while (newcap < ab->len + n) newcap *= 2;
        ab->b = realloc(ab->b, newcap);
        if (!ab->b) { perror("realloc"); exit(1); }
        ab->cap = newcap;
    }
    memcpy(ab->b + ab->len, s, n);
    ab->len += n;
}
static void ab_str(AppendBuf *ab, const char *s) { ab_append(ab, s, strlen(s)); }
static void ab_free(AppendBuf *ab) { free(ab->b); }

static const char *cat_sgr_on(Category cat) {
    switch (cat) {
        case CAT_STACK:   return "\x1b[30;46m"; /* black on cyan */
        case CAT_LITERAL: return "\x1b[30;43m"; /* black on yellow */
        case CAT_IO:      return "\x1b[30;42m"; /* black on green */
        case CAT_ARITH:   return "\x1b[30;45m"; /* black on magenta */
        case CAT_BLOCK:   return "\x1b[97;44m"; /* white on blue */
        case CAT_STRUCT:  return "\x1b[97;41m"; /* white on red */
        case CAT_DIM:     return "\x1b[2;90m";  /* dim grey, no bg */
        case CAT_SUSPECT: return "\x1b[1;100;93m"; /* bold bright-yellow on grey bg */
    }
    return "\x1b[0m";
}
#define SGR_OFF "\x1b[0m"

static void render_line(AppendBuf *ab, const Line *ln, int coloff, int avail,
                         long want_cx, int *cursor_col_out) {
    int col = 0;
    int emitted = 0;
    if (cursor_col_out) *cursor_col_out = -1;

    for (size_t i = 0; i <= ln->len; i++) {
        if ((long)i == want_cx && cursor_col_out)
            *cursor_col_out = col - coloff;

        if (i == ln->len) break;

        RenderInfo r = classify_cp(ln->chars[i]);
        int w = r.width;

        if (col + w <= coloff) { col += w; continue; }
        if (col >= coloff + avail) break;
        if (emitted + w > avail) break;

        if (r.is_token) {
            ab_str(ab, cat_sgr_on(r.cat));
            ab_append(ab, r.label, 2);
            ab_str(ab, SGR_OFF);
        } else {
            ByteBuf tmp = {0};
            utf8_encode_append(&tmp, ln->chars[i]);
            ab_append(ab, (char *)tmp.data, tmp.len);
            free(tmp.data);
        }
        col += w;
        emitted += w;
    }
}

static int column_for_cx(const Line *ln, size_t cx) {
    int col = 0;
    for (size_t i = 0; i < cx && i < ln->len; i++) {
        RenderInfo r = classify_cp(ln->chars[i]);
        col += r.width;
    }
    return col;
}

static void editor_scroll(void) {
    if (E.cy < E.rowoff) E.rowoff = E.cy;
    if (E.cy >= E.rowoff + (size_t)E.textrows) E.rowoff = E.cy - (size_t)E.textrows + 1;

    int avail = E.screencols - E.gutter_w;
    if (avail < 1) avail = 1;
    int curcol = column_for_cx(&E.lines[E.cy], E.cx);
    if (curcol < E.coloff) E.coloff = curcol;
    if (curcol >= E.coloff + avail) E.coloff = curcol - avail + 1;
    if (E.coloff < 0) E.coloff = 0;
}

static const char *cp_info_string(cp_t cp, char *buf, size_t bufsz) {
    const SpecEntry *s = spec_by_cp(cp);
    if (s) {
        snprintf(buf, bufsz, "U+%04X %s -- %s", cp, s->name, s->op);
        return buf;
    }
    const char *cname;
    if (is_confusable(cp, &cname)) {
        snprintf(buf, bufsz, "U+%04X %s -- not spec, but easy to lose track of", cp, cname);
        return buf;
    }
    if (cp == 0x09) { snprintf(buf, bufsz, "U+0009 TAB -- ordinary cover text"); return buf; }
    if (cp < 0x20 || cp == 0x7F) {
        snprintf(buf, bufsz, "U+%04X control byte -- ordinary cover text", cp);
        return buf;
    }
    if (cp < 0x80) snprintf(buf, bufsz, "'%c' -- ordinary cover text", (char)cp);
    else snprintf(buf, bufsz, "U+%04X -- ordinary cover text", cp);
    return buf;
}

static void draw_legend(AppendBuf *ab) {
    ab_str(ab, "\x1b[2J\x1b[H");
    ab_str(ab, "Invisible Ink glyph legend -- Ctrl-K then the 2-letter code inserts it\r\n\r\n");
    Category last_cat = 0;
    static const char *cat_titles[] = {
        NULL,
        "stack ops", "literals", "I/O", "arithmetic (arity 1/2/3)",
        "blocks / if / call", "document structure"
    };
    for (size_t i = 0; i < SPEC_COUNT; i++) {
        const SpecEntry *s = &SPEC_TABLE[i];
        if (s->cat != last_cat) {
            last_cat = s->cat;
            char hdr[64];
            snprintf(hdr, sizeof(hdr), "\r\n  %s\r\n", cat_titles[s->cat]);
            ab_str(ab, hdr);
        }
        char line[160];
        ab_str(ab, cat_sgr_on(s->cat));
        ab_str(ab, "  ");
        ab_append(ab, s->label, 2);
        ab_str(ab, "  ");
        ab_str(ab, SGR_OFF);
        snprintf(line, sizeof(line), "  U+%04X  %-28s %s\r\n", s->cp, s->name, s->op);
        ab_str(ab, line);
    }
    ab_str(ab, "\r\n  also flagged (not spec, but easy to mistake for it or lose track of):\r\n");
    for (size_t i = 0; i < CONFUSABLE_COUNT; i++) {
        char line[128];
        ab_str(ab, cat_sgr_on(CAT_SUSPECT));
        ab_str(ab, "  ??  ");
        ab_str(ab, SGR_OFF);
        if (CONFUSABLES[i].lo == CONFUSABLES[i].hi)
            snprintf(line, sizeof(line), "  U+%04X       %s\r\n", CONFUSABLES[i].lo, CONFUSABLES[i].name);
        else
            snprintf(line, sizeof(line), "  U+%04X-%04X  %s\r\n", CONFUSABLES[i].lo, CONFUSABLES[i].hi, CONFUSABLES[i].name);
        ab_str(ab, line);
    }
    ab_str(ab, "\r\n  (press any key to go back)\r\n");
}

static void refresh_screen(void) {
    if (g_resized) { get_window_size(); g_resized = 0; }
    editor_scroll();

    AppendBuf ab = {0};
    ab_str(&ab, "\x1b[?25l\x1b[H");

    if (E.show_legend) {
        draw_legend(&ab);
        ssize_t _w3 = write(STDOUT_FILENO, ab.b, ab.len); (void)_w3;
        ab_free(&ab);
        return;
    }

    int cursor_row = -1, cursor_col = -1;
    for (int y = 0; y < E.textrows; y++) {
        size_t filerow = E.rowoff + (size_t)y;
        ab_str(&ab, "\x1b[K");
        if (filerow < E.numlines) {
            char gutter[32];
            snprintf(gutter, sizeof(gutter), "\x1b[2;90m%*zu \x1b[0m", E.gutter_w - 1, filerow + 1);
            ab_str(&ab, gutter);
            int avail = E.screencols - E.gutter_w;
            if (avail < 1) avail = 1;
            int cc = -1;
            long want = (filerow == E.cy) ? (long)E.cx : -1;
            render_line(&ab, &E.lines[filerow], E.coloff, avail, want, &cc);
            if (filerow == E.cy) {
                cursor_row = y;
                cursor_col = E.gutter_w + (cc < 0 ? 0 : cc);
            }
        } else {
            ab_str(&ab, "\x1b[2;90m~\x1b[0m");
        }
        ab_str(&ab, "\r\n");
    }

    {
        char left[160], right[96], full[256];
        const SpecEntry *s = NULL;
        cp_t under = 0;
        bool have_under = (E.lines[E.cy].len > 0 && E.cx < E.lines[E.cy].len);
        if (have_under) { under = E.lines[E.cy].chars[E.cx]; s = spec_by_cp(under); }
        (void)s;
        snprintf(left, sizeof(left), " %s%s",
                 E.filename ? E.filename : "[No Name]",
                 E.dirty ? " [modified]" : "");
        snprintf(right, sizeof(right), "Ln %zu, Col %zu ",
                 E.cy + 1, E.cx + 1);
        int leftlen = (int)strlen(left), rightlen = (int)strlen(right);
        ab_str(&ab, "\x1b[7m");
        ab_str(&ab, left);
        int pad = E.screencols - leftlen - rightlen;
        if (pad < 1) pad = 1;
        for (int i = 0; i < pad; i++) ab_str(&ab, " ");
        ab_str(&ab, right);
        ab_str(&ab, "\x1b[0m\r\n");
        (void)full;
    }

    ab_str(&ab, "\x1b[K");
    if (E.statusmsg[0] && time(NULL) - E.statusmsg_time < 5) {
        ab_str(&ab, E.statusmsg);
    } else if (E.pending_digraph == 1) {
        ab_str(&ab, "Ctrl-K: type a 2-letter glyph code (Ctrl-G for the list, Esc to cancel)");
    } else if (E.pending_digraph == 2) {
        char m[64];
        snprintf(m, sizeof(m), "Ctrl-K %c: type the second letter", E.pend_c1);
        ab_str(&ab, m);
    } else if (E.lines[E.cy].len > 0 && E.cx < E.lines[E.cy].len) {
        char info[160];
        cp_info_string(E.lines[E.cy].chars[E.cx], info, sizeof(info));
        ab_str(&ab, "at cursor: ");
        ab_str(&ab, info);
    } else {
        ab_str(&ab, "Ctrl-S save  Ctrl-Q quit  Ctrl-K insert glyph  Ctrl-G legend");
    }

    if (cursor_row >= 0) {
        char buf[32];
        snprintf(buf, sizeof(buf), "\x1b[%d;%dH", cursor_row + 1, cursor_col + 1);
        ab_str(&ab, buf);
    } else {
        ab_str(&ab, "\x1b[H");
    }
    ab_str(&ab, "\x1b[?25h");

    ssize_t _w4 = write(STDOUT_FILENO, ab.b, ab.len); (void)_w4;
    ab_free(&ab);
}

static void set_status(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
    va_end(ap);
    E.statusmsg_time = time(NULL);
}

static char *prompt_line(const char *msg) {
    char buf[256] = {0};
    size_t len = 0;
    while (true) {
        set_status("%s%s", msg, buf);
        refresh_screen();
        int k = read_key();
        if (k == KEY_ESC) { set_status(""); return NULL; }
        if (k == KEY_ENTER) {
            if (len == 0) continue;
            set_status("");
            return strdup(buf);
        }
        if (k == KEY_BACKSPACE) {
            if (len > 0) buf[--len] = 0;
            continue;
        }
        if (k >= 0x20 && k < 0x7F && len < sizeof(buf) - 1) {
            buf[len++] = (char)k;
            buf[len] = 0;
        }
    }
}

static void do_save(void) {
    if (!E.filename) {
        char *name = prompt_line("Save as: ");
        if (!name) { set_status("Save cancelled"); return; }
        E.filename = name;
    }
    if (save_file_to(E.filename)) {
        E.dirty = false;
        set_status("Saved \"%s\"", E.filename);
    } else {
        set_status("Could not save: %s", strerror(errno));
    }
}

static void handle_digraph_char(int k) {
    if (k == KEY_ESC || k >= 0x110000) {
        E.pending_digraph = 0;
        set_status("");
        return;
    }
    if (E.pending_digraph == 1) {
        E.pend_c1 = k;
        E.pending_digraph = 2;
        return;
    }

    const SpecEntry *s = spec_by_mnemonic(E.pend_c1, k);
    if (s) {
        editor_insert_cp(s->cp);
        set_status("inserted %s (U+%04X, %s)", s->name, s->cp, s->op);
    } else {
        editor_insert_cp((cp_t)E.pend_c1);
        editor_insert_cp((cp_t)k);
        set_status("no glyph \"%c%c\" -- inserted literally", E.pend_c1, k);
    }
    E.pending_digraph = 0;
}

static void process_keypress(void) {
    int k = read_key();
    if (k == KEY_NONE) return;

    if (E.show_legend) { E.show_legend = false; return; }

    if (E.pending_digraph) { handle_digraph_char(k); return; }

    switch (k) {
        case KEY_CTRL_Q:
            if (E.dirty && E.quit_confirm < 1) {
                set_status("Unsaved changes -- press Ctrl-Q again to quit without saving");
                E.quit_confirm++;
                return;
            }
            disable_raw_mode();
            exit(0);
            return;

        case KEY_CTRL_S:
            do_save();
            break;

        case KEY_CTRL_K:
            E.pending_digraph = 1;
            break;

        case KEY_CTRL_G:
            E.show_legend = true;
            break;

        case KEY_ENTER:
            editor_insert_newline();
            break;

        case KEY_BACKSPACE:
            editor_backspace();
            break;

        case KEY_DEL:
            editor_delete_forward();
            break;

        case KEY_ARROW_LEFT: case KEY_ARROW_RIGHT:
        case KEY_ARROW_UP:   case KEY_ARROW_DOWN:
        case KEY_HOME:       case KEY_END:
            move_cursor(k);
            break;

        case KEY_PAGE_UP:
        case KEY_PAGE_DOWN: {
            if (k == KEY_PAGE_UP) E.cy = (E.cy > (size_t)E.textrows) ? E.cy - (size_t)E.textrows : 0;
            else E.cy = (E.cy + (size_t)E.textrows < E.numlines) ? E.cy + (size_t)E.textrows : E.numlines - 1;
            if (E.cx > E.lines[E.cy].len) E.cx = E.lines[E.cy].len;
            break;
        }

        case KEY_ESC:
            break; /* no-op */

        default:
            if (k >= 0 && k <= 0x10FFFF) editor_insert_cp((cp_t)k);
            break;
    }

    if (k != KEY_CTRL_Q) E.quit_confirm = 0;
}

static int selftest_failures = 0;
#define CHECK(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL: %s\n", msg); selftest_failures++; } \
    else fprintf(stderr, "ok:   %s\n", msg); \
} while (0)

static void run_selftest(void) {
    CHECK(SPEC_COUNT == 26, "spec table has all 26 codepoints");
    bool all_roundtrip = true;
    for (size_t i = 0; i < SPEC_COUNT; i++) {
        const SpecEntry *s = &SPEC_TABLE[i];
        const SpecEntry *back = spec_by_mnemonic(s->label[0], s->label[1]);
        if (back != s) all_roundtrip = false;
        if (spec_by_cp(s->cp) != s) all_roundtrip = false;
    }
    CHECK(all_roundtrip, "every spec entry round-trips cp <-> mnemonic");

    bool labels_unique = true;
    for (size_t i = 0; i < SPEC_COUNT; i++)
        for (size_t j = i + 1; j < SPEC_COUNT; j++)
            if (SPEC_TABLE[i].label[0] == SPEC_TABLE[j].label[0] &&
                SPEC_TABLE[i].label[1] == SPEC_TABLE[j].label[1])
                labels_unique = false;
    CHECK(labels_unique, "all digraph mnemonics are unique");

    bool all_tokens = true;
    for (size_t i = 0; i < SPEC_COUNT; i++) {
        RenderInfo r = classify_cp(SPEC_TABLE[i].cp);
        if (!r.is_token || r.width != 2) all_tokens = false;
    }
    CHECK(all_tokens, "every spec codepoint renders as a visible token");

    RenderInfo rnorm = classify_cp('A');
    CHECK(!rnorm.is_token && rnorm.width == 1, "plain ASCII renders as itself");

    RenderInfo rsoft = classify_cp(0x00AD); /* SOFT HYPHEN */
    CHECK(rsoft.is_token && rsoft.cat == CAT_LITERAL, "soft hyphen is a spec literal terminator, not confusable");

    cp_t sample[] = { 'h', 'i', ' ', 0x00A0, 0x2029, 0x1F600, 0x3000 };
    ByteBuf bb = {0};
    for (size_t i = 0; i < sizeof(sample)/sizeof(sample[0]); i++)
        utf8_encode_append(&bb, sample[i]);
    size_t n;
    cp_t *decoded = utf8_decode_buf(bb.data, bb.len, &n);
    bool codec_ok = (n == sizeof(sample)/sizeof(sample[0]));
    if (codec_ok)
        for (size_t i = 0; i < n; i++) if (decoded[i] != sample[i]) codec_ok = false;
    CHECK(codec_ok, "UTF-8 encode/decode round-trips ASCII + spec glyphs + astral codepoint");
    free(bb.data); free(decoded);

    {
        cp_t withnl[] = { 'a','b','c','\n','d','e','f','\n' };
        bool trailing;
        memset(&E, 0, sizeof(E));
        buffer_from_codepoints(withnl, 8, &trailing);
        CHECK(E.numlines == 2 && trailing, "split with trailing newline: 2 lines, flag set");
        for (size_t i = 0; i < E.numlines; i++) free(E.lines[i].chars);
        free(E.lines);

        cp_t nonl[] = { 'a','b','c','\n','d','e','f' };
        memset(&E, 0, sizeof(E));
        buffer_from_codepoints(nonl, 7, &trailing);
        CHECK(E.numlines == 2 && !trailing, "split without trailing newline: 2 lines, flag clear");
        for (size_t i = 0; i < E.numlines; i++) free(E.lines[i].chars);
        free(E.lines);
    }

    {
        const char *path = "/tmp/ink-edit-selftest.txt";
        ByteBuf src = {0};
        const char *cover = "call swap: ";
        for (const char *p = cover; *p; p++) bb_put_byte(&src, (unsigned char)*p);
        utf8_encode_append(&src, 0x00A0);
        utf8_encode_append(&src, 0x2061);
        bb_put_byte(&src, '\n');
        utf8_encode_append(&src, 0x200B);

        FILE *f = fopen(path, "wb");
        fwrite(src.data, 1, src.len, f);
        fclose(f);

        memset(&E, 0, sizeof(E));
        bool loaded = load_file(path);
        CHECK(loaded, "selftest file loads");

        bool saved = save_file_to("/tmp/ink-edit-selftest-out.txt");
        CHECK(saved, "selftest file saves");

        FILE *f2 = fopen("/tmp/ink-edit-selftest-out.txt", "rb");
        fseek(f2, 0, SEEK_END);
        long sz2 = ftell(f2);
        fseek(f2, 0, SEEK_SET);
        unsigned char *out = malloc((size_t)sz2);
        size_t _rd = fread(out, 1, (size_t)sz2, f2); (void)_rd;
        fclose(f2);

        bool byte_exact = ((size_t)sz2 == src.len) && (memcmp(out, src.data, src.len) == 0);
        CHECK(byte_exact, "load -> save reproduces the file byte-for-byte (no normalization)");

        free(out);
        free(src.data);
        for (size_t i = 0; i < E.numlines; i++) free(E.lines[i].chars);
        free(E.lines);
        remove(path);
        remove("/tmp/ink-edit-selftest-out.txt");
    }

    fprintf(stderr, "\n%s\n", selftest_failures == 0 ? "all checks passed" : "SOME CHECKS FAILED");
}

int main(int argc, char **argv) {
    const char *path = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--selftest") == 0) {
            run_selftest();
            return selftest_failures == 0 ? 0 : 1;
        }
        path = argv[i];
    }

    memset(&E, 0, sizeof(E));
    E.had_trailing_newline = true;
    if (path) E.filename = strdup(path);

    signal(SIGWINCH, handle_sigwinch);

    if (path) {
        if (!load_file(path)) {
            fprintf(stderr, "ink-edit: %s: %s\n", path, strerror(errno));
            return 1;
        }
    } else {
        lines_insert_blank(0);
    }

    enable_raw_mode();
    get_window_size();
    set_status("Ctrl-S save  Ctrl-Q quit  Ctrl-K insert glyph  Ctrl-G legend");

    while (true) {
        refresh_screen();
        process_keypress();
    }
    return 0;
}

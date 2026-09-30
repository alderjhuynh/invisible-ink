#include "cli.h"
#include "ink.h"
#include "parse.h"
#include "vm.h"
#include "util.h"
#include "quill.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdarg.h>

/* disasm helpers for dump */
static void disasm_block(Block *blk, int indent) {
    for (size_t i = 0; i < blk->count; i++) {
        Instr *ins = &blk->items[i];
        printf("%*s%s", indent, "", op_name(ins->op));
        if (ins->op == OP_PUSH) printf(" %lld", (long long)ins->value);
        printf("\n");
        if (ins->op == OP_IF) {
            printf("%*sthen:\n", indent + 2, "");
            disasm_block(ins->then_blk, indent + 4);
            if (ins->else_blk) {
                printf("%*selse:\n", indent + 2, "");
                disasm_block(ins->else_blk, indent + 4);
            }
        }
    }
}

void print_global_usage(const char *prog){
    fprintf(stderr,
        "ink %s — Invisible Ink\n"
        "\n"
        "Usage:\n"
        "  %s <command> [options] [args]\n"
        "  %s run <file> [options]          (run program; legacy: %s <file>)\n"
        "\n"
        "Commands:\n"
        "  run          Run an Invisible Ink program\n"
        "  check        Parse and validate without executing\n"
        "  dump         Dump parsed AST (internal disassembler, \"word N:\" style)\n"
        "  disassemble  Disassemble Invisible Ink to Quill (aliases: disasm, decompile)\n"
        "  assemble     Assemble Quill to Invisible Ink (aliases: asm, compile, build)\n"
        "  embed        Compile Quill and embed/thread into cover prose\n"
        "  edit         Launch InkIDE visual editor (alias: ide)\n"
        "  help         Show help for a command\n"
        "  version      Show version\n"
        "\n"
        "Global options:\n"
        "  -h, --help       Show help\n"
        "  -V, --version    Show version\n"
        "\n"
        "Run '%s help <command>' for command-specific help.\n"
        INK_VERSION, prog, prog, prog, prog, prog);
}

void print_help_run(const char *prog){
    fprintf(stderr,
        "Usage: %s run <file> [options]\n"
        "  Run an Invisible Ink program.\n"
        "\n"
        "Arguments:\n"
        "  <file>                  UTF-8 file containing Invisible Ink (cover text allowed)\n"
        "\n"
        "Options:\n"
        "  --steps <N>             Override step limit (default %lu, 0 = unlimited)\n"
        "  --max-steps <N>         Same as --steps\n"
        "  --depth <N>             Override max call depth (default %d)\n"
        "  --max-depth <N>         Same as --depth\n"
        "  --stats                 Print execution stats to stderr after run\n"
        "  -h, --help              Show this help\n"
        "\n"
        "Legacy:\n"
        "  %s <file>               Equivalent to 'run <file>'\n"
        "  %s -d <file>            Equivalent to 'dump <file>'\n",
        prog, DEFAULT_MAX_STEPS, DEFAULT_MAX_DEPTH, prog, prog);
}
void print_help_dump(const char *prog){
    fprintf(stderr,
        "Usage: %s dump <file>\n"
        "       %s disassemble <file> [-o output]\n"
        "       %s disasm <file> [-o output]\n"
        "\n"
        "  dump: print internal AST (word N: / then:/else:) to stdout.\n"
        "  disassemble: decompile Invisible Ink to Quill assembly.\n"
        "\n"
        "Arguments:\n"
        "  <file>                  Input Invisible Ink file\n"
        "\n"
        "Options:\n"
        "  -o, --output <file>     Write to file instead of stdout (disassemble only)\n"
        "  -h, --help              Show this help\n",
        prog, prog, prog);
}
void print_help_assemble(const char *prog){
    fprintf(stderr,
        "Usage: %s assemble <input> [-o output]\n"
        "       %s asm <input> [-o output]\n"
        "       %s compile <input> [-o output]\n"
        "\n"
        "  Assemble Quill (.q) to Invisible Ink.\n"
        "\n"
        "Arguments:\n"
        "  <input>                 Quill source file\n"
        "\n"
        "Options:\n"
        "  -o, --output <file>     Write output to file (default: stdout)\n"
        "  -h, --help              Show this help\n",
        prog, prog, prog);
}
void print_help_edit(const char *prog){
    fprintf(stderr,
        "Usage: %s edit [file] [options]\n"
        "       %s ide [file] [options]\n"
        "       %s ed [file]\n"
        "\n"
        "  Launch InkIDE, the visual editor for invisible codepoints.\n"
        "\n"
        "Arguments:\n"
        "  [file]                  File to open (new buffer if omitted)\n"
        "\n"
        "Options:\n"
        "  --selftest              Run internal self-tests and exit\n"
        "  -h, --help              Show this help\n"
        "\n"
        "Editor keys: Ctrl-S save, Ctrl-R raw, Ctrl-Q quit, Ctrl-K glyph, Ctrl-G legend.\n",
        prog, prog, prog);
}
void print_help_check(const char *prog){
    fprintf(stderr,
        "Usage: %s check <file>\n"
        "  Parse and validate an Invisible Ink file without executing.\n"
        "  Also validates embedded FEFF programs.\n"
        "\n"
        "Options:\n"
        "  -h, --help              Show this help\n",
        prog);
}
void print_help_embed(const char *prog){
    fprintf(stderr,
        "Usage: %s embed <input.q> [output] [options]\n"
        "       %s embed <input.q> -o <output> [options]\n"
        "\n"
        "  Compile Quill (.q) to Invisible Ink and embed/thread into cover prose.\n"
        "  Without prose, writes raw invisible codepoints (like assemble).\n"
        "  With prose, interleaves invisible codepoints through the cover text\n"
        "  so they are hidden in the visible document but still executable.\n"
        "\n"
        "Arguments:\n"
        "  <input.q>                 Quill source file (.q)\n"
        "  [output]                  Output file (alternate to -o)\n"
        "\n"
        "Options:\n"
        "  -o, --output <file>       Write output to file (default: stdout)\n"
        "  -p, --prose <file>        Cover text / prose file to thread ink through\n"
        "      --text <string>       Cover text directly as string\n"
        "      --wrap                Wrap with FEFF/ZWSP instead of threading (insert as single block)\n"
        "      --interval <N>        Insert one ink codepoint every N prose codepoints (default: auto even distribution)\n"
        "  -h, --help                Show this help\n"
        "\n"
        "Examples:\n"
        "  %s embed prog.q -o hidden.txt\n"
        "  %s embed prog.q hidden.txt --prose cover.txt\n"
        "  %s embed prog.q -o hidden.txt --prose cover.txt\n"
        "  %s embed prog.q -o hidden.txt --text \"Hello world, this is cover.\"\n"
        "  %s embed prog.q -o hidden.txt --prose cover.txt --wrap\n",
        prog, prog, prog, prog, prog, prog, prog);
}

int do_run(const char *path, bool do_stats){
    g_src_name = path;
    size_t raw_len; unsigned char *raw = read_file_raw(path, &raw_len);
    if(!raw) return 1;
    size_t full_len; cp_t *full = utf8_decode(raw, raw_len, &full_len);
    free(raw);
    size_t prog_len; bool embedded; cp_t *prog = extract_program(full, full_len, &prog_len, &embedded);
    free(full);
    g_words = split_and_parse_words(prog, prog_len, &g_word_count);
    free(prog);
    if (g_word_count == 0 || prog_len == 0) {
        fprintf(stderr, "%s: this document does not contain Invisible Ink\n", path);
        return 1;
    }
    g_outcome = OUT_NONE;
    g_steps = 0;
    g_sp = 0; g_mem_cap = 0; free(g_stack); g_stack=NULL; g_cap=0; free(g_mem); g_mem=NULL;
    vm_reset_prng();

    struct timespec t0, t1;
    bool timed = do_stats && clock_gettime(CLOCK_MONOTONIC,&t0)==0;
    int exit_code = 0;
    if (setjmp(g_jmp) == 0) {
        exec_block(g_words[0], 0);
        fflush(stdout);
        exit_code = 0;
    } else {
        fflush(stdout);
        if (g_outcome == OUT_HALT) exit_code = 0;
        else { fprintf(stderr, "%s: runtime error: %s\n", path, g_errmsg); exit_code = 1; }
    }
    if(do_stats){
        if(timed) clock_gettime(CLOCK_MONOTONIC,&t1);
        double elapsed = timed ? (t1.tv_sec - t0.tv_sec)*1000.0 + (t1.tv_nsec - t0.tv_nsec)/1e6 : 0;
        fprintf(stderr, "[ink] steps=%lu words=%zu stack=%zu mem=%zu%s  elapsed=%.2fms\n",
            g_steps, g_word_count, g_sp, g_mem_cap, embedded? " embedded":"", elapsed);
    }
    for(size_t i=0;i<g_word_count;i++) block_free(g_words[i]);
    free(g_words);
    free(g_stack); g_stack=NULL; g_sp=g_cap=0;
    free(g_mem); g_mem=NULL; g_mem_cap=0;
    g_words=NULL; g_word_count=0;
    return exit_code;
}

int do_dump(const char *path){
    g_src_name = path;
    size_t raw_len; unsigned char *raw = read_file_raw(path, &raw_len);
    if(!raw) return 1;
    size_t full_len; cp_t *full = utf8_decode(raw, raw_len, &full_len);
    free(raw);
    size_t prog_len; bool embedded; cp_t *prog = extract_program(full, full_len, &prog_len, &embedded);
    free(full);
    (void)embedded;
    g_words = split_and_parse_words(prog, prog_len, &g_word_count);
    free(prog);
    if (g_word_count == 0 || prog_len == 0) {
        fprintf(stderr, "%s: this document does not contain Invisible Ink\n", path);
        return 1;
    }
    for (size_t w = 0; w < g_word_count; w++) {
        printf("word %zu:\n", w);
        disasm_block(g_words[w], 2);
    }
    for(size_t i=0;i<g_word_count;i++) block_free(g_words[i]);
    free(g_words); g_words=NULL; g_word_count=0;
    return 0;
}

int do_disassemble(const char *input, const char *output){
    size_t len; unsigned char *data = read_file_raw(input, &len);
    if(!data) return 1;
    char *qsrc = q_disassemble_bytes(data,len);
    free(data);
    if(!qsrc){ fprintf(stderr, "error: %s\n", q_errmsg); return 1; }
    if(output){
        FILE *f=fopen(output,"w"); if(!f){ fprintf(stderr,"error: %s: %s\n",output,strerror(errno)); free(qsrc); return 1; }
        fputs(qsrc,f); fclose(f);
    } else {
        fputs(qsrc,stdout);
    }
    free(qsrc);
    return 0;
}

int do_assemble(const char *input, const char *output){
    size_t len; unsigned char *data = read_file_raw(input, &len);
    if(!data) return 1;
    char *src = malloc(len+1); memcpy(src,data,len); src[len]=0; free(data);
    QCpVec prog;
    if(!q_assemble(src,len,&prog)){
        fprintf(stderr,"error: %s\n",q_errmsg);
        free(src);
        return 1;
    }
    free(src);
    size_t max_bytes=prog.count*4 + 4;
    unsigned char *outbuf=malloc(max_bytes);
    size_t outpos=0;
    for(size_t i=0;i<prog.count;i++){
        unsigned char tmp[4]; size_t el=utf8_encode_cp(prog.data[i],tmp);
        memcpy(outbuf+outpos,tmp,el); outpos+=el;
    }
    q_cv_free(&prog);
    if(output){
        FILE *f=fopen(output,"wb"); if(!f){ fprintf(stderr,"error: %s: %s\n",output,strerror(errno)); free(outbuf); return 1; }
        fwrite(outbuf,1,outpos,f); fclose(f);
    } else {
        fwrite(outbuf,1,outpos,stdout);
    }
    free(outbuf);
    return 0;
}

int do_check(const char *path){
    g_src_name = path;
    size_t raw_len; unsigned char *raw = read_file_raw(path, &raw_len);
    if(!raw) return 1;
    size_t full_len; cp_t *full = utf8_decode(raw, raw_len, &full_len);
    free(raw);
    size_t prog_len; bool embedded; cp_t *prog = extract_program(full, full_len, &prog_len, &embedded);
    free(full);
    if(prog_len==0){ fprintf(stderr,"%s: this document does not contain Invisible Ink\n",path); free(prog); return 1; }
    size_t wc; Block **words = split_and_parse_words(prog, prog_len, &wc);
    free(prog);
    fprintf(stderr,"%s: OK — %zu word(s), %s, %zu spec codepoints\n", path, wc, embedded?"embedded":"plain", prog_len);
    for(size_t i=0;i<wc;i++) block_free(words[i]);
    free(words);
    return 0;
}

int do_embed(const char *input, const char *output, const char *prose_file, const char *prose_text, bool wrap, long interval){
    if(!input){ fprintf(stderr,"embed: missing <input.q>\n"); return 2; }
    if(prose_file && prose_text){
        fprintf(stderr,"embed: --prose and --text are mutually exclusive\n");
        return 2;
    }
    if(interval!=0 && interval < 1){
        fprintf(stderr,"embed: --interval must be >= 1\n");
        return 2;
    }
    /* assemble input.q -> QCpVec */
    size_t q_len; unsigned char *q_data = read_file_raw(input, &q_len);
    if(!q_data) return 1;
    char *src = malloc(q_len+1); memcpy(src,q_data,q_len); src[q_len]=0; free(q_data);
    QCpVec prog;
    if(!q_assemble(src,q_len,&prog)){
        fprintf(stderr,"error: %s\n", q_errmsg);
        free(src);
        return 1;
    }
    free(src);

    /* load prose if any */
    cp_t *prose_cps = NULL;
    size_t prose_len = 0;
    unsigned char *prose_raw = NULL;
    size_t prose_raw_len = 0;
    bool prose_owned = false;
    if(prose_file){
        prose_raw = read_file_raw(prose_file, &prose_raw_len);
        if(!prose_raw){ q_cv_free(&prog); return 1; }
        prose_owned = true;
    } else if(prose_text){
        prose_raw_len = strlen(prose_text);
        prose_raw = (unsigned char*)prose_text; /* not owned */
        prose_owned = false;
    }
    if(prose_raw){
        prose_cps = utf8_decode(prose_raw, prose_raw_len, &prose_len);
        if(prose_owned) free(prose_raw);
    }

    /* build output cps */
    cp_t *out_cps = NULL;
    size_t out_len = 0;
    size_t out_cap = 0;
    #define OUT_PUSH(c) do { if(out_len==out_cap){ out_cap=out_cap?out_cap*2:8192; out_cps=realloc(out_cps,out_cap*sizeof(cp_t)); } out_cps[out_len++]=(c); } while(0)

    if(!prose_cps || prose_len==0){
        if(wrap){
            OUT_PUSH(CP_BOM);
            for(size_t i=0;i<prog.count;i++) OUT_PUSH(prog.data[i]);
            OUT_PUSH(CP_ZWSP);
        } else {
            for(size_t i=0;i<prog.count;i++) OUT_PUSH(prog.data[i]);
        }
    } else {
        if(wrap){
            size_t mid = prose_len/2;
            for(size_t i=0;i<mid;i++) OUT_PUSH(prose_cps[i]);
            OUT_PUSH(CP_BOM);
            for(size_t i=0;i<prog.count;i++) OUT_PUSH(prog.data[i]);
            OUT_PUSH(CP_ZWSP);
            for(size_t i=mid;i<prose_len;i++) OUT_PUSH(prose_cps[i]);
        } else {
            long eff_interval = interval;
            if(eff_interval==0){
                if(prose_len > prog.count && prog.count>0){
                    eff_interval = (long)(prose_len / prog.count);
                    if(eff_interval < 1) eff_interval = 1;
                } else {
                    eff_interval = 1;
                }
            }
            size_t pi=0, qi=0;
            while(pi < prose_len || qi < prog.count){
                for(long k=0;k<eff_interval && pi < prose_len;k++){
                    OUT_PUSH(prose_cps[pi++]);
                }
                if(qi < prog.count){
                    OUT_PUSH(prog.data[qi++]);
                } else if(pi < prose_len){
                    /* no more ink, flush remaining prose */
                    while(pi < prose_len) OUT_PUSH(prose_cps[pi++]);
                    break;
                }
                if(pi >= prose_len && qi < prog.count){
                    /* prose exhausted, append remaining ink */
                    while(qi < prog.count) OUT_PUSH(prog.data[qi++]);
                    break;
                }
            }
        }
        free(prose_cps);
    }
    #undef OUT_PUSH
    q_cv_free(&prog);

    /* encode and write */
    size_t max_bytes = out_len*4 + 4;
    unsigned char *outbuf = malloc(max_bytes);
    size_t outpos=0;
    for(size_t i=0;i<out_len;i++){
        unsigned char tmp[4]; size_t el=utf8_encode_cp(out_cps[i], tmp);
        memcpy(outbuf+outpos, tmp, el); outpos+=el;
    }
    free(out_cps);
    int rc=0;
    if(output){
        FILE *f=fopen(output,"wb");
        if(!f){ fprintf(stderr,"error: %s: %s\n", output, strerror(errno)); free(outbuf); return 1; }
        if(fwrite(outbuf,1,outpos,f)!=outpos){ fprintf(stderr,"error writing %s\n", output); rc=1; }
        fclose(f);
    } else {
        if(fwrite(outbuf,1,outpos,stdout)!=outpos) rc=1;
    }
    free(outbuf);
    return rc;
}

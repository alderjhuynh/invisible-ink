#include "quill.h"
#include "parse.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <errno.h>

char q_errmsg[1024];

static void q_set_error(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    vsnprintf(q_errmsg, sizeof(q_errmsg), fmt, ap);
    va_end(ap);
}

typedef struct { char **data; size_t count; size_t cap; } QTokenVec;
static void q_tv_init(QTokenVec *v){ v->data=NULL; v->count=0; v->cap=0; }
static void q_tv_push(QTokenVec *v, char *s){
    if(v->count==v->cap){ v->cap=v->cap?v->cap*2:64; v->data=realloc(v->data, v->cap*sizeof(char*)); }
    v->data[v->count++]=s;
}
static void q_tv_free(QTokenVec *v){ for(size_t i=0;i<v->count;i++) free(v->data[i]); free(v->data); v->data=NULL; v->count=v->cap=0; }

void q_cv_init(QCpVec *v){ v->data=NULL; v->count=0; v->cap=0; }
void q_cv_push(QCpVec *v, cp_t c){ if(v->count==v->cap){ v->cap=v->cap?v->cap*2:64; v->data=realloc(v->data, v->cap*sizeof(cp_t)); } v->data[v->count++]=c; }
void q_cv_free(QCpVec *v){ free(v->data); v->data=NULL; v->count=v->cap=0; }

static bool q_is_integer_token(const char *tok){
    if(!tok||!*tok) return false;
    size_t i=0;
    if(tok[0]=='-'){ if(!tok[1]) return false; i=1; }
    for(;tok[i];i++) if(!isdigit((unsigned char)tok[i])) return false;
    return true;
}
static bool q_is_identifier(const char *tok){
    if(!tok||!*tok) return false;
    if(!(isalpha((unsigned char)tok[0])||tok[0]=='_')) return false;
    for(size_t i=1;tok[i];i++){ char c=tok[i]; if(!(isalnum((unsigned char)c)||c=='_'||c=='-')) return false; }
    return true;
}
static const char *Q_KEYWORDS[] = {"word","data","if","else","while","let","call","halt","push","dup","swap","drop","rot","neg","sub","mod","id","add","div","sign","mul","pow","not","and","or","xor","shl","shr","num-out","char-out","char-in","rand","sleep","store","load","sock-create","sock-bind","sock-listen","sock-accept","sock-recv","sock-send","sock-close","close","file-open","file-read","file-stat","file-close","dir-list","file-create","file-write","file-append","sys",NULL};
static bool q_is_keyword(const char *tok){ for(int i=0;Q_KEYWORDS[i];i++) if(strcmp(tok,Q_KEYWORDS[i])==0) return true; return false; }

/*
 * String data literals ("...") become one cell per byte — the tape's byte
 * representation everywhere else (vm_mem_cstr, file/sock buffers). Escapes:
 * \\ \" \n \t \r \0. Strings live as data; the interpreter never lexes them.
 */
static bool q_string_scan(const char *tok, int64_t *cells) {
    /* tok[0] == '"'; returns cell count into *cells, or false on bad escape
       or no closing quote. When cells is NULL this is a pure count pass. */
    size_t len = strlen(tok);
    if (len < 2 || tok[0] != '"' || tok[len-1] != '"') return false;
    int64_t n = 0;
    for (size_t i = 1; i + 1 < len; i++) {
        if (tok[i] == '\\') {
            i++;
            char e = tok[i];
            if (e == 'n' || e == 't' || e == 'r' || e == '\\' || e == '"' || e == '0') { n++; continue; }
            return false;
        }
        n++;
    }
    *cells = n;
    return true;
}
static bool q_is_string_token(const char *tok) {
    size_t len = tok ? strlen(tok) : 0;
    return len >= 2 && tok[0] == '"';
}
/* Appends the string's bytes (no terminator; callers add one) as values. */
static bool q_string_values(const char *tok, int64_t *out) {
    size_t len = strlen(tok);
    int64_t k = 0;
    for (size_t i = 1; i + 1 < len; i++) {
        if (tok[i] == '\\') {
            i++;
            char e = tok[i];
            switch (e) {
                case 'n': out[k++] = 10; break;
                case 't': out[k++] = 9;  break;
                case 'r': out[k++] = 13; break;
                case '\\': out[k++] = '\\'; break;
                case '"': out[k++] = '"'; break;
                case '0': out[k++] = 0; break;
            }
            continue;
        }
        out[k++] = (int64_t)(unsigned char)tok[i];
    }
    return true;
}

static bool q_tokenize(const char *src, size_t len, QTokenVec *out){
    q_tv_init(out);
    size_t i=0;
    while(i < len){
        char c=src[i];
        if(c==' '||c=='\t'||c=='\r'||c=='\n'){ i++; continue; }
        if(c==';'){ while(i<len && src[i]!='\n') i++; continue; }
        if(c=='{'||c=='}'||c=='='){ char *tok=malloc(2); tok[0]=c; tok[1]=0; q_tv_push(out,tok); i++; continue; }
        if(c==','){ char *tok=malloc(2); tok[0]=c; tok[1]=0; q_tv_push(out,tok); i++; continue; }
        if(c=='"'){ /* string literal with escapes, kept raw; closed on one line */
            size_t j=i+1;
            while(j<len && src[j]!='"'){
                if(src[j]=='\\'){ j+=2; continue; }
                if(src[j]=='\n') break;
                j++;
            }
            if(j>=len || src[j]!='"'){
                snprintf(q_errmsg,sizeof(q_errmsg),"unterminated string literal at position %zu", i);
                for(size_t k=0;k<out->count;k++) free(out->data[k]);
                free(out->data); out->data=NULL; out->count=out->cap=0;
                return false;
            }
            size_t l=j-i+1;
            char *tok=malloc(l+1); memcpy(tok,src+i,l); tok[l]=0; q_tv_push(out,tok);
            i=j+1;
            continue;
        }
        if(c=='-' && i+1<len && isdigit((unsigned char)src[i+1])){
            size_t j=i+1; while(j<len && isdigit((unsigned char)src[j])) j++;
            size_t l=j-i; char *tok=malloc(l+1); memcpy(tok,src+i,l); tok[l]=0; q_tv_push(out,tok); i=j; continue;
        }
        if(isdigit((unsigned char)c)){
            size_t j=i; while(j<len && isdigit((unsigned char)src[j])) j++;
            size_t l=j-i; char *tok=malloc(l+1); memcpy(tok,src+i,l); tok[l]=0; q_tv_push(out,tok); i=j; continue;
        }
        if(isalpha((unsigned char)c)||c=='_'){
            size_t j=i; while(j<len && (isalnum((unsigned char)src[j])||src[j]=='_'||src[j]=='-')) j++;
            size_t l=j-i; char *tok=malloc(l+1); memcpy(tok,src+i,l); tok[l]=0; q_tv_push(out,tok); i=j; continue;
        }
        snprintf(q_errmsg,sizeof(q_errmsg),"unexpected character '%c' at position %zu", c, i);
        for(size_t k=0;k<out->count;k++) free(out->data[k]);
        free(out->data);
        out->data=NULL; out->count=out->cap=0;
        return false;
    }
    return true;
}

static void q_encode_literal(QCpVec *out, uint64_t uv){
    /* takes the *unsigned* magnitude: INT64_MIN's magnitude is 2^63, which
       only exists on the 64-bit side — negative values on the data stack
       are built by callers appending CP_EN (-) after the magnitude */
    q_cv_push(out, CP_HAIR);
    char bits[128];
    if(uv==0){ bits[0]='0'; bits[1]=0; }
    else {
        char tmp[128]; int t=0;
        while(uv>0){ tmp[t++]=(uv&1)?'1':'0'; uv>>=1; }
        for(int j=0;j<t;j++) bits[j]=tmp[t-1-j]; bits[t]=0;
    }
    for(size_t j=0;bits[j];j++) q_cv_push(out, bits[j]=='1'?CP_DIG1:CP_DIG0);
    q_cv_push(out, CP_LS);
}

typedef struct { char *name; int idx; } QMapEntry;
typedef struct { QMapEntry *data; size_t count; size_t cap; } QMap;
static void q_map_init(QMap *m){ m->data=NULL; m->count=0; m->cap=0; }
static void q_map_free(QMap *m){ for(size_t i=0;i<m->count;i++) free(m->data[i].name); free(m->data); m->data=NULL; m->count=m->cap=0; }
static void q_map_push(QMap *m, const char *name, int idx){ if(m->count==m->cap){ m->cap=m->cap?m->cap*2:16; m->data=realloc(m->data,m->cap*sizeof(QMapEntry)); } m->data[m->count].name=strdup(name); m->data[m->count].idx=idx; m->count++; }
static bool q_map_find(QMap *m, const char *name, int *out_idx){ for(size_t i=0;i<m->count;i++) if(strcmp(m->data[i].name,name)==0){ if(out_idx) *out_idx=m->data[i].idx; return true; } return false; }

static bool q_mnemonic_to_cps_simple(const char *tok, QCpVec *out){
    if(strcmp(tok,"dup")==0){ q_cv_push(out,CP_ITIMES); return true; }
    if(strcmp(tok,"swap")==0){ q_cv_push(out,CP_NBSP); return true; }
    if(strcmp(tok,"drop")==0){ q_cv_push(out,CP_FIGURE); return true; }
    if(strcmp(tok,"rot")==0){ q_cv_push(out,CP_THIN); return true; }
    if(strcmp(tok,"neg")==0){ q_cv_push(out,CP_EN); return true; }
    if(strcmp(tok,"sub")==0){ q_cv_push(out,CP_EN); q_cv_push(out,CP_EN); return true; }
    if(strcmp(tok,"mod")==0){ q_cv_push(out,CP_EN); q_cv_push(out,CP_EN); q_cv_push(out,CP_EN); return true; }
    if(strcmp(tok,"id")==0){ q_cv_push(out,CP_EM); return true; }
    if(strcmp(tok,"add")==0){ q_cv_push(out,CP_EM); q_cv_push(out,CP_EM); return true; }
    if(strcmp(tok,"div")==0){ q_cv_push(out,CP_EM); q_cv_push(out,CP_EM); q_cv_push(out,CP_EM); return true; }
    if(strcmp(tok,"sign")==0){ q_cv_push(out,CP_THREE); return true; }
    if(strcmp(tok,"mul")==0){ q_cv_push(out,CP_THREE); q_cv_push(out,CP_THREE); return true; }
    if(strcmp(tok,"pow")==0){ q_cv_push(out,CP_THREE); q_cv_push(out,CP_THREE); q_cv_push(out,CP_THREE); return true; }
    if(strcmp(tok,"and")==0){ q_cv_push(out,CP_LRE); return true; }
    if(strcmp(tok,"or")==0){ q_cv_push(out,CP_RLE); return true; }
    if(strcmp(tok,"xor")==0){ q_cv_push(out,CP_PDF); return true; }
    if(strcmp(tok,"shl")==0){ q_cv_push(out,CP_LRO); return true; }
    if(strcmp(tok,"shr")==0){ q_cv_push(out,CP_RLO); return true; }
    if(strcmp(tok,"num-out")==0){ q_cv_push(out,CP_IDEO); return true; }
    if(strcmp(tok,"char-out")==0){ q_cv_push(out,CP_PUNCT); return true; }
    if(strcmp(tok,"char-in")==0){ q_cv_push(out,CP_MEDMATH); return true; }
    if(strcmp(tok,"rand")==0){ q_cv_push(out,CP_INV_SEP); return true; }
    if(strcmp(tok,"sleep")==0){ q_cv_push(out,CP_RLM); return true; }
    if(strcmp(tok,"store")==0){ q_cv_push(out,CP_INV_PLUS); return true; }
    if(strcmp(tok,"load")==0){ q_cv_push(out,CP_LRM); return true; }
    if(strcmp(tok,"sock-create")==0){ q_cv_push(out,CP_SOCK_CREATE); return true; }
    if(strcmp(tok,"sock-bind")==0){ q_cv_push(out,CP_SOCK_BIND); return true; }
    if(strcmp(tok,"sock-listen")==0){ q_cv_push(out,CP_SOCK_LISTEN); return true; }
    if(strcmp(tok,"sock-accept")==0){ q_cv_push(out,CP_SOCK_ACCEPT); return true; }
    if(strcmp(tok,"sock-recv")==0){ q_cv_push(out,CP_SOCK_RECV); return true; }
    if(strcmp(tok,"sock-send")==0){ q_cv_push(out,CP_SOCK_SEND); return true; }
    if(strcmp(tok,"sock-close")==0){ q_cv_push(out,CP_SOCK_CLOSE); return true; }
    if(strcmp(tok,"close")==0){ q_cv_push(out,CP_SOCK_CLOSE); return true; }
    if(strcmp(tok,"file-close")==0){ q_cv_push(out,CP_SOCK_CLOSE); return true; }
    if(strcmp(tok,"file-open")==0){ q_cv_push(out,CP_FILE_OPEN); return true; }
    if(strcmp(tok,"file-read")==0){ q_cv_push(out,CP_FILE_READ); return true; }
    if(strcmp(tok,"file-stat")==0){ q_cv_push(out,CP_FILE_STAT); return true; }
    if(strcmp(tok,"dir-list")==0){ q_cv_push(out,CP_DIR_LIST); return true; }
    if(strcmp(tok,"file-create")==0){ q_cv_push(out,CP_FILE_CREATE); return true; }
    if(strcmp(tok,"file-write")==0){ q_cv_push(out,CP_FILE_WRITE); return true; }
    if(strcmp(tok,"file-append")==0){ q_cv_push(out,CP_FILE_APPEND); return true; }
    return false;
}

typedef struct { char **body; size_t body_len; QMap *word_map; QMap *slot_map; QCpVec *word_cps; } QEmitCtx;

/* Emit `push value` (literal + EN-QUAD terminator), negative-safe: the
   encoding used everywhere a value must reach the data stack. */
static void q_emit_push_value(QCpVec *out, int64_t v){
    if(v<0){ q_encode_literal(out,0 - (uint64_t)v); q_cv_push(out,CP_EN); q_cv_push(out,CP_LS); }
    else q_encode_literal(out,(uint64_t)v);
}
/* Fill runs become `sys 18` (mem-set) calls; singles become push/store.
   Trailing zeros are skipped: the tape is zero-initialised. */
static void q_emit_data_values(QCpVec *out, const int64_t *v, size_t n, int64_t base){
    while(n>0 && v[n-1]==0) n--;
    size_t pos=0;
    while(pos<n){
        size_t run=1;
        while(pos+run<n && v[pos+run]==v[pos]) run++;
        if(run>=2){
            q_emit_push_value(out, base+(int64_t)pos);      /* addr */
            q_emit_push_value(out, v[pos]);                /* fill */
            q_emit_push_value(out, (int64_t)run);          /* len  */
            q_cv_push(out,CP_SYS); q_cv_push(out,CP_LS);   /* sys 18 */
        } else {
            q_emit_push_value(out, base+(int64_t)pos);
            q_emit_push_value(out, v[pos]);
            q_cv_push(out,CP_INV_PLUS); q_cv_push(out,CP_LS);
        }
        pos+=run;
    }
}
static bool q_emit_block(QEmitCtx *ctx, size_t *p_ptr){
    size_t p=*p_ptr; char **body=ctx->body; size_t n=ctx->body_len; QCpVec *word_cps=ctx->word_cps; QMap *word_map=ctx->word_map; QMap *slot_map=ctx->slot_map;
    while(p < n){
        const char *tok=body[p];
        if(strcmp(tok,"}")==0){ *p_ptr=p+1; return true; }
        else if(strcmp(tok,"if")==0){
            p+=1;
            if(p>=n||strcmp(body[p],"{")!=0){ q_set_error("expected '{' after 'if'"); return false; }
            p+=1;
            q_cv_push(word_cps,CP_WJ); q_cv_push(word_cps,CP_ZWNJ);
            QEmitCtx sub=*ctx; size_t sub_p=p;
            if(!q_emit_block(&sub,&sub_p)) return false;
            p=sub_p; q_cv_push(word_cps,CP_ZWJ);
            if(p<n && strcmp(body[p],"else")==0){
                p+=1;
                if(p>=n||strcmp(body[p],"{")!=0){ q_set_error("expected '{' after 'else'"); return false; }
                p+=1; q_cv_push(word_cps,CP_ZWNJ);
                QEmitCtx sub2=*ctx; size_t sub2_p=p;
                if(!q_emit_block(&sub2,&sub2_p)) return false;
                p=sub2_p; q_cv_push(word_cps,CP_ZWJ);
            }
            q_cv_push(word_cps,CP_LS);
        } else if(strcmp(tok,"while")==0){
            p+=1;
            if(p>=n||strcmp(body[p],"{")!=0){ q_set_error("expected '{' after 'while'"); return false; }
            p+=1;
            q_cv_push(word_cps,CP_ALM); q_cv_push(word_cps,CP_ZWNJ);
            QEmitCtx sub=*ctx; size_t sub_p=p;
            if(!q_emit_block(&sub,&sub_p)) return false;
            p=sub_p; q_cv_push(word_cps,CP_ZWJ);
            if(p>=n||strcmp(body[p],"{")!=0){ q_set_error("expected '{' loop body after 'while' condition (grammar: while { cond } { body })"); return false; }
            p+=1; q_cv_push(word_cps,CP_ZWNJ);
            QEmitCtx sub2=*ctx; size_t sub2_p=p;
            if(!q_emit_block(&sub2,&sub2_p)) return false;
            p=sub2_p; q_cv_push(word_cps,CP_ZWJ);
            q_cv_push(word_cps,CP_LS);
        } else if(strcmp(tok,"else")==0){ q_set_error("unexpected 'else' without matching 'if'"); return false; }
        else if(strcmp(tok,"let")==0){
            if(p+1>=n){ q_set_error("let without name"); return false; }
            const char *name=body[p+1]; int addr;
            if(!q_map_find(slot_map,name,&addr)){ q_set_error("undefined slot '%s' in let",name); return false; }
            if(p+2>=n||strcmp(body[p+2],"=")!=0){ q_set_error("let missing '='"); return false; }
            if(p+3>=n||!q_is_integer_token(body[p+3])){ q_set_error("let missing integer"); return false; }
            int64_t val=atoll(body[p+3]);
            q_encode_literal(word_cps,addr);
            if(val<0){ q_encode_literal(word_cps,-val); q_cv_push(word_cps,CP_EN); q_cv_push(word_cps,CP_LS); } else q_encode_literal(word_cps,val);
            q_cv_push(word_cps,CP_INV_PLUS); q_cv_push(word_cps,CP_LS);
            p+=4;
        } else if(strcmp(tok,"push")==0){
            if(p+1>=n){ q_set_error("push requires an operand"); return false; }
            const char *arg=body[p+1];
            if(q_is_integer_token(arg)){
                int64_t val=atoll(arg);
                q_emit_push_value(word_cps,val);
                p+=2;
            } else if(q_is_identifier(arg)){
                /* push <name>: the slot's / data block's base address —
                   the one general way to take a data address in Quill */
                int addr;
                if(q_map_find(slot_map,arg,&addr)){ q_encode_literal(word_cps,addr); p+=2; }
                else { q_set_error("push: unknown slot or data name '%s'",arg); return false; }
            } else { q_set_error("push operand must be an integer, slot or data name, got '%s'",arg); return false; }
        } else if(strcmp(tok,"not")==0){
            /* bitwise complement: sugar for push -1; xor (U+202C run) */
            q_encode_literal(word_cps,1);
            q_cv_push(word_cps,CP_EN);       /* neg: -1 */
            q_cv_push(word_cps,CP_PDF);      /* xor: a ^ -1 */
            q_cv_push(word_cps,CP_LS);
            p+=1;
        } else if(strcmp(tok,"sys")==0){
            /* `sys <num>` sugar pushes the number first. A bare `sys` (what
               the Scribe compiler emits after pushing the number itself)
               just appends the syscall codepoint. */
            if(p+1<n && q_is_integer_token(body[p+1])){
                int64_t num=atoll(body[p+1]);
                if(num<0){ q_set_error("sys requires a non-negative syscall number"); return false; }
                q_encode_literal(word_cps,num);
                p+=2;
            } else {
                p+=1;
            }
            q_cv_push(word_cps,CP_SYS);
            q_cv_push(word_cps,CP_LS);
        } else if(strcmp(tok,"dup")==0||strcmp(tok,"swap")==0||strcmp(tok,"drop")==0||strcmp(tok,"rot")==0||strcmp(tok,"neg")==0||strcmp(tok,"sub")==0||strcmp(tok,"mod")==0||strcmp(tok,"id")==0||strcmp(tok,"add")==0||strcmp(tok,"div")==0||strcmp(tok,"sign")==0||strcmp(tok,"mul")==0||strcmp(tok,"pow")==0||strcmp(tok,"and")==0||strcmp(tok,"or")==0||strcmp(tok,"xor")==0||strcmp(tok,"shl")==0||strcmp(tok,"shr")==0||strcmp(tok,"num-out")==0||strcmp(tok,"char-out")==0||strcmp(tok,"char-in")==0||strcmp(tok,"rand")==0||strcmp(tok,"sleep")==0||strcmp(tok,"halt")==0||strcmp(tok,"load")==0||strcmp(tok,"store")==0||strcmp(tok,"call")==0||strcmp(tok,"sock-create")==0||strcmp(tok,"sock-bind")==0||strcmp(tok,"sock-listen")==0||strcmp(tok,"sock-accept")==0||strcmp(tok,"sock-recv")==0||strcmp(tok,"sock-send")==0||strcmp(tok,"sock-close")==0||strcmp(tok,"close")==0||strcmp(tok,"file-close")==0||strcmp(tok,"file-open")==0||strcmp(tok,"file-read")==0||strcmp(tok,"file-stat")==0||strcmp(tok,"dir-list")==0||strcmp(tok,"file-create")==0||strcmp(tok,"file-write")==0||strcmp(tok,"file-append")==0){
            if(strcmp(tok,"halt")==0){ q_cv_push(word_cps,CP_ZWSP); p+=1; }
            else if(strcmp(tok,"call")==0){
                if(p+1>=n){ q_set_error("call requires word name or index"); return false; }
                const char *arg=body[p+1];
                if(q_is_integer_token(arg)){
                    int64_t idx=atoll(arg);
                    if(idx<0){ q_encode_literal(word_cps,-idx); q_cv_push(word_cps,CP_EN); q_cv_push(word_cps,CP_LS); } else q_encode_literal(word_cps,idx);
                    q_cv_push(word_cps,CP_FA); q_cv_push(word_cps,CP_LS); p+=2;
                } else if(q_is_identifier(arg)){
                    int idx; if(q_map_find(word_map,arg,&idx)){ q_encode_literal(word_cps,idx); q_cv_push(word_cps,CP_FA); q_cv_push(word_cps,CP_LS); p+=2; }
                    else { q_set_error("call to undefined word '%s'",arg); return false; }
                } else { q_set_error("call argument must be word name or integer, got '%s'",arg); return false; }
            } else if(strcmp(tok,"load")==0){
                if(p+1 < n && !q_is_keyword(body[p+1]) && strcmp(body[p+1],"{")!=0 && strcmp(body[p+1],"}")!=0 && strcmp(body[p+1],"=")!=0 && q_is_identifier(body[p+1])){
                    const char *name=body[p+1]; int addr; if(q_map_find(slot_map,name,&addr)){ q_encode_literal(word_cps,addr); q_cv_push(word_cps,CP_LRM); q_cv_push(word_cps,CP_LS); p+=2; } else { q_set_error("load from undefined slot '%s'",name); return false; }
                } else if(p+1 < n && q_is_integer_token(body[p+1])){
                    int64_t addr=atoll(body[p+1]); if(addr<0){ q_encode_literal(word_cps,-addr); q_cv_push(word_cps,CP_EN); q_cv_push(word_cps,CP_LS);} else q_encode_literal(word_cps,addr); q_cv_push(word_cps,CP_LRM); q_cv_push(word_cps,CP_LS); p+=2;
                } else { q_cv_push(word_cps,CP_LRM); q_cv_push(word_cps,CP_LS); p+=1; }
            } else if(strcmp(tok,"store")==0){
                if(p+1 < n && !q_is_keyword(body[p+1]) && strcmp(body[p+1],"{")!=0 && strcmp(body[p+1],"}")!=0 && strcmp(body[p+1],"=")!=0 && q_is_identifier(body[p+1])){
                    const char *name=body[p+1]; int addr; if(q_map_find(slot_map,name,&addr)){ q_encode_literal(word_cps,addr); q_cv_push(word_cps,CP_NBSP); q_cv_push(word_cps,CP_LS); q_cv_push(word_cps,CP_INV_PLUS); q_cv_push(word_cps,CP_LS); p+=2; } else { q_set_error("store to undefined slot '%s'",name); return false; }
                } else if(p+1 < n && q_is_integer_token(body[p+1])){
                    int64_t addr=atoll(body[p+1]); if(addr<0){ q_encode_literal(word_cps,-addr); q_cv_push(word_cps,CP_EN); q_cv_push(word_cps,CP_LS);} else q_encode_literal(word_cps,addr); q_cv_push(word_cps,CP_NBSP); q_cv_push(word_cps,CP_LS); q_cv_push(word_cps,CP_INV_PLUS); q_cv_push(word_cps,CP_LS); p+=2;
                } else { q_cv_push(word_cps,CP_INV_PLUS); q_cv_push(word_cps,CP_LS); p+=1; }
            } else if(strcmp(tok,"id")==0){ p+=1; }
            else { QCpVec tmp; q_cv_init(&tmp); q_mnemonic_to_cps_simple(tok,&tmp); for(size_t j=0;j<tmp.count;j++) q_cv_push(word_cps,tmp.data[j]); q_cv_free(&tmp); q_cv_push(word_cps,CP_LS); p+=1; }
        } else if(strcmp(tok,"data")==0){
            q_set_error("unexpected 'data' inside a word body (data blocks are top-level, like 'word')");
            return false;
        } else if(strcmp(tok,"{")==0||strcmp(tok,"}")==0||strcmp(tok,"=")==0){ q_set_error("unexpected token '%s'",tok); return false; }
        else if(q_is_integer_token(tok)){ q_set_error("unexpected integer '%s' (missing 'push'?)",tok); return false; }
        else if(q_is_identifier(tok)){ q_set_error("unknown identifier or misplaced token '%s'",tok); return false; }
        else if(strcmp(tok,"word")==0){ q_set_error("unexpected 'word' inside word body (word definitions must be top-level)"); return false; }
        else { q_set_error("unexpected token '%s'",tok); return false; }
    }
    *p_ptr=p; return true;
}

typedef struct { char *name; int64_t base; int64_t len; size_t tstart, tend; } QData;
typedef struct { QData *data; size_t count, cap; } QDataVec;
static void q_dv_init(QDataVec *v){ v->data=NULL; v->count=0; v->cap=0; }
static void q_dv_push(QDataVec *v, QData d){
    if(v->count==v->cap){ v->cap=v->cap?v->cap*2:8; v->data=realloc(v->data, v->cap*sizeof(QData)); }
    v->data[v->count++]=d;
}
static void q_dv_free(QDataVec *v){ for(size_t i=0;i<v->count;i++) free(v->data[i].name); free(v->data); v->data=NULL; v->count=v->cap=0; }

bool q_assemble(const char *source, size_t src_len, QCpVec *out_prog){
    QTokenVec tokens;
    if(!q_tokenize(source,src_len,&tokens)) return false;
    if(tokens.count==0){ q_set_error("empty source: need at least one word definition"); q_tv_free(&tokens); return false; }
    QMap word_map; q_map_init(&word_map); QMap slot_map; q_map_init(&slot_map);
    QDataVec dvec; q_dv_init(&dvec);
    int next_word=0,next_slot=0; size_t i=0;
    while(i<tokens.count){
        const char *tok=tokens.data[i];
        if(strcmp(tok,"word")==0){
            if(i+1>=tokens.count){ q_set_error("word without name"); goto fail; }
            const char *name=tokens.data[i+1];
            if(q_is_keyword(name)){ q_set_error("word name cannot be keyword '%s'",name); goto fail; }
            if(!q_is_identifier(name)){ q_set_error("invalid word name '%s'",name); goto fail; }
            int dummy; if(q_map_find(&word_map,name,&dummy)){ q_set_error("duplicate word name '%s'",name); goto fail; }
            q_map_push(&word_map,name,next_word++); i+=2;
        } else if(strcmp(tok,"let")==0){
            if(i+1>=tokens.count){ q_set_error("let without name"); goto fail; }
            const char *name=tokens.data[i+1];
            if(q_is_keyword(name)){ q_set_error("slot name cannot be keyword '%s'",name); goto fail; }
            if(!q_is_identifier(name)){ q_set_error("invalid slot name '%s'",name); goto fail; }
            int dummy; if(q_map_find(&slot_map,name,&dummy)){ q_set_error("duplicate slot name '%s'",name); goto fail; }
            q_map_push(&slot_map,name,next_slot++);
            if(i+2>=tokens.count||strcmp(tokens.data[i+2],"=")!=0){ q_set_error("let %s missing '='",name); goto fail; }
            if(i+3>=tokens.count||!q_is_integer_token(tokens.data[i+3])){ q_set_error("let %s missing integer value",name); goto fail; }
            i+=4;
        } else if(strcmp(tok,"data")==0){
            /* data <name> = item (, item)*   — items: integer or string.
               Strings append a zero terminator cell (C-string convention). */
            if(i+1>=tokens.count){ q_set_error("data without name"); goto fail; }
            const char *name=tokens.data[i+1];
            if(q_is_keyword(name)){ q_set_error("data name cannot be keyword '%s'",name); goto fail; }
            if(!q_is_identifier(name)){ q_set_error("invalid data name '%s'",name); goto fail; }
            int dummy;
            if(q_map_find(&slot_map,name,&dummy)){ q_set_error("duplicate slot/data name '%s'",name); goto fail; }
            if(q_map_find(&word_map,name,&dummy)){ q_set_error("'%s' is both a word and a data name",name); goto fail; }
            q_map_push(&slot_map,name,next_slot);          /* base address */
            if(i+2>=tokens.count||strcmp(tokens.data[i+2],"=")!=0){ q_set_error("data %s missing '='",name); goto fail; }
            size_t j=i+3;
            int64_t span=0;
            if(j>=tokens.count){ q_set_error("data %s without values",name); goto fail; }
            for(;;){
                const char *item=tokens.data[j];
                if(q_is_string_token(item)){
                    int64_t cells;
                    if(!q_string_scan(item,&cells)){ q_set_error("data %s: bad string literal or escape in %s",name,item); goto fail; }
                    span += cells + 1;                 /* + zero terminator */
                } else if(q_is_integer_token(item)){
                    span += 1;
                } else { q_set_error("data %s: item must be an integer or string, got '%s'",name,item); goto fail; }
                j++;
                if(j<tokens.count && strcmp(tokens.data[j],",")==0){
                    j++;
                    if(j>=tokens.count){ q_set_error("data %s: list ends with ','",name); goto fail; }
                    continue;
                }
                break;
            }
            QData d; d.name=strdup(name); d.base=next_slot; d.len=span;
            d.tstart=i+3; d.tend=j;
            q_dv_push(&dvec,d);
            next_slot += span;
            i=j;
        } else i+=1;
    }
    if(next_word==0){ q_set_error("program must contain at least one `word` definition (first is entry point)"); goto fail; }
    size_t word_starts_cap=0,word_starts_count=0; size_t *word_starts=NULL;
    for(size_t idx=0;idx<tokens.count;idx++) if(strcmp(tokens.data[idx],"word")==0){ if(word_starts_count==word_starts_cap){ word_starts_cap=word_starts_cap?word_starts_cap*2:8; word_starts=realloc(word_starts,word_starts_cap*sizeof(size_t)); } word_starts[word_starts_count++]=idx; }
    if(word_starts_count != (size_t)next_word){ q_set_error("internal: word count mismatch"); goto fail; }
    q_cv_init(out_prog);
    for(size_t wi=0;wi<word_starts_count;wi++){
        size_t start=word_starts[wi]; size_t end=(wi+1<word_starts_count)?word_starts[wi+1]:tokens.count;
        size_t body_len=(end>start+2)?(end-(start+2)):0;
        char **body= body_len? &tokens.data[start+2] : NULL;
        QCpVec word_cps; q_cv_init(&word_cps);
        if(wi==0){ /* prologue: initialise every data block before word 0 runs */
            for(size_t k=0;k<dvec.count;k++){
                int64_t init_idx=(int64_t)((size_t)next_word + k);
                q_encode_literal(&word_cps,init_idx);
                q_cv_push(&word_cps,CP_FA); q_cv_push(&word_cps,CP_LS);
            }
        }
        QEmitCtx ctx; ctx.body=body; ctx.body_len=body_len; ctx.word_map=&word_map; ctx.slot_map=&slot_map; ctx.word_cps=&word_cps;
        size_t p=0; if(!q_emit_block(&ctx,&p)){ q_cv_free(&word_cps); q_cv_free(out_prog); goto fail_prog_made; }
        for(size_t k=0;k<word_cps.count;k++) q_cv_push(out_prog,word_cps.data[k]);
        q_cv_free(&word_cps);
        if(wi!=word_starts_count-1) q_cv_push(out_prog,CP_PS);
    }
    /* data init words come last: one per data block, called by the word-0
       prologue. Disassembly shows them as ordinary store/fill words. */
    for(size_t k=0;k<dvec.count;k++){
        QData *d=&dvec.data[k];
        int64_t *vals=malloc((size_t)d->len*sizeof(int64_t));
        size_t pos=0;
        size_t j=d->tstart;
        for(;;){
            const char *item=tokens.data[j];
            if(q_is_string_token(item)){
                /* string bytes, then the terminator cell (strings are
                   C-strings: one trailing zero cell each) */
                int64_t cells; q_string_scan(item,&cells);
                int64_t *tmp=malloc((size_t)(cells>0?cells:1)*sizeof(int64_t));
                q_string_values(item,tmp);
                for(int64_t c=0;c<cells;c++) vals[pos++]=tmp[c];
                free(tmp);
                vals[pos++]=0;
            } else {
                vals[pos++]=atoll(item);
            }
            j++;
            if(j<tokens.count && strcmp(tokens.data[j],",")==0){ j++; continue; }
            break;
        }
        if(pos != (size_t)d->len){ q_set_error("internal: data '%s' length mismatch",d->name); free(vals); q_cv_free(out_prog); goto fail_prog_made; }
        q_cv_push(out_prog,CP_PS);
        q_emit_data_values(out_prog, vals, pos, d->base);
        free(vals);
    }
    free(word_starts); q_tv_free(&tokens); q_map_free(&word_map); q_map_free(&slot_map);
    q_dv_free(&dvec);
    return true;

fail_prog_made:
    free(word_starts);
fail:
    q_tv_free(&tokens); q_map_free(&word_map); q_map_free(&slot_map);
    q_dv_free(&dvec);
    return false;
}


typedef struct { char *data; size_t len; size_t cap; } SB;
static void sb_init(SB *sb){ sb->cap=8192; sb->len=0; sb->data=malloc(sb->cap); sb->data[0]=0; }
static void sb_append(SB *sb, const char *fmt, ...){
    va_list ap; va_start(ap,fmt);
    int needed=vsnprintf(NULL,0,fmt,ap); va_end(ap);
    if(sb->len + (size_t)needed + 1 > sb->cap){ while(sb->len+(size_t)needed+1 > sb->cap) sb->cap*=2; sb->data=realloc(sb->data,sb->cap); }
    va_start(ap,fmt); int w=vsnprintf(sb->data+sb->len, sb->cap - sb->len, fmt, ap); va_end(ap); sb->len+=(size_t)w;
}
static void sb_free(SB *sb) __attribute__((unused));
static void sb_free(SB *sb){ free(sb->data); }

static void sb_emit_block(SB *sb, Block *blk, int indent){
    for(size_t i=0;i<blk->count;i++){
        Instr *ins=&blk->items[i];
        if(ins->op==OP_PUSH && i+1<blk->count && blk->items[i+1].op==OP_CALL){
            for(int k=0;k<indent;k++) sb_append(sb,"    ");
            sb_append(sb,"call %lld\n",(long long)ins->value);
            i++; continue;
        }
        if(ins->op==OP_PUSH){
            for(int k=0;k<indent;k++) sb_append(sb,"    ");
            sb_append(sb,"push %lld\n",(long long)ins->value);
            continue;
        }
        if(ins->op==OP_IF){
            for(int k=0;k<indent;k++) sb_append(sb,"    ");
            sb_append(sb,"if {\n");
            sb_emit_block(sb, ins->then_blk, indent+1);
            if(ins->else_blk){
                for(int k=0;k<indent;k++) sb_append(sb,"    ");
                sb_append(sb,"} else {\n");
                sb_emit_block(sb, ins->else_blk, indent+1);
                for(int k=0;k<indent;k++) sb_append(sb,"    ");
                sb_append(sb,"}\n");
            } else {
                for(int k=0;k<indent;k++) sb_append(sb,"    ");
                sb_append(sb,"}\n");
            }
            continue;
        }
        if(ins->op==OP_LOOP){
            for(int k=0;k<indent;k++) sb_append(sb,"    ");
            sb_append(sb,"while {\n");
            sb_emit_block(sb, ins->then_blk, indent+1);
            for(int k=0;k<indent;k++) sb_append(sb,"    ");
            sb_append(sb,"} {\n");
            sb_emit_block(sb, ins->else_blk, indent+1);
            for(int k=0;k<indent;k++) sb_append(sb,"    ");
            sb_append(sb,"}\n");
            continue;
        } else {
            const char *mn=NULL;
            switch(ins->op){
                case OP_DUP: mn="dup"; break; case OP_SWAP: mn="swap"; break; case OP_DROP: mn="drop"; break; case OP_ROT: mn="rot"; break;
                case OP_NEG: mn="neg"; break; case OP_SUB: mn="sub"; break; case OP_MOD: mn="mod"; break;
                case OP_ADD: mn="add"; break; case OP_DIV: mn="div"; break; case OP_SIGN: mn="sign"; break; case OP_MUL: mn="mul"; break; case OP_POW: mn="pow"; break;
                case OP_AND: mn="and"; break; case OP_OR: mn="or"; break; case OP_XOR: mn="xor"; break; case OP_SHL: mn="shl"; break; case OP_SHR: mn="shr"; break;
                case OP_NUMOUT: mn="num-out"; break; case OP_CHAROUT: mn="char-out"; break; case OP_CHARIN: mn="char-in"; break;
                case OP_CALL: mn="call"; break; case OP_HALT: mn="halt"; break; case OP_RAND: mn="rand"; break; case OP_STORE: mn="store"; break; case OP_LOAD: mn="load"; break; case OP_SLEEP: mn="sleep"; break;
                case OP_SOCK_CREATE: mn="sock-create"; break; case OP_SOCK_BIND: mn="sock-bind"; break; case OP_SOCK_LISTEN: mn="sock-listen"; break; case OP_SOCK_ACCEPT: mn="sock-accept"; break; case OP_SOCK_RECV: mn="sock-recv"; break; case OP_SOCK_SEND: mn="sock-send"; break; case OP_SOCK_CLOSE: mn="sock-close"; break;
                case OP_FILE_OPEN: mn="file-open"; break; case OP_FILE_READ: mn="file-read"; break; case OP_FILE_STAT: mn="file-stat"; break; case OP_DIR_LIST: mn="dir-list"; break;
                case OP_FILE_CREATE: mn="file-create"; break; case OP_FILE_WRITE: mn="file-write"; break; case OP_FILE_APPEND: mn="file-append"; break;
                case OP_SYSCALL: mn="sys"; break;
                default: mn="unknown"; break;
            }
            for(int k=0;k<indent;k++) sb_append(sb,"    ");
            sb_append(sb,"%s\n",mn);
        }
    }
}

char *q_disassemble_words(Block **blocks, size_t word_count){
    if(word_count==0 || !blocks){ q_set_error("no words to disassemble"); return NULL; }
    SB sb; sb_init(&sb);
    for(size_t wi=0; wi<word_count; wi++){
        sb_append(&sb,"word word_%zu\n",wi);
        sb_emit_block(&sb, blocks[wi], 1);
        if(wi!=word_count-1) sb_append(&sb,"\n");
    }
    if(sb.len==0 || sb.data[sb.len-1]!='\n') sb_append(&sb,"\n");
    return sb.data;
}

char* q_disassemble_bytes(const unsigned char *data, size_t data_len){
    size_t full_len; cp_t *full=utf8_decode(data,data_len,&full_len);
    size_t bom_idx=(size_t)-1;
    for(size_t idx=0;idx<full_len;idx++) if(full[idx]==CP_BOM){ bom_idx=idx; break; }
    size_t start,end;
    if(bom_idx==(size_t)-1){ start=0; end=full_len; }
    else { start=bom_idx+1; end=full_len; for(size_t idx=start; idx<full_len; idx++) if(full[idx]==CP_ZWSP){ end=idx; break; } }
    cp_t *prog=malloc((end-start)*sizeof(cp_t));
    size_t prog_len=0;
    for(size_t i=start;i<end;i++) if(is_spec_cp(full[i])) prog[prog_len++]=full[i];
    free(full);
    if(prog_len==0){ q_set_error("this document does not contain Invisible Ink"); free(prog); return NULL; }
    size_t word_count=1; for(size_t i=0;i<prog_len;i++) if(prog[i]==CP_PS) word_count++;
    Block **blocks=malloc(word_count*sizeof(Block*));
    size_t seg_start=0, w=0;
    for(size_t i=0;i<=prog_len;i++){
        if(i==prog_len||prog[i]==CP_PS){
            size_t seg_len=i-seg_start; size_t pos=0;
            Block *blk=parse_block(prog+seg_start, seg_len, &pos, false);
            if(!blk){
                for(size_t k=0;k<w;k++) block_free(blocks[k]);
                free(blocks);
                free(prog);
                q_set_error("parse error: %s (at codepoint %zu)", g_parse_err, g_parse_pos + seg_start);
                return NULL;
            }
            blocks[w++]=blk; seg_start=i+1;
        }
    }
    free(prog);
    char *out = q_disassemble_words(blocks, word_count);
    for(size_t k=0;k<word_count;k++) block_free(blocks[k]);
    free(blocks);
    return out;
}

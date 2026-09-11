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
static const char *Q_KEYWORDS[] = {"word","if","else","let","call","halt","push","dup","swap","drop","rot","neg","sub","mod","id","add","div","sign","mul","pow","num-out","char-out","char-in","rand","sleep","store","load",NULL};
static bool q_is_keyword(const char *tok){ for(int i=0;Q_KEYWORDS[i];i++) if(strcmp(tok,Q_KEYWORDS[i])==0) return true; return false; }

static bool q_tokenize(const char *src, size_t len, QTokenVec *out){
    q_tv_init(out);
    size_t i=0;
    while(i < len){
        char c=src[i];
        if(c==' '||c=='\t'||c=='\r'||c=='\n'){ i++; continue; }
        if(c==';'){ while(i<len && src[i]!='\n') i++; continue; }
        if(c=='{'||c=='}'||c=='='){ char *tok=malloc(2); tok[0]=c; tok[1]=0; q_tv_push(out,tok); i++; continue; }
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

static void q_encode_literal(QCpVec *out, int64_t value){
    q_cv_push(out, CP_HAIR);
    char bits[128];
    if(value==0){ bits[0]='0'; bits[1]=0; }
    else {
        char tmp[128]; int t=0; int64_t v=value;
        while(v>0){ tmp[t++]=(v&1)?'1':'0'; v>>=1; }
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
    if(strcmp(tok,"num-out")==0){ q_cv_push(out,CP_IDEO); return true; }
    if(strcmp(tok,"char-out")==0){ q_cv_push(out,CP_PUNCT); return true; }
    if(strcmp(tok,"char-in")==0){ q_cv_push(out,CP_MEDMATH); return true; }
    if(strcmp(tok,"rand")==0){ q_cv_push(out,CP_INV_SEP); return true; }
    if(strcmp(tok,"sleep")==0){ q_cv_push(out,CP_RLM); return true; }
    if(strcmp(tok,"store")==0){ q_cv_push(out,CP_INV_PLUS); return true; }
    if(strcmp(tok,"load")==0){ q_cv_push(out,CP_LRM); return true; }
    return false;
}

typedef struct { char **body; size_t body_len; QMap *word_map; QMap *slot_map; QCpVec *word_cps; } QEmitCtx;
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
            if(p+1>=n||!q_is_integer_token(body[p+1])){ q_set_error("push requires integer literal"); return false; }
            int64_t val=atoll(body[p+1]);
            if(val<0){ q_encode_literal(word_cps,-val); q_cv_push(word_cps,CP_EN); q_cv_push(word_cps,CP_LS); } else q_encode_literal(word_cps,val);
            p+=2;
        } else if(strcmp(tok,"dup")==0||strcmp(tok,"swap")==0||strcmp(tok,"drop")==0||strcmp(tok,"rot")==0||strcmp(tok,"neg")==0||strcmp(tok,"sub")==0||strcmp(tok,"mod")==0||strcmp(tok,"id")==0||strcmp(tok,"add")==0||strcmp(tok,"div")==0||strcmp(tok,"sign")==0||strcmp(tok,"mul")==0||strcmp(tok,"pow")==0||strcmp(tok,"num-out")==0||strcmp(tok,"char-out")==0||strcmp(tok,"char-in")==0||strcmp(tok,"rand")==0||strcmp(tok,"sleep")==0||strcmp(tok,"halt")==0||strcmp(tok,"load")==0||strcmp(tok,"store")==0||strcmp(tok,"call")==0){
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
        } else if(strcmp(tok,"{")==0||strcmp(tok,"}")==0||strcmp(tok,"=")==0){ q_set_error("unexpected token '%s'",tok); return false; }
        else if(q_is_integer_token(tok)){ q_set_error("unexpected integer '%s' (missing 'push'?)",tok); return false; }
        else if(q_is_identifier(tok)){ q_set_error("unknown identifier or misplaced token '%s'",tok); return false; }
        else if(strcmp(tok,"word")==0){ q_set_error("unexpected 'word' inside word body (word definitions must be top-level)"); return false; }
        else { q_set_error("unexpected token '%s'",tok); return false; }
    }
    *p_ptr=p; return true;
}

bool q_assemble(const char *source, size_t src_len, QCpVec *out_prog){
    QTokenVec tokens;
    if(!q_tokenize(source,src_len,&tokens)) return false;
    if(tokens.count==0){ q_set_error("empty source: need at least one word definition"); q_tv_free(&tokens); return false; }
    QMap word_map; q_map_init(&word_map); QMap slot_map; q_map_init(&slot_map);
    int next_word=0,next_slot=0; size_t i=0;
    while(i<tokens.count){
        const char *tok=tokens.data[i];
        if(strcmp(tok,"word")==0){
            if(i+1>=tokens.count){ q_set_error("word without name"); q_tv_free(&tokens); q_map_free(&word_map); q_map_free(&slot_map); return false; }
            const char *name=tokens.data[i+1];
            if(q_is_keyword(name)){ q_set_error("word name cannot be keyword '%s'",name); q_tv_free(&tokens); q_map_free(&word_map); q_map_free(&slot_map); return false; }
            if(!q_is_identifier(name)){ q_set_error("invalid word name '%s'",name); q_tv_free(&tokens); q_map_free(&word_map); q_map_free(&slot_map); return false; }
            int dummy; if(q_map_find(&word_map,name,&dummy)){ q_set_error("duplicate word name '%s'",name); q_tv_free(&tokens); q_map_free(&word_map); q_map_free(&slot_map); return false; }
            q_map_push(&word_map,name,next_word++); i+=2;
        } else if(strcmp(tok,"let")==0){
            if(i+1>=tokens.count){ q_set_error("let without name"); q_tv_free(&tokens); q_map_free(&word_map); q_map_free(&slot_map); return false; }
            const char *name=tokens.data[i+1];
            if(q_is_keyword(name)){ q_set_error("slot name cannot be keyword '%s'",name); q_tv_free(&tokens); q_map_free(&word_map); q_map_free(&slot_map); return false; }
            if(!q_is_identifier(name)){ q_set_error("invalid slot name '%s'",name); q_tv_free(&tokens); q_map_free(&word_map); q_map_free(&slot_map); return false; }
            int dummy; if(q_map_find(&slot_map,name,&dummy)){ q_set_error("duplicate slot name '%s'",name); q_tv_free(&tokens); q_map_free(&word_map); q_map_free(&slot_map); return false; }
            q_map_push(&slot_map,name,next_slot++);
            if(i+2>=tokens.count||strcmp(tokens.data[i+2],"=")!=0){ q_set_error("let %s missing '='",name); q_tv_free(&tokens); q_map_free(&word_map); q_map_free(&slot_map); return false; }
            if(i+3>=tokens.count||!q_is_integer_token(tokens.data[i+3])){ q_set_error("let %s missing integer value",name); q_tv_free(&tokens); q_map_free(&word_map); q_map_free(&slot_map); return false; }
            i+=4;
        } else i+=1;
    }
    if(next_word==0){ q_set_error("program must contain at least one `word` definition (first is entry point)"); q_tv_free(&tokens); q_map_free(&word_map); q_map_free(&slot_map); return false; }
    size_t word_starts_cap=0,word_starts_count=0; size_t *word_starts=NULL;
    for(size_t idx=0;idx<tokens.count;idx++) if(strcmp(tokens.data[idx],"word")==0){ if(word_starts_count==word_starts_cap){ word_starts_cap=word_starts_cap?word_starts_cap*2:8; word_starts=realloc(word_starts,word_starts_cap*sizeof(size_t)); } word_starts[word_starts_count++]=idx; }
    q_cv_init(out_prog);
    for(size_t wi=0;wi<word_starts_count;wi++){
        size_t start=word_starts[wi]; size_t end=(wi+1<word_starts_count)?word_starts[wi+1]:tokens.count;
        size_t body_len=(end>start+2)?(end-(start+2)):0;
        char **body= body_len? &tokens.data[start+2] : NULL;
        QCpVec word_cps; q_cv_init(&word_cps);
        QEmitCtx ctx; ctx.body=body; ctx.body_len=body_len; ctx.word_map=&word_map; ctx.slot_map=&slot_map; ctx.word_cps=&word_cps;
        size_t p=0; if(!q_emit_block(&ctx,&p)){ q_cv_free(&word_cps); q_cv_free(out_prog); free(word_starts); q_tv_free(&tokens); q_map_free(&word_map); q_map_free(&slot_map); return false; }
        for(size_t k=0;k<word_cps.count;k++) q_cv_push(out_prog,word_cps.data[k]);
        q_cv_free(&word_cps);
        if(wi!=word_starts_count-1) q_cv_push(out_prog,CP_PS);
    }
    free(word_starts); q_tv_free(&tokens); q_map_free(&word_map); q_map_free(&slot_map);
    return true;
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
        } else {
            const char *mn=NULL;
            switch(ins->op){
                case OP_DUP: mn="dup"; break; case OP_SWAP: mn="swap"; break; case OP_DROP: mn="drop"; break; case OP_ROT: mn="rot"; break;
                case OP_NEG: mn="neg"; break; case OP_SUB: mn="sub"; break; case OP_MOD: mn="mod"; break;
                case OP_ADD: mn="add"; break; case OP_DIV: mn="div"; break; case OP_SIGN: mn="sign"; break; case OP_MUL: mn="mul"; break; case OP_POW: mn="pow"; break;
                case OP_NUMOUT: mn="num-out"; break; case OP_CHAROUT: mn="char-out"; break; case OP_CHARIN: mn="char-in"; break;
                case OP_CALL: mn="call"; break; case OP_HALT: mn="halt"; break; case OP_RAND: mn="rand"; break; case OP_STORE: mn="store"; break; case OP_LOAD: mn="load"; break; case OP_SLEEP: mn="sleep"; break;
                default: mn="unknown"; break;
            }
            for(int k=0;k<indent;k++) sb_append(sb,"    ");
            sb_append(sb,"%s\n",mn);
        }
    }
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
            blocks[w++]=blk; seg_start=i+1;
        }
    }
    free(prog);
    SB sb; sb_init(&sb);
    for(size_t wi=0; wi<word_count; wi++){
        sb_append(&sb,"word word_%zu\n",wi);
        sb_emit_block(&sb, blocks[wi], 1);
        if(wi!=word_count-1) sb_append(&sb,"\n");
    }
    if(sb.len==0 || sb.data[sb.len-1]!='\n') sb_append(&sb,"\n");
    for(size_t k=0;k<word_count;k++) block_free(blocks[k]);
    free(blocks);
    char *out=sb.data;
    return out;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdarg.h>
#include <errno.h>

enum {
    CP_ITIMES  = 0x2062,
    CP_NBSP    = 0x00A0,
    CP_FIGURE  = 0x2007,
    CP_THIN    = 0x2009,
    CP_HAIR    = 0x200A,
    CP_DIG0    = 0x2005,
    CP_DIG1    = 0x2006,
    CP_IDEO    = 0x3000,
    CP_PUNCT   = 0x2008,
    CP_MEDMATH = 0x205F,
    CP_EN      = 0x2002,
    CP_EM      = 0x2003,
    CP_THREE   = 0x2004,
    CP_ZWNJ    = 0x200C,
    CP_ZWJ     = 0x200D,
    CP_WJ      = 0x2060,
    CP_FA      = 0x2061,
    CP_LS      = 0x2000,
    CP_PS      = 0x2001,
    CP_ZWSP    = 0x200B,
    CP_BOM     = 0xFEFF,
    CP_INV_SEP = 0x2063,
    CP_INV_PLUS= 0x2064,
    CP_LRM     = 0x200E,
    CP_RLM     = 0x200F,
    CP_SOFT    = 0x00AD
};

static bool is_spec_cp(uint32_t c) {
    switch(c){
        case CP_ITIMES: case CP_NBSP: case CP_FIGURE: case CP_THIN:
        case CP_HAIR: case CP_DIG0: case CP_DIG1:
        case CP_IDEO: case CP_PUNCT: case CP_MEDMATH:
        case CP_EN: case CP_EM: case CP_THREE:
        case CP_ZWNJ: case CP_ZWJ: case CP_WJ: case CP_FA:
        case CP_LS: case CP_PS: case CP_ZWSP: case CP_BOM:
        case CP_INV_SEP: case CP_INV_PLUS: case CP_LRM: case CP_RLM: case CP_SOFT:
            return true;
        default: return false;
    }
}

static char g_errmsg[1024];
static void set_error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(g_errmsg, sizeof(g_errmsg), fmt, ap);
    va_end(ap);
}

typedef struct { char **data; size_t count; size_t cap; } TokenVec;
static void tv_init(TokenVec *v){ v->data=NULL; v->count=0; v->cap=0; }
static void tv_push(TokenVec *v, char *s){
    if(v->count==v->cap){ v->cap=v->cap?v->cap*2:64; v->data=realloc(v->data, v->cap*sizeof(char*)); }
    v->data[v->count++]=s;
}
static void tv_free(TokenVec *v){ for(size_t i=0;i<v->count;i++) free(v->data[i]); free(v->data); v->data=NULL; v->count=v->cap=0; }

typedef struct { uint32_t *data; size_t count; size_t cap; } CpVec;
static void cv_init(CpVec *v){ v->data=NULL; v->count=0; v->cap=0; }
static void cv_push(CpVec *v, uint32_t c){ if(v->count==v->cap){ v->cap=v->cap?v->cap*2:64; v->data=realloc(v->data, v->cap*sizeof(uint32_t)); } v->data[v->count++]=c; }
static void cv_free(CpVec *v){ free(v->data); v->data=NULL; v->count=v->cap=0; }

static bool is_integer_token(const char *tok){
    if(!tok||!*tok) return false;
    size_t i=0;
    if(tok[0]=='-'){ if(!tok[1]) return false; i=1; }
    for(;tok[i];i++) if(!isdigit((unsigned char)tok[i])) return false;
    return true;
}
static bool is_identifier(const char *tok){
    if(!tok||!*tok) return false;
    if(!(isalpha((unsigned char)tok[0])||tok[0]=='_')) return false;
    for(size_t i=1;tok[i];i++){ char c=tok[i]; if(!(isalnum((unsigned char)c)||c=='_'||c=='-')) return false; }
    return true;
}
static const char *KEYWORDS[] = {"word","if","else","let","call","halt","push","dup","swap","drop","rot","neg","sub","mod","id","add","div","sign","mul","pow","num-out","char-out","char-in","rand","sleep","store","load",NULL};
static bool is_keyword(const char *tok){ for(int i=0;KEYWORDS[i];i++) if(strcmp(tok,KEYWORDS[i])==0) return true; return false; }

static bool tokenize(const char *src, size_t len, TokenVec *out){
    tv_init(out);
    size_t i=0;
    while(i < len){
        char c=src[i];
        if(c==' '||c=='\t'||c=='\r'||c=='\n'){ i++; continue; }
        if(c==';'){ while(i<len && src[i]!='\n') i++; continue; }
        if(c=='{'||c=='}'||c=='='){ char *tok=malloc(2); tok[0]=c; tok[1]=0; tv_push(out,tok); i++; continue; }
        if(c=='-' && i+1<len && isdigit((unsigned char)src[i+1])){
            size_t j=i+1; while(j<len && isdigit((unsigned char)src[j])) j++;
            size_t l=j-i; char *tok=malloc(l+1); memcpy(tok,src+i,l); tok[l]=0; tv_push(out,tok); i=j; continue;
        }
        if(isdigit((unsigned char)c)){
            size_t j=i; while(j<len && isdigit((unsigned char)src[j])) j++;
            size_t l=j-i; char *tok=malloc(l+1); memcpy(tok,src+i,l); tok[l]=0; tv_push(out,tok); i=j; continue;
        }
        if(isalpha((unsigned char)c)||c=='_'){
            size_t j=i; while(j<len && (isalnum((unsigned char)src[j])||src[j]=='_'||src[j]=='-')) j++;
            size_t l=j-i; char *tok=malloc(l+1); memcpy(tok,src+i,l); tok[l]=0; tv_push(out,tok); i=j; continue;
        }
        char buf[128];
        snprintf(buf,sizeof(buf),"unexpected character '%c' at position %zu", c, i);
        snprintf(g_errmsg,sizeof(g_errmsg),"%s",buf);
        for(size_t k=0;k<out->count;k++) free(out->data[k]);
        free(out->data);
        out->data=NULL; out->count=out->cap=0;
        return false;
    }
    return true;
}

static void encode_literal(CpVec *out, int64_t value){
    cv_push(out, CP_HAIR);
    char bits[128];
    if(value==0){ bits[0]='0'; bits[1]=0; }
    else {
        char tmp[128]; int t=0; int64_t v=value;
        while(v>0){ tmp[t++]=(v&1)?'1':'0'; v>>=1; }
        for(int i=0;i<t;i++) bits[i]=tmp[t-1-i]; bits[t]=0;
    }
    for(size_t i=0;bits[i];i++) cv_push(out, bits[i]=='1'?CP_DIG1:CP_DIG0);
    cv_push(out, CP_LS);
}

typedef struct { char *name; int idx; } MapEntry;
typedef struct { MapEntry *data; size_t count; size_t cap; } Map;
static void map_init(Map *m){ m->data=NULL; m->count=0; m->cap=0; }
static void map_free(Map *m){ for(size_t i=0;i<m->count;i++) free(m->data[i].name); free(m->data); m->data=NULL; m->count=m->cap=0; }
static void map_push(Map *m, const char *name, int idx){ if(m->count==m->cap){ m->cap=m->cap?m->cap*2:16; m->data=realloc(m->data,m->cap*sizeof(MapEntry)); } m->data[m->count].name=strdup(name); m->data[m->count].idx=idx; m->count++; }
static bool map_find(Map *m, const char *name, int *out_idx){ for(size_t i=0;i<m->count;i++) if(strcmp(m->data[i].name,name)==0){ if(out_idx) *out_idx=m->data[i].idx; return true; } return false; }

static bool mnemonic_to_cps_simple(const char *tok, CpVec *out){
    if(strcmp(tok,"dup")==0){ cv_push(out,CP_ITIMES); return true; }
    if(strcmp(tok,"swap")==0){ cv_push(out,CP_NBSP); return true; }
    if(strcmp(tok,"drop")==0){ cv_push(out,CP_FIGURE); return true; }
    if(strcmp(tok,"rot")==0){ cv_push(out,CP_THIN); return true; }
    if(strcmp(tok,"neg")==0){ cv_push(out,CP_EN); return true; }
    if(strcmp(tok,"sub")==0){ cv_push(out,CP_EN); cv_push(out,CP_EN); return true; }
    if(strcmp(tok,"mod")==0){ cv_push(out,CP_EN); cv_push(out,CP_EN); cv_push(out,CP_EN); return true; }
    if(strcmp(tok,"id")==0){ cv_push(out,CP_EM); return true; }
    if(strcmp(tok,"add")==0){ cv_push(out,CP_EM); cv_push(out,CP_EM); return true; }
    if(strcmp(tok,"div")==0){ cv_push(out,CP_EM); cv_push(out,CP_EM); cv_push(out,CP_EM); return true; }
    if(strcmp(tok,"sign")==0){ cv_push(out,CP_THREE); return true; }
    if(strcmp(tok,"mul")==0){ cv_push(out,CP_THREE); cv_push(out,CP_THREE); return true; }
    if(strcmp(tok,"pow")==0){ cv_push(out,CP_THREE); cv_push(out,CP_THREE); cv_push(out,CP_THREE); return true; }
    if(strcmp(tok,"num-out")==0){ cv_push(out,CP_IDEO); return true; }
    if(strcmp(tok,"char-out")==0){ cv_push(out,CP_PUNCT); return true; }
    if(strcmp(tok,"char-in")==0){ cv_push(out,CP_MEDMATH); return true; }
    if(strcmp(tok,"rand")==0){ cv_push(out,CP_INV_SEP); return true; }
    if(strcmp(tok,"sleep")==0){ cv_push(out,CP_RLM); return true; }
    if(strcmp(tok,"store")==0){ cv_push(out,CP_INV_PLUS); return true; }
    if(strcmp(tok,"load")==0){ cv_push(out,CP_LRM); return true; }
    return false;
}

typedef struct { char **body; size_t body_len; Map *word_map; Map *slot_map; CpVec *word_cps; } EmitCtx;
static bool emit_block(EmitCtx *ctx, size_t *p_ptr){
    size_t p=*p_ptr; char **body=ctx->body; size_t n=ctx->body_len; CpVec *word_cps=ctx->word_cps; Map *word_map=ctx->word_map; Map *slot_map=ctx->slot_map;
    while(p < n){
        const char *tok=body[p];
        if(strcmp(tok,"}")==0){ *p_ptr=p+1; return true; }
        else if(strcmp(tok,"if")==0){
            p+=1;
            if(p>=n||strcmp(body[p],"{")!=0){ set_error("expected '{' after 'if'"); return false; }
            p+=1;
            cv_push(word_cps,CP_WJ); cv_push(word_cps,CP_ZWNJ);
            EmitCtx sub=*ctx; size_t sub_p=p;
            if(!emit_block(&sub,&sub_p)) return false;
            p=sub_p; cv_push(word_cps,CP_ZWJ);
            if(p<n && strcmp(body[p],"else")==0){
                p+=1;
                if(p>=n||strcmp(body[p],"{")!=0){ set_error("expected '{' after 'else'"); return false; }
                p+=1; cv_push(word_cps,CP_ZWNJ);
                EmitCtx sub2=*ctx; size_t sub2_p=p;
                if(!emit_block(&sub2,&sub2_p)) return false;
                p=sub2_p; cv_push(word_cps,CP_ZWJ);
            }
            cv_push(word_cps,CP_LS);
        } else if(strcmp(tok,"else")==0){ set_error("unexpected 'else' without matching 'if'"); return false; }
        else if(strcmp(tok,"let")==0){
            if(p+1>=n){ set_error("let without name"); return false; }
            const char *name=body[p+1]; int addr;
            if(!map_find(slot_map,name,&addr)){ set_error("undefined slot '%s' in let",name); return false; }
            if(p+2>=n||strcmp(body[p+2],"=")!=0){ set_error("let missing '='"); return false; }
            if(p+3>=n||!is_integer_token(body[p+3])){ set_error("let missing integer"); return false; }
            int64_t val=atoll(body[p+3]);
            encode_literal(word_cps,addr);
            if(val<0){ encode_literal(word_cps,-val); cv_push(word_cps,CP_EN); cv_push(word_cps,CP_LS); } else encode_literal(word_cps,val);
            cv_push(word_cps,CP_INV_PLUS); cv_push(word_cps,CP_LS);
            p+=4;
        } else if(strcmp(tok,"push")==0){
            if(p+1>=n||!is_integer_token(body[p+1])){ set_error("push requires integer literal"); return false; }
            int64_t val=atoll(body[p+1]);
            if(val<0){ encode_literal(word_cps,-val); cv_push(word_cps,CP_EN); cv_push(word_cps,CP_LS); } else encode_literal(word_cps,val);
            p+=2;
        } else if(strcmp(tok,"dup")==0||strcmp(tok,"swap")==0||strcmp(tok,"drop")==0||strcmp(tok,"rot")==0||strcmp(tok,"neg")==0||strcmp(tok,"sub")==0||strcmp(tok,"mod")==0||strcmp(tok,"id")==0||strcmp(tok,"add")==0||strcmp(tok,"div")==0||strcmp(tok,"sign")==0||strcmp(tok,"mul")==0||strcmp(tok,"pow")==0||strcmp(tok,"num-out")==0||strcmp(tok,"char-out")==0||strcmp(tok,"char-in")==0||strcmp(tok,"rand")==0||strcmp(tok,"sleep")==0||strcmp(tok,"halt")==0||strcmp(tok,"load")==0||strcmp(tok,"store")==0||strcmp(tok,"call")==0){
            if(strcmp(tok,"halt")==0){ cv_push(word_cps,CP_ZWSP); p+=1; }
            else if(strcmp(tok,"call")==0){
                if(p+1>=n){ set_error("call requires word name or index"); return false; }
                const char *arg=body[p+1];
                if(is_integer_token(arg)){
                    int64_t idx=atoll(arg);
                    if(idx<0){ encode_literal(word_cps,-idx); cv_push(word_cps,CP_EN); cv_push(word_cps,CP_LS); } else encode_literal(word_cps,idx);
                    cv_push(word_cps,CP_FA); cv_push(word_cps,CP_LS); p+=2;
                } else if(is_identifier(arg)){
                    int idx; if(map_find(word_map,arg,&idx)){ encode_literal(word_cps,idx); cv_push(word_cps,CP_FA); cv_push(word_cps,CP_LS); p+=2; }
                    else { set_error("call to undefined word '%s'",arg); return false; }
                } else { set_error("call argument must be word name or integer, got '%s'",arg); return false; }
            } else if(strcmp(tok,"load")==0){
                if(p+1 < n && !is_keyword(body[p+1]) && strcmp(body[p+1],"{")!=0 && strcmp(body[p+1],"}")!=0 && strcmp(body[p+1],"=")!=0 && is_identifier(body[p+1])){
                    const char *name=body[p+1]; int addr; if(map_find(slot_map,name,&addr)){ encode_literal(word_cps,addr); cv_push(word_cps,CP_LRM); cv_push(word_cps,CP_LS); p+=2; } else { set_error("load from undefined slot '%s'",name); return false; }
                } else if(p+1 < n && is_integer_token(body[p+1])){
                    int64_t addr=atoll(body[p+1]); if(addr<0){ encode_literal(word_cps,-addr); cv_push(word_cps,CP_EN); cv_push(word_cps,CP_LS);} else encode_literal(word_cps,addr); cv_push(word_cps,CP_LRM); cv_push(word_cps,CP_LS); p+=2;
                } else { cv_push(word_cps,CP_LRM); cv_push(word_cps,CP_LS); p+=1; }
            } else if(strcmp(tok,"store")==0){
                if(p+1 < n && !is_keyword(body[p+1]) && strcmp(body[p+1],"{")!=0 && strcmp(body[p+1],"}")!=0 && strcmp(body[p+1],"=")!=0 && is_identifier(body[p+1])){
                    const char *name=body[p+1]; int addr; if(map_find(slot_map,name,&addr)){ encode_literal(word_cps,addr); cv_push(word_cps,CP_NBSP); cv_push(word_cps,CP_LS); cv_push(word_cps,CP_INV_PLUS); cv_push(word_cps,CP_LS); p+=2; } else { set_error("store to undefined slot '%s'",name); return false; }
                } else if(p+1 < n && is_integer_token(body[p+1])){
                    int64_t addr=atoll(body[p+1]); if(addr<0){ encode_literal(word_cps,-addr); cv_push(word_cps,CP_EN); cv_push(word_cps,CP_LS);} else encode_literal(word_cps,addr); cv_push(word_cps,CP_NBSP); cv_push(word_cps,CP_LS); cv_push(word_cps,CP_INV_PLUS); cv_push(word_cps,CP_LS); p+=2;
                } else { cv_push(word_cps,CP_INV_PLUS); cv_push(word_cps,CP_LS); p+=1; }
            } else if(strcmp(tok,"id")==0){ p+=1; }
            else { CpVec tmp; cv_init(&tmp); mnemonic_to_cps_simple(tok,&tmp); for(size_t i=0;i<tmp.count;i++) cv_push(word_cps,tmp.data[i]); cv_free(&tmp); cv_push(word_cps,CP_LS); p+=1; }
        } else if(strcmp(tok,"{")==0||strcmp(tok,"}")==0||strcmp(tok,"=")==0){ set_error("unexpected token '%s'",tok); return false; }
        else if(is_integer_token(tok)){ set_error("unexpected integer '%s' (missing 'push'?)",tok); return false; }
        else if(is_identifier(tok)){ set_error("unknown identifier or misplaced token '%s'",tok); return false; }
        else if(strcmp(tok,"word")==0){ set_error("unexpected 'word' inside word body (word definitions must be top-level)"); return false; }
        else { set_error("unexpected token '%s'",tok); return false; }
    }
    *p_ptr=p; return true;
}

static bool assemble(const char *source, size_t src_len, CpVec *out_prog){
    TokenVec tokens;
    if(!tokenize(source,src_len,&tokens)) return false;
    if(tokens.count==0){ set_error("empty source: need at least one word definition"); tv_free(&tokens); return false; }
    Map word_map; map_init(&word_map); Map slot_map; map_init(&slot_map);
    int next_word=0,next_slot=0; size_t i=0;
    while(i<tokens.count){
        const char *tok=tokens.data[i];
        if(strcmp(tok,"word")==0){
            if(i+1>=tokens.count){ set_error("word without name"); tv_free(&tokens); map_free(&word_map); map_free(&slot_map); return false; }
            const char *name=tokens.data[i+1];
            if(is_keyword(name)){ set_error("word name cannot be keyword '%s'",name); tv_free(&tokens); map_free(&word_map); map_free(&slot_map); return false; }
            if(!is_identifier(name)){ set_error("invalid word name '%s'",name); tv_free(&tokens); map_free(&word_map); map_free(&slot_map); return false; }
            int dummy; if(map_find(&word_map,name,&dummy)){ set_error("duplicate word name '%s'",name); tv_free(&tokens); map_free(&word_map); map_free(&slot_map); return false; }
            map_push(&word_map,name,next_word++); i+=2;
        } else if(strcmp(tok,"let")==0){
            if(i+1>=tokens.count){ set_error("let without name"); tv_free(&tokens); map_free(&word_map); map_free(&slot_map); return false; }
            const char *name=tokens.data[i+1];
            if(is_keyword(name)){ set_error("slot name cannot be keyword '%s'",name); tv_free(&tokens); map_free(&word_map); map_free(&slot_map); return false; }
            if(!is_identifier(name)){ set_error("invalid slot name '%s'",name); tv_free(&tokens); map_free(&word_map); map_free(&slot_map); return false; }
            int dummy; if(map_find(&slot_map,name,&dummy)){ set_error("duplicate slot name '%s'",name); tv_free(&tokens); map_free(&word_map); map_free(&slot_map); return false; }
            map_push(&slot_map,name,next_slot++);
            if(i+2>=tokens.count||strcmp(tokens.data[i+2],"=")!=0){ set_error("let %s missing '='",name); tv_free(&tokens); map_free(&word_map); map_free(&slot_map); return false; }
            if(i+3>=tokens.count||!is_integer_token(tokens.data[i+3])){ set_error("let %s missing integer value",name); tv_free(&tokens); map_free(&word_map); map_free(&slot_map); return false; }
            i+=4;
        } else i+=1;
    }
    if(next_word==0){ set_error("program must contain at least one `word` definition (first is entry point)"); tv_free(&tokens); map_free(&word_map); map_free(&slot_map); return false; }
    size_t word_starts_cap=0,word_starts_count=0; size_t *word_starts=NULL;
    for(size_t idx=0;idx<tokens.count;idx++) if(strcmp(tokens.data[idx],"word")==0){ if(word_starts_count==word_starts_cap){ word_starts_cap=word_starts_cap?word_starts_cap*2:8; word_starts=realloc(word_starts,word_starts_cap*sizeof(size_t)); } word_starts[word_starts_count++]=idx; }
    cv_init(out_prog);
    for(size_t wi=0;wi<word_starts_count;wi++){
        size_t start=word_starts[wi]; size_t end=(wi+1<word_starts_count)?word_starts[wi+1]:tokens.count;
        size_t body_len=(end>start+2)?(end-(start+2)):0;
        char **body= body_len? &tokens.data[start+2] : NULL;
        CpVec word_cps; cv_init(&word_cps);
        EmitCtx ctx; ctx.body=body; ctx.body_len=body_len; ctx.word_map=&word_map; ctx.slot_map=&slot_map; ctx.word_cps=&word_cps;
        size_t p=0; if(!emit_block(&ctx,&p)){ cv_free(&word_cps); cv_free(out_prog); free(word_starts); tv_free(&tokens); map_free(&word_map); map_free(&slot_map); return false; }
        for(size_t k=0;k<word_cps.count;k++) cv_push(out_prog,word_cps.data[k]);
        cv_free(&word_cps);
        if(wi!=word_starts_count-1) cv_push(out_prog,CP_PS);
    }
    free(word_starts); tv_free(&tokens); map_free(&word_map); map_free(&slot_map);
    return true;
}

static size_t utf8_encode(uint32_t cp, unsigned char *out){
    if(cp<0x80){ out[0]=(unsigned char)cp; return 1; }
    else if(cp<0x800){ out[0]=(unsigned char)(0xC0|(cp>>6)); out[1]=(unsigned char)(0x80|(cp&0x3F)); return 2; }
    else if(cp<0x10000){ out[0]=(unsigned char)(0xE0|(cp>>12)); out[1]=(unsigned char)(0x80|((cp>>6)&0x3F)); out[2]=(unsigned char)(0x80|(cp&0x3F)); return 3; }
    else { out[0]=(unsigned char)(0xF0|(cp>>18)); out[1]=(unsigned char)(0x80|((cp>>12)&0x3F)); out[2]=(unsigned char)(0x80|((cp>>6)&0x3F)); out[3]=(unsigned char)(0x80|(cp&0x3F)); return 4; }
}

typedef enum { OP_PUSH, OP_DUP, OP_SWAP, OP_DROP, OP_ROT, OP_NEG, OP_SUB, OP_MOD, OP_ID, OP_ADD, OP_DIV, OP_SIGN, OP_MUL, OP_POW, OP_NUMOUT, OP_CHAROUT, OP_CHARIN, OP_CALL, OP_IF, OP_HALT, OP_RAND, OP_STORE, OP_LOAD, OP_SLEEP } OpCode;
typedef struct Block Block;
typedef struct { OpCode op; int64_t value; Block *then_blk; Block *else_blk; } Instr;
struct Block { Instr *items; size_t count; size_t cap; };
static Block* block_new(void){ Block *b=malloc(sizeof(Block)); b->items=NULL; b->count=0; b->cap=0; return b; }
static Instr* block_push_instr(Block *b, OpCode op){ if(b->count==b->cap){ b->cap=b->cap?b->cap*2:8; b->items=realloc(b->items,b->cap*sizeof(Instr)); } Instr *ins=&b->items[b->count++]; memset(ins,0,sizeof(Instr)); ins->op=op; return ins; }
static void block_free(Block *b){ if(!b) return; for(size_t i=0;i<b->count;i++) if(b->items[i].op==OP_IF){ block_free(b->items[i].then_blk); block_free(b->items[i].else_blk); } free(b->items); free(b); }
static OpCode arity_op_cp(uint32_t ch, int count){ if(ch==CP_EN) return count==1?OP_NEG:count==2?OP_SUB:OP_MOD; if(ch==CP_EM) return count==1?OP_ID:count==2?OP_ADD:OP_DIV; return count==1?OP_SIGN:count==2?OP_MUL:OP_POW; }

static uint32_t* utf8_decode(const unsigned char *buf, size_t len, size_t *out_count){
    uint32_t *out=malloc(len*sizeof(uint32_t)); size_t n=0,i=0;
    while(i<len){
        unsigned char b0=buf[i]; uint32_t cp; int extra;
        if(b0<0x80){ cp=b0; extra=0; }
        else if((b0&0xE0)==0xC0){ cp=b0&0x1F; extra=1; }
        else if((b0&0xF0)==0xE0){ cp=b0&0x0F; extra=2; }
        else if((b0&0xF8)==0xF0){ cp=b0&0x07; extra=3; }
        else { out[n++]=0xFFFD; i++; continue; }
        if(i+(size_t)extra>=len){ out[n++]=0xFFFD; i++; continue; }
        bool ok=true; uint32_t val=cp;
        for(int k=1;k<=extra;k++){ unsigned char bk=buf[i+k]; if((bk&0xC0)!=0x80){ ok=false; break; } val=(val<<6)|(bk&0x3F); }
        if(!ok){ out[n++]=0xFFFD; i++; continue; }
        out[n++]=val; i+=(size_t)extra+1;
    }
    *out_count=n; return out;
}

static Block* parse_block_disasm(const uint32_t *cps, size_t len, size_t *pos_ref, bool stop_at_zwj, bool *err){
    Block *blk=block_new(); size_t i=*pos_ref;
    while(i<len){
        uint32_t ch=cps[i];
        if(stop_at_zwj && ch==CP_ZWJ){ i++; *pos_ref=i; return blk; }
        if(ch==CP_LS){ i++; continue; }
        if(ch==CP_ZWSP){ block_push_instr(blk,OP_HALT); i++; continue; }
        if(ch==CP_HAIR){
            i++; int64_t bits=0; size_t nbits=0;
            while(i<len && (cps[i]==CP_DIG0||cps[i]==CP_DIG1)){ bits=bits*2+(cps[i]==CP_DIG1?1:0); nbits++; i++; }
            if(nbits==0){ set_error("empty literal"); *err=true; block_free(blk); return NULL; }
            if(i>=len||(cps[i]!=CP_LS && cps[i]!=CP_SOFT)){ set_error("unterminated literal"); *err=true; block_free(blk); return NULL; }
            bool soft=(cps[i]==CP_SOFT); i++;
            Instr *ins=block_push_instr(blk,OP_PUSH); ins->value=bits;
            if(soft) block_push_instr(blk,OP_CHAROUT);
            continue;
        }
        if(ch==CP_WJ){
            i++; if(i>=len||cps[i]!=CP_ZWNJ){ set_error("expected block open after IF"); *err=true; block_free(blk); return NULL; }
            i++; size_t p_then=i; bool e=false;
            Block *then_blk=parse_block_disasm(cps,len,&p_then,true,&e); if(e){ block_free(blk); *err=true; return NULL; }
            i=p_then; Block *else_blk=NULL;
            if(i<len && cps[i]==CP_ZWNJ){ i++; size_t p_else=i; else_blk=parse_block_disasm(cps,len,&p_else,true,&e); if(e){ block_free(then_blk); block_free(blk); *err=true; return NULL; } i=p_else; }
            Instr *ins=block_push_instr(blk,OP_IF); ins->then_blk=then_blk; ins->else_blk=else_blk; continue;
        }
        if(ch==CP_EN||ch==CP_EM||ch==CP_THREE){
            int count=1; while(i+(size_t)count<len && cps[i+(size_t)count]==ch && count<3) count++;
            OpCode op=arity_op_cp(ch,count); if(op!=OP_ID) block_push_instr(blk,op); i+=(size_t)count; continue;
        }
        if(ch==CP_ITIMES){ block_push_instr(blk,OP_DUP); i++; }
        else if(ch==CP_NBSP){ block_push_instr(blk,OP_SWAP); i++; }
        else if(ch==CP_FIGURE){ block_push_instr(blk,OP_DROP); i++; }
        else if(ch==CP_THIN){ block_push_instr(blk,OP_ROT); i++; }
        else if(ch==CP_IDEO){ block_push_instr(blk,OP_NUMOUT); i++; }
        else if(ch==CP_PUNCT){ block_push_instr(blk,OP_CHAROUT); i++; }
        else if(ch==CP_MEDMATH){ block_push_instr(blk,OP_CHARIN); i++; }
        else if(ch==CP_FA){ block_push_instr(blk,OP_CALL); i++; }
        else if(ch==CP_INV_SEP){ block_push_instr(blk,OP_RAND); i++; }
        else if(ch==CP_INV_PLUS){ block_push_instr(blk,OP_STORE); i++; }
        else if(ch==CP_LRM){ block_push_instr(blk,OP_LOAD); i++; }
        else if(ch==CP_RLM){ block_push_instr(blk,OP_SLEEP); i++; }
        else if(ch==CP_SOFT){ i++; }
        else if(ch==CP_ZWJ||ch==CP_ZWNJ||ch==CP_BOM){ i++; }
        else i++;
    }
    *pos_ref=i; return blk;
}

typedef struct { char *data; size_t len; size_t cap; } SB;
static void sb_init(SB *sb){ sb->cap=8192; sb->len=0; sb->data=malloc(sb->cap); sb->data[0]=0; }
static void sb_append(SB *sb, const char *fmt, ...){
    va_list ap; va_start(ap,fmt);
    int needed=vsnprintf(NULL,0,fmt,ap); va_end(ap);
    if(sb->len + needed + 1 > sb->cap){ while(sb->len+needed+1 > sb->cap) sb->cap*=2; sb->data=realloc(sb->data,sb->cap); }
    va_start(ap,fmt); int w=vsnprintf(sb->data+sb->len, sb->cap - sb->len, fmt, ap); va_end(ap); sb->len+=w;
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

static char* disassemble_bytes(const unsigned char *data, size_t data_len){
    size_t full_len; uint32_t *full=utf8_decode(data,data_len,&full_len);
    size_t bom_idx=(size_t)-1;
    for(size_t idx=0;idx<full_len;idx++) if(full[idx]==CP_BOM){ bom_idx=idx; break; }
    size_t start,end;
    if(bom_idx==(size_t)-1){ start=0; end=full_len; }
    else { start=bom_idx+1; end=full_len; for(size_t idx=start; idx<full_len; idx++) if(full[idx]==CP_ZWSP){ end=idx; break; } }
    uint32_t *prog=malloc((end-start)*sizeof(uint32_t));
    size_t prog_len=0;
    for(size_t i=start;i<end;i++) if(is_spec_cp(full[i])) prog[prog_len++]=full[i];
    free(full);
    if(prog_len==0){ set_error("this document does not contain Invisible Ink"); free(prog); return NULL; }
    size_t word_count=1; for(size_t i=0;i<prog_len;i++) if(prog[i]==CP_PS) word_count++;
    Block **blocks=malloc(word_count*sizeof(Block*));
    size_t seg_start=0, w=0;
    for(size_t i=0;i<=prog_len;i++){
        if(i==prog_len||prog[i]==CP_PS){
            size_t seg_len=i-seg_start; size_t pos=0; bool err=false;
            Block *blk=parse_block_disasm(prog+seg_start, seg_len, &pos, false, &err);
            if(err){ for(size_t k=0;k<w;k++) block_free(blocks[k]); free(blocks); free(prog); return NULL; }
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

static unsigned char* read_file(const char *path, size_t *out_len){
    FILE *f=fopen(path,"rb");
    if(!f){ set_error("%s: %s",path,strerror(errno)); return NULL; }
    fseek(f,0,SEEK_END); long sz=ftell(f); fseek(f,0,SEEK_SET);
    if(sz<0) sz=0;
    unsigned char *buf=malloc((size_t)sz + 1);
    size_t got=fread(buf,1,(size_t)sz,f); fclose(f);
    *out_len=got; return buf;
}

static void print_usage(const char *prog){
    fprintf(stderr,"usage: %s assemble <input> [-o output] | disassemble <input> [-o output]\n",prog);
}

int main(int argc, char **argv){
    if(argc<3){ print_usage(argv[0]); return 2; }
    const char *cmd=argv[1];
    const char *input=NULL;
    const char *output=NULL;
    for(int i=2;i<argc;i++){
        if(strcmp(argv[i],"-o")==0||strcmp(argv[i],"--output")==0){
            if(i+1>=argc){ fprintf(stderr,"error: %s requires argument\n",argv[i]); return 1; }
            output=argv[i+1]; i++;
        } else if(!input){ input=argv[i]; }
        else { fprintf(stderr,"error: unexpected argument '%s'\n",argv[i]); return 1; }
    }
    if(!input){ fprintf(stderr,"error: missing input file\n"); print_usage(argv[0]); return 2; }

    if(strcmp(cmd,"assemble")==0){
        size_t len; unsigned char *data=read_file(input,&len);
        if(!data){ fprintf(stderr,"error: %s\n",g_errmsg); return 1; }
        char *src=malloc(len+1); memcpy(src,data,len); src[len]=0; free(data);
        CpVec prog; 
        if(!assemble(src,len,&prog)){
            fprintf(stderr,"error: %s\n",g_errmsg);
            free(src);
            return 1;
        }
        free(src);
        size_t max_bytes=prog.count*4 + 4;
        unsigned char *outbuf=malloc(max_bytes);
        size_t outpos=0;
        for(size_t i=0;i<prog.count;i++){
            unsigned char tmp[4]; size_t el=utf8_encode(prog.data[i],tmp);
            memcpy(outbuf+outpos,tmp,el); outpos+=el;
        }
        cv_free(&prog);
        if(output){
            FILE *f=fopen(output,"wb"); if(!f){ fprintf(stderr,"error: %s: %s\n",output,strerror(errno)); free(outbuf); return 1; }
            fwrite(outbuf,1,outpos,f); fclose(f);
        } else {
            fwrite(outbuf,1,outpos,stdout);
        }
        free(outbuf);
        return 0;
    } else if(strcmp(cmd,"disassemble")==0){
        size_t len; unsigned char *data=read_file(input,&len);
        if(!data){ fprintf(stderr,"error: %s\n",g_errmsg); return 1; }
        char *qsrc=disassemble_bytes(data,len);
        free(data);
        if(!qsrc){ fprintf(stderr,"error: %s\n",g_errmsg); return 1; }
        if(output){
            FILE *f=fopen(output,"w"); if(!f){ fprintf(stderr,"error: %s: %s\n",output,strerror(errno)); free(qsrc); return 1; }
            fputs(qsrc,f); fclose(f);
        } else {
            fputs(qsrc,stdout);
        }
        free(qsrc);
        return 0;
    } else {
        fprintf(stderr,"error: unknown command '%s' (expected assemble or disassemble)\n",cmd);
        print_usage(argv[0]); return 2;
    }
}

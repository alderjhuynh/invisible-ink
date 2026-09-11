#include "ink.h"
#include "vm.h"
#include "cli.h"
#include "editor.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv) {
    const char *prog = argv[0];

    if(argc<2){
        print_global_usage(prog);
        return 2;
    }
    
    if(strcmp(argv[1],"-h")==0||strcmp(argv[1],"--help")==0||strcmp(argv[1],"help")==0){
        if(argc>=3){
            const char *sub=argv[2];
            if(strcmp(sub,"run")==0) print_help_run(prog);
            else if(strcmp(sub,"dump")==0) print_help_dump(prog);
            else if(strcmp(sub,"disassemble")==0||strcmp(sub,"disasm")==0) print_help_dump(prog);
            else if(strcmp(sub,"assemble")==0||strcmp(sub,"asm")==0||strcmp(sub,"compile")==0||strcmp(sub,"build")==0) print_help_assemble(prog);
            else if(strcmp(sub,"embed")==0||strcmp(sub,"em")==0) print_help_embed(prog);
            else if(strcmp(sub,"edit")==0||strcmp(sub,"ide")==0||strcmp(sub,"ed")==0) print_help_edit(prog);
            else if(strcmp(sub,"check")==0) print_help_check(prog);
            else if(strcmp(sub,"version")==0) fprintf(stderr,"ink %s\n",INK_VERSION);
            else { fprintf(stderr,"unknown help topic '%s'\n", sub); print_global_usage(prog); return 2; }
            return 0;
        }
        print_global_usage(prog);
        return 0;
    }
    if(strcmp(argv[1],"-V")==0||strcmp(argv[1],"--version")==0||strcmp(argv[1],"version")==0){
        printf("ink %s\n", INK_VERSION);
        return 0;
    }

    if(strcmp(argv[1],"-d")==0 && argc==3){
        return do_dump(argv[2]);
    }
    
    const char *known[] = {"run","r","check","dump","disassemble","disasm","decompile","d","assemble","asm","compile","build","c","embed","em","edit","ide","ed","help","version",NULL};
    bool is_known=false;
    for(int i=0;known[i];i++) if(strcmp(argv[1],known[i])==0){ is_known=true; break; }
    if(!is_known){
        if(access(argv[1], F_OK)==0){
            g_max_steps = DEFAULT_MAX_STEPS; g_max_depth = DEFAULT_MAX_DEPTH;
            bool stats=false;
            for(int i=2;i<argc;i++){
                if(strcmp(argv[i],"--stats")==0) stats=true;
                else if((strcmp(argv[i],"--steps")==0||strcmp(argv[i],"--max-steps")==0) && i+1<argc){ g_max_steps=strtoul(argv[++i],NULL,10); }
                else if((strcmp(argv[i],"--depth")==0||strcmp(argv[i],"--max-depth")==0) && i+1<argc){ g_max_depth=atoi(argv[++i]); }
            }
            return do_run(argv[1], stats);
        } else {
            if(argv[1][0]=='-'){
                fprintf(stderr, "unknown option '%s'\n", argv[1]);
                print_global_usage(prog);
                return 2;
            }
            fprintf(stderr, "unknown command '%s'\n", argv[1]);
            print_global_usage(prog);
            return 2;
        }
    }

    const char *cmd = argv[1];

    if(strcmp(cmd,"run")==0||strcmp(cmd,"r")==0){
        const char *file=NULL;
        bool stats=false;
        g_max_steps = DEFAULT_MAX_STEPS; g_max_depth = DEFAULT_MAX_DEPTH;
        for(int i=2;i<argc;i++){
            if(strcmp(argv[i],"-h")==0||strcmp(argv[i],"--help")==0){ print_help_run(prog); return 0; }
            else if(strcmp(argv[i],"--stats")==0) stats=true;
            else if((strcmp(argv[i],"--steps")==0||strcmp(argv[i],"--max-steps")==0)){
                if(i+1>=argc){ fprintf(stderr,"run: %s requires argument\n", argv[i]); return 2; }
                g_max_steps=strtoul(argv[++i],NULL,10);
            } else if((strcmp(argv[i],"--depth")==0||strcmp(argv[i],"--max-depth")==0)){
                if(i+1>=argc){ fprintf(stderr,"run: %s requires argument\n", argv[i]); return 2; }
                g_max_depth=atoi(argv[++i]);
            } else if(argv[i][0]=='-' ){
                fprintf(stderr,"run: unknown option '%s'\n", argv[i]); print_help_run(prog); return 2;
            } else if(!file) file=argv[i];
            else { fprintf(stderr,"run: unexpected argument '%s'\n", argv[i]); print_help_run(prog); return 2; }
        }
        if(!file){ fprintf(stderr,"run: missing <file>\n"); print_help_run(prog); return 2; }
        return do_run(file, stats);
    }

    if(strcmp(cmd,"dump")==0){
        const char *file=NULL;
        for(int i=2;i<argc;i++){
            if(strcmp(argv[i],"-h")==0||strcmp(argv[i],"--help")==0){ print_help_dump(prog); return 0; }
            else if(!file) file=argv[i];
            else { fprintf(stderr,"dump: unexpected argument '%s'\n", argv[i]); return 2; }
        }
        if(!file){ fprintf(stderr,"dump: missing <file>\n"); print_help_dump(prog); return 2; }
        return do_dump(file);
    }

    if(strcmp(cmd,"disassemble")==0||strcmp(cmd,"disasm")==0||strcmp(cmd,"decompile")==0||strcmp(cmd,"d")==0){
        const char *input=NULL; const char *output=NULL;
        for(int i=2;i<argc;i++){
            if(strcmp(argv[i],"-h")==0||strcmp(argv[i],"--help")==0){ print_help_dump(prog); return 0; }
            else if(strcmp(argv[i],"-o")==0||strcmp(argv[i],"--output")==0){
                if(i+1>=argc){ fprintf(stderr,"disassemble: %s requires argument\n", argv[i]); return 2; }
                output=argv[++i];
            } else if(argv[i][0]=='-' ){
                fprintf(stderr,"disassemble: unknown option '%s'\n", argv[i]); return 2;
            } else if(!input) input=argv[i];
            else { fprintf(stderr,"disassemble: unexpected argument '%s'\n", argv[i]); return 2; }
        }
        if(!input){ fprintf(stderr,"disassemble: missing <file>\n"); print_help_dump(prog); return 2; }
        return do_disassemble(input, output);
    }

    if(strcmp(cmd,"assemble")==0||strcmp(cmd,"asm")==0||strcmp(cmd,"compile")==0||strcmp(cmd,"build")==0||strcmp(cmd,"c")==0){
        const char *input=NULL; const char *output=NULL;
        for(int i=2;i<argc;i++){
            if(strcmp(argv[i],"-h")==0||strcmp(argv[i],"--help")==0){ print_help_assemble(prog); return 0; }
            else if(strcmp(argv[i],"-o")==0||strcmp(argv[i],"--output")==0){
                if(i+1>=argc){ fprintf(stderr,"assemble: %s requires argument\n", argv[i]); return 2; }
                output=argv[++i];
            } else if(argv[i][0]=='-' ){
                fprintf(stderr,"assemble: unknown option '%s'\n", argv[i]); return 2;
            } else if(!input) input=argv[i];
            else { fprintf(stderr,"assemble: unexpected argument '%s'\n", argv[i]); return 2; }
        }
        if(!input){ fprintf(stderr,"assemble: missing <input>\n"); print_help_assemble(prog); return 2; }
        return do_assemble(input, output);
    }

    if(strcmp(cmd,"check")==0){
        const char *file=NULL;
        for(int i=2;i<argc;i++){
            if(strcmp(argv[i],"-h")==0||strcmp(argv[i],"--help")==0){ print_help_check(prog); return 0; }
            else if(!file) file=argv[i];
            else { fprintf(stderr,"check: unexpected argument '%s'\n", argv[i]); return 2; }
        }
        if(!file){ fprintf(stderr,"check: missing <file>\n"); print_help_check(prog); return 2; }
        return do_check(file);
    }

    if(strcmp(cmd,"embed")==0||strcmp(cmd,"em")==0){
        const char *input=NULL; const char *output=NULL; const char *prose_file=NULL; const char *prose_text=NULL;
        bool wrap=false; long interval=0;
        for(int i=2;i<argc;i++){
            if(strcmp(argv[i],"-h")==0||strcmp(argv[i],"--help")==0){ print_help_embed(prog); return 0; }
            else if(strcmp(argv[i],"-o")==0||strcmp(argv[i],"--output")==0){
                if(i+1>=argc){ fprintf(stderr,"embed: %s requires argument\n", argv[i]); return 2; }
                output=argv[++i];
            } else if(strcmp(argv[i],"-p")==0||strcmp(argv[i],"--prose")==0){
                if(i+1>=argc){ fprintf(stderr,"embed: %s requires argument\n", argv[i]); return 2; }
                prose_file=argv[++i];
            } else if(strcmp(argv[i],"--text")==0){
                if(i+1>=argc){ fprintf(stderr,"embed: --text requires argument\n"); return 2; }
                prose_text=argv[++i];
            } else if(strcmp(argv[i],"--wrap")==0){
                wrap=true;
            } else if(strcmp(argv[i],"--interval")==0){
                if(i+1>=argc){ fprintf(stderr,"embed: --interval requires argument\n"); return 2; }
                interval=atol(argv[++i]);
            } else if(argv[i][0]=='-'){
                fprintf(stderr,"embed: unknown option '%s'\n", argv[i]); print_help_embed(prog); return 2;
            } else if(!input) input=argv[i];
            else if(!output) output=argv[i];
            else { fprintf(stderr,"embed: unexpected argument '%s'\n", argv[i]); print_help_embed(prog); return 2; }
        }
        if(!input){ fprintf(stderr,"embed: missing <input.q>\n"); print_help_embed(prog); return 2; }
        return do_embed(input, output, prose_file, prose_text, wrap, interval);
    }

    if(strcmp(cmd,"edit")==0||strcmp(cmd,"ide")==0||strcmp(cmd,"ed")==0){
        return do_edit(argc, argv);
    }

    if(strcmp(cmd,"help")==0){
        if(argc>=3){
            const char *sub=argv[2];
            if(strcmp(sub,"run")==0) print_help_run(prog);
            else if(strcmp(sub,"dump")==0) print_help_dump(prog);
            else if(strcmp(sub,"disassemble")==0||strcmp(sub,"disasm")==0) print_help_dump(prog);
            else if(strcmp(sub,"assemble")==0||strcmp(sub,"asm")==0) print_help_assemble(prog);
            else if(strcmp(sub,"embed")==0||strcmp(sub,"em")==0) print_help_embed(prog);
            else if(strcmp(sub,"edit")==0||strcmp(sub,"ide")==0) print_help_edit(prog);
            else if(strcmp(sub,"check")==0) print_help_check(prog);
            else { fprintf(stderr,"unknown help topic '%s'\n", sub); return 2; }
            return 0;
        }
        print_global_usage(prog);
        return 0;
    }

    fprintf(stderr,"unknown command '%s'\n", cmd);
    print_global_usage(prog);
    return 2;
}

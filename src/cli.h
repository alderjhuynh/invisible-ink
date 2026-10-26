#ifndef CLI_H
#define CLI_H

#include <stdbool.h>

void print_global_usage(const char *prog);
void print_help_run(const char *prog);
void print_help_dump(const char *prog);
void print_help_assemble(const char *prog);
void print_help_edit(const char *prog);
void print_help_check(const char *prog);
void print_help_embed(const char *prog);
void print_help_scc(const char *prog);

int scc_main(int argc, char **argv);

int do_run(const char *path, bool do_stats);
int do_dump(const char *path);
int do_disassemble(const char *input, const char *output);
int do_assemble(const char *input, const char *output);
int do_check(const char *path);
int do_embed(const char *input, const char *output, const char *prose_file, const char *prose_text, bool wrap, long interval);

#endif

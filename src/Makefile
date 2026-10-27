CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -O2
CFLAGS += -D_XOPEN_SOURCE=700

SRCS := util.c parse.c vm.c obj.c progload.c vmhost.c sched.c quill.c editor.c cli.c main.c scc.c sdec.c
OBJS := $(SRCS:.c=.o)

all: ink

ink: $(OBJS)
	$(CC) $(CFLAGS) -o ink $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Conformance suite: every ISA semantics decision the OS will depend on.
test: ink
	../tests/run.sh ../src/ink

# LibFuzzer harnesses (clang only: needs -fsanitize=fuzzer).
#   make fuzz CC=clang CFLAGS="-std=c11 -g -O1 -fsanitize=fuzzer,address,undefined"
# Without clang, build the standalone drivers (they take corpus files as args):
#   make fuzz CC=gcc CFLAGS="-std=c11 -g -O1 -fsanitize=address,undefined -DFUZZ_STANDALONE"
FUZZ_SRCS := util.c parse.c obj.c progload.c vm.c vmhost.c
fuzz: fuzz_parse fuzz_vm

fuzz_parse: fuzz_parse.c $(FUZZ_SRCS:.c=.o)
	$(CC) $(CFLAGS) -DFUZZ_STANDALONE -o fuzz_parse fuzz_parse.c $(FUZZ_SRCS:.c=.o)

fuzz_vm: fuzz_vm.c $(FUZZ_SRCS:.c=.o)
	$(CC) $(CFLAGS) -DFUZZ_STANDALONE -o fuzz_vm fuzz_vm.c $(FUZZ_SRCS:.c=.o)

clean:
	rm -f ink $(OBJS)

.PHONY: all clean test fuzz fuzz_parse fuzz_vm

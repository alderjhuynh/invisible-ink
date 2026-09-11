CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -O2
CFLAGS += -D_POSIX_C_SOURCE=200809L

SRCS := util.c parse.c vm.c quill.c editor.c cli.c main.c
OBJS := $(SRCS:.c=.o)

all: ink

ink: $(OBJS)
	$(CC) $(CFLAGS) -o ink $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f ink $(OBJS)

.PHONY: all clean

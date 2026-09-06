CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -O2
CFLAGS += -D_POSIX_C_SOURCE=200809L

all: ink quill

ink: invisible-ink.c
	$(CC) $(CFLAGS) -o ink invisible-ink.c

quill: quill.c
	$(CC) $(CFLAGS) -o quill quill.c

.PHONY: clean
clean:
	rm -f ink quill

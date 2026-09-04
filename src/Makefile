CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -O2
CFLAGS += -D_POSIX_C_SOURCE=200809L

ink: invisible-ink.c
	$(CC) $(CFLAGS) -o ink invisible-ink.c

.PHONY: clean
clean:
	rm -f ink

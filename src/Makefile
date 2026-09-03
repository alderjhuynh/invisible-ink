CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -O2

ink: invisible-ink.c
	$(CC) $(CFLAGS) -o ink invisible-ink.c

.PHONY: clean
clean:
	rm -f ink

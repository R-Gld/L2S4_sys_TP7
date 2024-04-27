CC = gcc
CFLAGS = -Wall -pedantic -std=c11
SOURCES = $(wildcard src/*.c)
OUTDIR = execs
EXECUTABLES = $(patsubst src/%,$(OUTDIR)/%,$(SOURCES:%.c=%))

all: $(OUTDIR) $(EXECUTABLES)

$(OUTDIR):
	mkdir -p $(OUTDIR)

$(OUTDIR)/%: src/%.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(EXECUTABLES)

.PHONY: all clean
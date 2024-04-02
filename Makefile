CC = gcc
CFLAGS = -Wall -pedantic -std=c11
SOURCES = $(wildcard src/*.c)
OUTDIR = execs
EXECUTABLES = $(patsubst src/%,$(OUTDIR)/%,$(SOURCES:%.c=%))

all: $(EXECUTABLES)

$(OUTDIR)/%: src/%.c
	mkdir -p $(OUTDIR)
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(EXECUTABLES)
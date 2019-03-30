.PHONY: all bin clean

CC = i686-elf-gcc
LD = i686-elf-ld
CFLAGS = -Wall -Wextra -O0
SOURCES=$(wildcard src/*.c)
OBJECTS=$(addprefix bin/,$(notdir $(SOURCES:.c=.o)))

all: crt0 $(OBJECTS)

crt0: util/crt0.S | bin
	$(CC) -c $+ -o bin/crt0.o

bin/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@_tmp $<
	$(LD) -o $@ bin/crt0.o $@_tmp -lk -L util
	@rm -f $@_tmp

bin:
	@mkdir -p $@

clean:
	rm -rf bin

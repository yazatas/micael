#!/bin/bash

if [[ "$#" -eq 2 ]]; then
	i686-elf-gcc -c $1 -o "$1.o"
	i686-elf-gcc -c crt0.S -o crt0.o
	i686-elf-ld -o $2 crt0.o "$1.o" -lk -L .
	rm -f crt0.o "$1.o"
else
	echo "usage: ./make_program.sh <source.c> <output>"
fi

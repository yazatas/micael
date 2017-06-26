# micael

micael is a 32-bit monolithic kernel written in C and x86 assembly.

You'll need a [cross-compiler](http://wiki.osdev.org/GCC_Cross-Compiler) to compile micael on your system (don't worry, it's easy to build). In case you just want to run micael, you'll need QEMU.

To run micael in QEMU, type: `qemu-system-i386 -kernel kernel.elf`

# how-to
## Compile boot.s
`<name-of-your-cc> -std=gnu99 -ffreestanding -g -c boot.s -o boot.o`

## Compile kernel.c
`<name-of-your-cc> -std=gnu99 -ffreestanding -g -c kernel.c -o kernel.o`

## Linking everything together
`<name-of-your-cc> -ffreestanding -nostdlib -g -T linker.ld boot.o kernel.o -o kernel.elf -lgcc`


# Copying
micael is free software. It's licensed under the MIT license.

# micael

## ToC
1) Overview
2) Folder structure
3) Build system
4) Documentation

## Overview

micael is a monolithic kernel written in C99 and x86 assembly. It is a hobby operating system I'm writing on my free time so development is quite slow. Eventually micael shall have user mode, 3D rendering and a way to get to the internet.

I've been pondering the idea to port other programs to micael but OTOH writing a completely independent system is an intriguing idea.


## Folder structure

micael's consists of two major folders: kernel and libc (ie. kernel code and C standard library)


Here's the folder structure:

* libc
   * ctype
   * stdio
   * stdlib
   * string
   * time
   * include
* kernel
   * algorithm
   * arch
      * i386
   * drivers
   * kernel
   * include


### libc
Right now libc is embryonic. Only functions kernel uses from it are memcpy and memset and user mode is still far away so there's no need to invest in it at the moment.

Every header has its own directory and there's a source file for every function (so stdio directory contains files printf.c, putchar.c etc.)

### kernel
Kernel has 4 major source directories: algorithm, arch/i386, drivers and kernel


#### algorithm
This directory contains all algorithm related stuff so red-black tree, bitvector, queue and stuff like this are to be found from here.

#### arch/i386
All architecture related stuff are in this folder. Basic rule of thumb is that if your code has inline assembly, place the code here.
It's not the most optimal way of handling platform support and most likely will go through an overhaul if any other platform is supported in the future.

You can find, for example, memory manager and interrupt handlers from this directory.

#### drivers
As the name suggests this directory contains drivers. 

#### kernel
This directory contains all non-architecture related (so it should contain most of the stuff when proper architecture abstraction is added). You can find files like kmain.c and kprint.c from this directory

## Build system

You need to have a cross-compiler to compile the system. Building a cross compiler is very easy and OsDev has a great tutorial (https://wiki.osdev.org/GCC_Cross-Compiler)

You'll also need an i386 QEMU to run the system. Arch Linux Wiki has a great article about QEMU (https://wiki.archlinux.org/index.php/QEMU#Installation)

When you have a cross-compiler and QEMU ready, micael is very easy to build. All you need to do is run `make` and a micael.iso image is generated. You can run `make all run` to build and run the system

## Documentation

All documentation is located in doc folder. The documentation mightn't be up to date, so one should not rely on it completely. I'll try to keep it up to date but sometimes writing code wins writing documentation.

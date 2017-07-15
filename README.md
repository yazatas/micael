# micael

micael is a 32-bit monolithic kernel written in C and x86 assembly.

I'm currently implementing libc so there isn't going to be a lot of kernel functionality available for testing. I won't be implementing the whole library, only the most crucial parts (errno, std{io,lib}, time, string, ctype, signal)
None of the functions in libc have been tested unless explicitly stated in "What is ready" section down below.

Following headers are provided in a freestanding environment by GCC:

* float.h
* iso646.h
* limits.h
* stdarg.h
* stdbool.h
* stddef.h
* stdint.h


You'll need QEMU and a [cross-compiler](http://wiki.osdev.org/GCC_Cross-Compiler) to compile and run micael;

### What is ready:
* string.h (not tested)
* ctype.h

### What is not ready
* libc:
  * errno.h
  * stdio.h
  * stdlib.h
  * time.h
  * signal.h
  * TESTING!
* kernel:
  * everything

# Compiling and running

Run following commands to compile and run micael:

`./build.sh; ./run`

# Copying
micael is free software. It's licensed under the MIT license.

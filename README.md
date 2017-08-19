# micael

micael is a 32-bit monolithic kernel written in C and x86 assembly.

Currently I'm in the process of implementing virtual memory. I decided to give libc a rest as it's not going to be needed for a while. 

You'll need QEMU and a [cross-compiler](http://wiki.osdev.org/GCC_Cross-Compiler) to compile and run micael

### What is ready:
* libc
  * string.h (needs testing)
  * ctype.h
* kernel
  * global descriptor table
  * interrupts (ISRs and IRQs)
  * paging

### What is not ready
* libc:
  * errno.h
  * stdio.h
  * stdlib.h
  * time.h
  * signal.h
* kernel:
  * virtual memory
  * kernel multitasking
  * process management
  * everything else

# Compiling and running

Run following commands to compile and run micael:

`make all run`

Using make to run an application is unconventional but I think it's better than a script.
All on all, make is better approach than sh build scripts.

# Copying
micael is free software. It's licensed under the MIT license.

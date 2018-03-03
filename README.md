# micael

micael is a 32-bit monolithic kernel written in C and x86 assembly.

I've started to implement kernel multitasking and right now a very basic cooperative multitasking model is provided but you can't do much with it yet. I've started to implement scheduling and mutual exclusions but they may take a while.

After I have a working file system I'll ditch grub and write my own boot loader because of dkmFS's special needs.


### What is ready:
* libc
  * string.h (needs testing)
  * ctype.h
* kernel
   * Memory Management
      * paging
      * kernel heap
   * Multitasking
      * cooperative multitasking
   * Miscellaneous
      * global descriptor table
      * interrupts (ISRs and IRQs)

### What is not ready aka TODO
* libc:
  * errno.h
  * stdio.h
  * stdlib.h
  * time.h   (needs kernel(?))
  * signal.h (needs kernel)
* kernel:
   * Multitasking
      * preemptive multitasking
      * mutual exclusion using spinlocks
   * Memory Management
      * switch from linked lists to red-black tree
   * File System
      * working fs
      * paging to disk (what's the proper term?)
      * boot loader
   * Miscellaneous
      * move all code containing assembly to kernel/arch/i386
      * proper vga driver
      * tcp/ip stack


# Copying
micael is free software. It's licensed under the MIT license.

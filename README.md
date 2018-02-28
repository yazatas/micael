# micael

micael is a 32-bit monolithic kernel written in C and x86 assembly.

* todo add short overview
* todo move all development discussion to doc
* todo move all subsystem related stuff to doc
* todo add more info about mmu
* todo create helpers.s assembly file
* move all code containing assembly to kernel/arch/i386

I've started to implement kernel multitasking and right now a very basic cooperative multitasking model is provided but you can't do much with it yet. I've started to implement scheduling and mutual exclusions but they may take a while.

After I have a working file system I'll ditch grub and write my own boot loader because of dkmFS's special needs.

# Kernel memory layout

todo move all this to doc

Kernel is loaded to physical address 0x00100000 and virtual address 0xc0100000.

| start address | reserved for |
| --------------| :------------|
| 0xc0000000    | kernel code, data, bss |
| 0xd0000000    | kernel heap |
| 0xe0000000    | free page frames for miscellaneous use |

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
      * proper vga driver
      * tcp/ip stack


# Compiling and running

todo move all this doc

You'll need QEMU and a [cross-compiler](http://wiki.osdev.org/GCC_Cross-Compiler) to compile and run micael

Run following commands to compile and run micael:

`make all run`

Using make to run an application is unconventional but I think it's better than a script.
All and all, make is better approach than sh build scripts IMO.

# Copying
micael is free software. It's licensed under the MIT license.

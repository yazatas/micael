# micael

micael is a 32-bit monolithic kernel written in C and x86 assembly.

Currently I'm in the process of implementing virtual memory. I decided to give libc a rest as it's not going to be needed for a while. 

After I have a working file system I'll ditch grub and write my own boot loader because of dkmFS's special needs.

You'll need QEMU and a [cross-compiler](http://wiki.osdev.org/GCC_Cross-Compiler) to compile and run micael

# Kernel memory layout

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
  * global descriptor table
  * interrupts (ISRs and IRQs)
  * mmu
      * (demand) paging
      * kernel heap

### What is not ready aka TODO
* libc:
  * errno.h
  * stdio.h
  * stdlib.h
  * time.h
  * signal.h
* kernel:
  * vga driver
  * process management and scheduling
  * file system
      * paging to disk (what's the proper term?)
      * boot loader
  * mmu
      * switch from linked lists to red-black tree
  * tcp/ip stack


# Compiling and running

Run following commands to compile and run micael:

`make all run`

Using make to run an application is unconventional but I think it's better than a script.
All on all, make is better approach than sh build scripts IMO.

# Copying
micael is free software. It's licensed under the MIT license.

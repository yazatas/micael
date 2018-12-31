# micael

micael is a 32-bit monolithic kernel written in C and x86 assembly.

See doc/OVERVIEW.md for more details

# What is ready:
* libc
  * string.h (needs testing)
  * ctype.h
* kernel
   * Memory Management
      * Paging
      * Kernel heap
	  * Page cache
   * Multitasking
      * Preemptive multitasking
   * File System
      * Virtual File System (WIP)
	  * Initrd

### What is not ready aka TODO
* libc:
  * errno.h
  * stdio.h
  * stdlib.h
  * time.h   (needs kernel)
  * signal.h (needs kernel)
  * Unit tests for libc functions
* kernel:
   * Multitasking
      * semaphores
      * switch from linked list to priority-based queue
      * protect all critical parts with mutexes
   * Memory Management
      * switch from linked lists to red-black tree
	  * address space layout randomization
      * Copy-on-Write forking
   * File System
      * Working file system
      * Boot loader
   * Miscellaneous
	  * buffered i/o
      * tcp/ip stack
      * support for ARM architecture
         * remove arch-specific code from vmm.c
         * move all code containing assembly to kernel/arch/i386
      * core utils
      * VGA driver
	  * POSIX compliance
      * multicore support
         * spinlocks
      * add submodules
         * core utils
         * shell
         * file system
      * create installation scripts for qemu and cross-compiler

# Copying
micael is free software. It's licensed under the MIT license.

# micael

micael is a 32-bit monolithic kernel written in C and x86 assembly.

See doc/OVERVIEW.md for more details

# What is ready:
* libc
  * string.h (needs testing)
  * ctype.h
* kernel
   * Memory Management
      * paging
      * kernel heap
   * Multitasking
      * cooperative multitasking (legacy)
      * mutual exclusions
      * User mode processes
	  * System calls
   * Miscellaneous
      * global descriptor table with TSS
      * interrupts (ISRs and IRQs)

### Working on
* libc
   * stdio.h
   * syscall support
* kernel
   * Multitasking
	  * preemptive multitasking
   * Memory Management
      * create function for address space duplication (fork)
   * File System
	  * VFS
   * Miscellaneous
      * core utils
	  * POSIX compliance

### What is not ready aka TODO
* libc:
  * errno.h
  * stdio.h
  * stdlib.h
  * time.h   (needs kernel)
  * signal.h (needs kernel)
* kernel:
   * Multitasking
	  * preemptive multitasking
      * semaphores
      * switch from linked list to priority-based queue
      * protect all critical parts with mutexes
   * Memory Management
      * switch from linked lists to red-black tree
	  * address space layout randomization
   * File System
	  * VFS
      * Working file system
      * boot loader
      * paging to disk (what's the proper term?)
   * Miscellaneous
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

# micael

micael is a 32-bit monolithic kernel written in C and x86 assembly.

See doc/OVERVIEW.md for more details

Currently I'm rewriting all tasking-related code. It's so messy it's hard to update or extend so I think the best thing is to just rewrite the whole thing. That includes also major MMU updates

Once MMU is bug-free, I'll implement the VFS layer so that reading data from storage (be it initrd or disk) is sane. After that's done I'll refocus my attention to tasking and this time hopefully it works. No more "temporary solutions" and "Proof-of-Concepts"

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
      * user mode
	  * system calls
         * write, fork, execv
   * Miscellaneous
      * global descriptor table with TSS
      * interrupts (ISRs and IRQs)

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
	  * preemptive multitasking
      * semaphores
      * switch from linked list to priority-based queue
      * protect all critical parts with mutexes
   * Memory Management
      * switch from linked lists to red-black tree
	  * address space layout randomization
      * Copy-on-Write forking
   * File System
	  * VFS
      * Working file system
      * boot loader
      * page cache
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

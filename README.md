# micael

micael is a 32-bit monolithic kernel written in C and x86 assembly.

If you're missing a cross-compiler or the correct emulator, check the tools directory. It has scripts for automatic installation of needed software.

### What is ready:
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
      * calculate available pages
      * rewrite vmm_kalloc_frame()
      * relocate kernel stack to upper 3GB area (??)
      * remove arch-specific code from vmm.c
      * refactor vmm_init
      * create functions for page table/directory allocation
	  * address space layout randomization
   * File System
	  * VFS
      * Working file system
      * boot loader
      * paging to disk (what's the proper term?)
   * Miscellaneous
      * move all code containing assembly to kernel/arch/i386
      * tcp/ip stack
      * support for ARM architecture
      * core utils
      * VGA driver
      * multicore support
         * spinlocks
      * add submodules
         * core utils
         * shell
         * file system


# Copying
micael is free software. It's licensed under the MIT license.

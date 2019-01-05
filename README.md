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
      * Kernel Heap
	  * Page Cache
   * Multitasking
      * Preemptive Multitasking
	  * User Mode
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
	  * Implement priority-based scheduling policy
	  * Symmetric Multiprocessing (SMP)
   * Memory Management
	  * Implement better scheme for address space/memory management
   * File System
      * Boot loader
	  * Port dkmFS
      * Pseudo filesystems
         * devfs
         * tmpfs
         * procfs
         * sysfs
   * Drivers
      * BGA/VGA
	  * PCI(-e)
	  * SATA
	  * SCSI?
	  * Generic USB drivers
      * PC speaker driver
   * Networking
	  * Ethernet, IP, TCP/UDP
   * Miscellaneous
      * 64-bit support

# Copying
micael is free software. It's licensed under the MIT license.

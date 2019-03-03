# micael

micael is a 32-bit monolithic kernel written in C and x86 assembly.

See doc/OVERVIEW.md and doc/ROADMAP.md for more details

# Features:
* Memory Management
  * Paging
  * Kernel Heap
  * Page Cache
* Multitasking
  * Preemptive Multitasking
  * User Mode
  * POSIX-compliant system call interface:
	 * execv
	 * fork
	 * exit
* File System
  * Virtual File System
     * devfs
	 * initramfs
* Drivers
  * VESA VBE 3.0

# Copying
micael is free software. It's licensed under the MIT license.

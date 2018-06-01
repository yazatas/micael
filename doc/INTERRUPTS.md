# Interrupts in micael

micael has both software and hardware interrupts. For now only kernel uses interrupts but in the future user mode programs can raise interrupts (indirectly) by issuing system calls.

ISRs 0-16 are reserved for OS (division by zero, page fault, double fault etc.), 17-31 are reserved for the processor and interrupts from 32 to 47 are used for interrupt requests (PIT, keyboard etc).

The ISR 128 (0x80) is reserved for user mode interrupts used for system calls.

Currently micael has three specific handlers:
   * General Protection Fault
   * Page Fault
   * System calls

GPF handler prints a more details about the error condition. It's quite a short handler for now at least, maybe in the future it'll improve.

Page fault handler shall handle all virtual memory related faults:
	* User mode faults
	   * Copy-on-Write page faults (future)
	   * Page not present (future)
	      * Mapped     -> fetch from disk
		  * Not mapped -> segmention fault


# System calls

Right now micael supports 4 POSIX-compliant system calls. Their behaviour and calling conventions are defined below

The simplest system calls are the ones where you only have to set the syscall number and then issue interrupt 0x80

## read

Read n bytes of data from device pointed to by file descriptor to buffer.

This system call is actually not really supported. There's a dummy function you can call but I haven't added the needed abstraction for this to work so there's nothing you can read from right now.

### Registers for system call read
EAX: 0 (syscall number)

EBX: file descriptor

ECX: pointer to buffer

EDX: buffer length


## write

Write n bytes of data from buffer to device pointed to by a file descriptor

### Registers for system call write

EAX: 1 (syscall number)

EBX: file descriptor

ECX: pointer to buffer

EDX: buffer length

## fork

Create new process by duplicating the address space of currently running process'

The new process is scheduled automatically

### Registers for system call fork

EAX: 2 (syscall number)

## execv

Replace the currently running process with new code.

Following things are (re)initialized:
	* Code segment
	* Memory map
	* File system context

Basically everything set up by the fork is replaced, effectively rendering the CoW functionality useless.

### Registers for system call fork
EAX: 3 (syscall number)

EBX: file name of the executable

ECX: list of command line arguments (char **argv)

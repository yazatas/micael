# Declare constants for the multiboot header.
.set ALIGN,    1<<0             # align loaded modules on page boundaries
.set MEMINFO,  1<<1             # provide memory map
.set FLAGS,    ALIGN | MEMINFO  # this is the Multiboot 'flag' field
.set MAGIC,    0x1BADB002       # 'magic number' lets bootloader find the header
.set CHECKSUM, -(MAGIC + FLAGS) # checksum of above, to prove we are multiboot

# Declare a header as in the Multiboot Standard.
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

# Reserve a stack for the initial thread.
.section .bss
.align 16
stack_bottom:
.skip 16384 # 16 KiB
stack_top:

.section .text

# The kernel entry point.
.global _start
.global isr0  # divide by zero

_start:
	movl $stack_top, %esp

	# Call the global constructors.
	call _init

	# Transfer control to the main kernel.
	call kernel_main

	# Hang if kernel_main unexpectedly returns.
	cli
1:	hlt
	jmp 1b


.global gdt_flush
gdt_flush:
	movw $0x10, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs

	ljmp $0x08, $.flush
.flush:
	ret

# Interrupt service routines

.global isr0  # division by zero
.global isr1  # debug
.global isr2  # non-maskable interrupt
.global isr3  # breakpoint
.global isr4  # into detected overflow
.global isr5  # out of bounds
.global isr6  # invalid opcode
.global isr7  # no coprocessor 
.global isr8  # double fault
.global isr9  # coprocessor segment overrun 
.global isr10 # bad tss 
.global isr11 # segment not present
.global isr12 # stack fault
.global isr13 # general protection fault
.global isr14 # page faul
.global isr15 # unknown interrupt
.global isr16 # coprocessor fault

# how everything works:
# 1. clear interrupt flag
# 2. push "dummy" error code (not for every interrupt)
# 3. push ISR number
# 4. jump to isr_common, save cpu state and call interrupt handler

# divide by zero
isr0:
	cli
	pushl $0 # error code 0 == no error code
	pushl $0 # interrupt number
	jmp isr_common

# debug
isr1:
	cli
	pushl $0
	pushl $1
	jmp isr_common

# non-maskable interrupt
isr2:
	cli
	pushl $0
	pushl $2
	jmp isr_common

# breakpoint
isr3:
	cli
	pushl $0
	pushl $3
	jmp isr_common

# into detected overflow
isr4:
	cli
	pushl $0
	pushl $4
	jmp isr_common

# out of bounds
isr5:
	cli
	pushl $0
	pushl $5
	jmp isr_common

# invalid opcode
isr6:
	cli
	pushl $0
	pushl $6
	jmp isr_common

# no coprocessor
isr7:
	cli
	pushl $0
	pushl $7
	jmp isr_common

# double fault
isr8:
	cli
	pushl $8 # NOTE! no error code pushed
	jmp isr_common

# coprocessor segment overrun
isr9:
	cli
	pushl $0
	pushl $9
	jmp isr_common

# bad tss
isr10:
	cli
	pushl $10
	jmp isr_common

# segment not present 
isr11:
	cli
	pushl $11
	jmp isr_common

# stack fault
isr12:
	cli
	pushl $12
	jmp isr_common

# general protection fault
isr13:
	cli
	pushl $13
	jmp isr_common

# page fault
isr14:
	cli
	pushl $14
	jmp isr_common

## unknown interrupt
isr15:
	cli
	pushl $0
	pushl $15
	jmp isr_common

# coprocessor fault
isr16:
	cli
	pushl $0
	pushl $16
	jmp isr_common

# save cpu state, call interrupt handler 
# and then restore state
isr_common:
	pusha
	pushw %ds
	pushw %es
	pushw %fs
	pushw %gs
	movw $0x10, %ax # load kernel data segment descriptor
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs
	movl %esp, %eax # push current stack pointer
	pushl %eax
	mov $interrupt_handler, %eax
	call *%eax
	pop %eax
	pop %gs
	pop %fs
	pop %es
	pop %ds
	popa
	addl $8, %esp
	iret 

.size _start, . - _start

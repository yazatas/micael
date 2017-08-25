# Declare constants for the multiboot header.
.set ALIGN,    1 << 0           # align loaded modules on page boundaries
.set MEMINFO,  1 << 1           # provide memory map
.set FLAGS,    ALIGN | MEMINFO  # this is the Multiboot 'flag' field
.set MAGIC,    0x1BADB002       # 'magic number' lets bootloader find the header
.set CHECKSUM, -(MAGIC + FLAGS) # checksum of above, to prove we are multiboot

# Declare a header as in the Multiboot Standard.
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

.section .data
.global boot_page_dir
.align 0x1000
boot_page_dir:
	.long 0x83 # map first 4M
	.rept 768 - 1 # skip 3GB
	.long 0
	.endr
	.long 0x83 # map first 4M of kernel space
	.rept 256 - 1 # skip the rest
	.long 0
	.endr


# Reserve a stack for the initial thread.
.section .bss
.align 16
stack_bottom:
.skip 16384 # 16 KiB
stack_top:

.section .text

# The kernel entry point.
.global _start

_start:
	cli

	mov $(boot_page_dir - 0xc0000000), %ecx
	mov %ecx, %cr3

	mov %cr4, %ecx
	or $0x10, %ecx
	mov %ecx, %cr4

	mov %cr0, %ecx
	or $0x80000000, %ecx
	mov %ecx, %cr0

	lea higher_half_kernel, %ecx
	jmp *%ecx # long jump to update instruction pointer

higher_half_kernel:
	movl $stack_top, %esp

	# Transfer control to the main kernel.
	call kmain

	# Hang if kernel_main unexpectedly returns.
	cli
	hlt

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
.size _start, . - _start

// kernel main function name
.extern kernel_main

.global start

// define Multiboot header
.set MB_MAGIC, 0x1BADB002
.set MB_FLAGS, (1 << 0) | (1 << 1) // load modules on page boundaries and provide a memory map
.set MB_CHECKSUM, (0 - (MB_MAGIC + MB_FLAGS))

.section .multiboot
	.align 4
	.long MB_MAGIC
	.long MB_FLAGS
	.long MB_CHECKSUM

.section .bss
	.align 16
	stack_bottom:
		.skip 1024
	stack_top:


// setup a stack and call kernel_main
// if kernel_main returns (it shouldn't), halt the cpu
.section .text
	start:
		mov $stack_top, %esp // stack grows downwards

		call kernel_main

		hang:
			cli // disable cpu interrupts
			hlt // halt the cpu
			jmp hang // loop until halting works

.section .trampoline
.code16

# After INIT-SIPI-SIPI APs are in 16-bit real mode
# which means we can't directly jump to boot code 
# because it's located at 0x100000 (1MB)
#
# Switch to protected mode so we get full 32-bit access to memory,
# jump to boot code and properly initialize the AP

_trmp_entry:
    cli

    movw %cs,  %ax
    movw %ax,  %ds
    movw %ax,  %es
    movw %ax,  %fs
    movw %ax,  %gs
    movw %ax,  %ss

    # enable a20
    inb $0x92, %al
    orb $0x2, %al
    andb $0xfe, %al
    outb %al, $0x92

    # Loading the GDT in real mode is a little trickier than in 32-bit real mode
    # we need to get the base from CS and multiply it by 16 to get the "base address"
    # (0x55000 for us), then we need to add the relative address of GDT table to that
    # address. Now we have the real address of GDT. This value must be saved to GDT ptr
    # structure before we can actually execute lgdlt
    mov %cs, %ebx
    shl $4, %ebx
	addl $0 + gdt_start - _trmp_entry, %ebx
	movl %ebx, (gdt_ptr_ - _trmp_entry + 2)

	lgdtl (gdt_ptr_ - _trmp_entry)

    # enable protected mode and disable paging
    mov %cr0, %eax
    or $0x1, %eax
    and $0x7fffffff, %eax
    mov %eax, %cr0

    ljmpl $0x8, $_trmp_32

error:
    hlt
    jmp error

# null, code and data
.align 0x10
gdt_start:
	.quad 0x0000000000000000

    # kernel code
    .word 	0xffff
	.word	0x0000
	.byte 	0x00
    .byte 	0x9a
    .byte   0xcf
	.byte 	0x00

    # kernel data
	.word 	0xffff
	.word 	0x0000
	.byte 	0x00
	.byte 	0x92
	.byte 	0xcf
	.byte 	0x00
gdt_ptr_:
    .short 23
    .long gdt_start

.code32
_trmp_32:
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    mov $0x13371338, %ebx
    call _start

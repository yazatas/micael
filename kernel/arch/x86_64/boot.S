#include <arch/x86_64/gdtdef.h>

#define PML4_ATOEI(addr) (((addr) >> 39) & 0x1FF)
#define PDPT_ATOEI(addr) (((addr) >> 30) & 0x1FF)

# general defines
.set KPSTART,         0x0000000000400000
.set KVSTART,         0xFFFFFFFF80400000
.set BOOT_STACK_SIZE,              16384

# control defines
.set CR4_PAE,       (1 <<  5)
.set CR0_PROTECTED, (1 <<  0)
.set CR0_EXT_TYPE,  (1 <<  4)
.set CR0_PAGING,    (1 << 31)

# mmu defines
.set MM_PRESENT, (1 << 0)
.set MM_RDWR,    (1 << 1)
.set MM_2MB,     (1 << 7)

# TODO
.set MM_ALIGN,       0x1000
.set MM_STRUCT_SIZE, 0x1000
.set MM_ENTRY_SIZE,     0x8

# multiboot defines
.set MULTIBOOT2_HEADER_MAGIC,     0xE85250D6
.set MULTIBOOT_ARCHITECTURE_I386, 0x0
.set MULTIBOOT_HEADER_TAG_END,    0x0

.set HEADER_LENGTH, header_end - header_start
.set CHECKSUM, -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT_ARCHITECTURE_I386 + HEADER_LENGTH)

.section .multiboot
header_start:
    .long MULTIBOOT2_HEADER_MAGIC
    .long MULTIBOOT_ARCHITECTURE_I386
    .long HEADER_LENGTH
    .long CHECKSUM

    // TODO provide memory map

    .short MULTIBOOT_HEADER_TAG_END
    .short 0
    .long 8
header_end:

.code32
.section .bss
.comm boot_stack, BOOT_STACK_SIZE

# mmu related stuff
.comm pml4,  MM_STRUCT_SIZE, MM_ALIGN
.comm lpdpt, MM_STRUCT_SIZE, MM_ALIGN
.comm hpdpt, MM_STRUCT_SIZE, MM_ALIGN
.comm lpdt,  MM_STRUCT_SIZE, MM_ALIGN
.comm hpdt,  MM_STRUCT_SIZE, MM_ALIGN

.section .data
.align GDT_TABLE_ALIGNMENT
gdt_table_start:
    .8byte GDT_ENTRY_NULL
    .8byte GDT_ENTRY_KERNEL_CODE

gdt_table_end:
    .skip (GDT_TABLE_SIZE - (gdt_table_end - gdt_table_start))

gdt_ptr:
    .short GDT_TABLE_SIZE - 1
    .long gdt_table_start

.section .text
.global _start

# loader entry point
_start:
    movl $boot_stack + BOOT_STACK_SIZE, %esp

    movl $lpdpt, %eax
    or $(MM_PRESENT | MM_RDWR), %eax
    movl %eax, pml4 + (PML4_ATOEI(KPSTART) * MM_ENTRY_SIZE)

    movl $hpdpt, %eax
    or $(MM_PRESENT | MM_RDWR), %eax
    movl %eax, pml4 + (PML4_ATOEI(KVSTART) * MM_ENTRY_SIZE)

    movl $lpdt, %eax
    or $(MM_PRESENT | MM_RDWR), %eax
    movl %eax, lpdpt + (PDPT_ATOEI(KPSTART) * MM_ENTRY_SIZE)

    movl $hpdt, %eax
    or $(MM_PRESENT | MM_RDWR), %eax
    movl %eax, hpdpt + (PDPT_ATOEI(KVSTART) * MM_ENTRY_SIZE)

    mov $0, %ecx
    movl $_kernel_physical_end, %esi
    shrl $21, %esi
    addl $1, %esi

loop:
    movl $(1 << 21), %eax # two megabytes
    mul %ecx
    or $(MM_PRESENT | MM_RDWR | MM_2MB), %eax
    movl %eax, lpdt(, %ecx, MM_ENTRY_SIZE)
    movl %eax, hpdt(, %ecx, MM_ENTRY_SIZE)

    inc %ecx
    cmp %esi, %ecx
    jne loop

    movl $pml4, %eax
    movl %eax, %cr3

    # enable PAE
    movl $CR4_PAE, %eax
    movl %eax, %cr4

    movl $0xC0000080, %ecx
    rdmsr
    or $(1 << 8), %eax
    wrmsr

    movl $(CR0_PROTECTED | CR0_EXT_TYPE | CR0_PAGING), %eax
    movl %eax, %cr0

    # load GDT and long jump to update instruction pointer
    lgdt gdt_ptr
    ljmp $0x8, $_start_64

    cli
    hlt

.code64
.global _start_64
_start_64:
    movw $0, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    # multiboot info
    pushq %rbx
    call kmain

    cli
    hlt

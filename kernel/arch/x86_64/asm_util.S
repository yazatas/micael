.code64
.section .text
.global gdt_flush
.global tss_flush

gdt_flush:
    lgdt 4(%esp)
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movq $flush, %rax

    .byte 0x48
    ljmp *(%rax)
flush:
    ret

tss_flush:
    movw $0x28, %ax 
    ltr %ax
    ret

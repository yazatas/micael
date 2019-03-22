.text
.global _start

_start:
    mov %esp, %ebp
    mov 0(%ebp), %edi
    lea 4(%ebp), %esi

    call main

lbl:
    jmp lbl

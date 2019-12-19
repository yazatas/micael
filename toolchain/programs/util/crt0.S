.code64
.text
.global _start

_start:
    mov %rsp, %rbp
    mov 0(%rbp), %rdi
    lea 4(%rbp), %rsi

    call main

lbl:
    jmp lbl

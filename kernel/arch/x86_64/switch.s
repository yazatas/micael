.section .text
.global context_switch

# @brief:  switch page directory and update register contents
# @param1: physical address of thread's page directory
# @param2: new stack containing exec state of the thread
context_switch:
    cli

    # address of new page dir and stack
    mov %rdi, %cr3
    mov %rsi, %rsp

    # general-purpose registers
    popq %rax
    popq %rcx
    popq %rdx
    popq %rbx
    popq %rbp
    popq %rsi
    popq %rdi

    # discard irq number and error code
    addq $16, %rsp

    iretq

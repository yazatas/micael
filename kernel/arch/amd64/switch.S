.section .text
.global native_context_switch
.global native_context_switch_user
.global native_context_load

# switch kernel stacks, this is the new way of context switching
#
# instead of just jumping to new task from sched_switch(), switch
# the kernel stacks of prev and cur and rewind curren's stack until
# we're back where we came from [probably __tmr_handler()]
#
# This kind of context switching allows micael to call schedule within
# kernel and is necessary to support f.ex blocking I/O
# (or rather it gives a clean of implementing wait queues and voluntary sleep)
native_context_switch:
    # callee-saved store (sysv abi)
    pushq %r15
    pushq %r14
    pushq %r13
    pushq %r12
    pushq %rsi
    pushq %rdi
    pushq %rbx
    pushq %rbp

    # switch kernel stacks
    movq %rsp, (%rdi)
    movq %rsi, %rsp

    # callee-saved restore (sysv abi)
    popq %rbp
    popq %rbx
    popq %rdi
    popq %rsi
    popq %r12
    popq %r13
    popq %r14
    popq %r15

    ret

# Tasks must be started a little differently, we need to use iretq
# to load the correct segment registers
native_context_switch_user:
    # callee-saved store (sysv abi)
    pushq %r15
    pushq %r14
    pushq %r13
    pushq %r12
    pushq %rsi
    pushq %rdi
    pushq %rbx
    pushq %rbp

    # switch kernel stacks
    movq %rsp, (%rdi)
    movq %rsi, %rsp

    # general-purpose registers
    popq %rax
    popq %rcx
    popq %rdx
    popq %rbx
    popq %rbp
    popq %rsi
    popq %rdi
    addq $16, %rsp

    cmp $0x8, 8(%rsp)
    je 2f
    swapgs
2:
    iretq

native_context_load:
    movq %rdi, %rax
    movq %rax, %cr3
    movq %rsi, %rsp
    popq %rax
    popq %rcx
    popq %rdx
    popq %rbx
    popq %rbp
    popq %rsi
    popq %rdi
    addq $16, %rsp

    cmp $0x8, 8(%rsp)
    je 1f
    swapgs
1:
    iretq

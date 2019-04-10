.section .text
.global context_switch

# @brief:  switch page directory and update register contents
# @param1: physical address of thread's page directory
# @param2: new stack containing exec state of the thread
context_switch:
    cli

    add $4, %rsp

    # address of new page dir and stack
    pop %rcx
    pop %rsp

    mov %ecx, %cr3

    # ignore segment registers
    add $8, %rsp
	/* popw %gs */
	/* popw %fs */
	/* popw %es */
	/* popw %ds */

    # general purpose registers (pusha)
	/* popa */

    # discard irq number and error code
	addl $8, %esp

    iret

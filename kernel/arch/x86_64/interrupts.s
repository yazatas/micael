.section .text

# Interrupt service routines

.global isr0  # division by zero
.global isr1  # debug
.global isr2  # non-maskable interrupt
.global isr3  # breakpoint
.global isr4  # into detected overflow
.global isr5  # out of bounds
.global isr6  # invalid opcode
.global isr7  # no coprocessor
.global isr8  # double fault
.global isr9  # coprocessor segment overrun
.global isr10 # bad tss
.global isr11 # segment not present
.global isr12 # stack fault
.global isr13 # general protection fault
.global isr14 # page faul
.global isr15 # unknown interrupt
.global isr16 # coprocessor fault
.global isr17 # alignment check exception
.global isr18 # machine check exception
.global isr19 # simd floating point exception
.global isr20 # virtualization exception
.global isr128 # system call

# how everything works:
# 1. clear interrupt flag
# 2. push "dummy" error code (not for every interrupt)
# 3. push ISR number
# 4. jump to isr_common, save cpu state and call interrupt handler

# divide by zero
isr0:
	cli
	push $0 # error code 0 == no error code
	push $0 # interrupt number
	jmp isr_common

# debug
isr1:
	cli
	push $0
	push $1
	jmp isr_common

# non-maskable interrupt
isr2:
	cli
	push $0
	push $2
	jmp isr_common

# breakpoint
isr3:
	cli
	push $0
	push $3
	jmp isr_common

# into detected overflow
isr4:
	cli
	push $0
	push $4
	jmp isr_common

# out of bounds
isr5:
	cli
	push $0
	push $5
	jmp isr_common

# invalid opcode
isr6:
	cli
	push $0
	push $6
	jmp isr_common

# no coprocessor
isr7:
	cli
	push $0
	push $7
	jmp isr_common

# double fault
isr8:
	cli
	push $8 # NOTE! no error code pushed
	jmp isr_common

# coprocessor segment overrun
isr9:
	cli
	push $0
	push $9
	jmp isr_common

# bad tss
isr10:
	cli
	push $10
	jmp isr_common

# segment not present
isr11:
	cli
	push $11
	jmp isr_common

# stack fault
isr12:
	cli
	push $12
	jmp isr_common

# general protection fault
isr13:
	cli
	push $13
	jmp isr_common

# page fault
isr14:
	cli
	push $14
	jmp isr_common

## unknown interrupt
isr15:
	cli
	push $0
	push $15
	jmp isr_common

# coprocessor fault
isr16:
	cli
	push $0
	push $16
	jmp isr_common

isr17:
	cli
	push $17
	jmp isr_common

isr18:
	cli
	push $0
	push $18
	jmp isr_common

isr19:
	cli
	push $0
	push $19
	jmp isr_common

isr20:
	cli
	push $0
	push $20
	jmp isr_common

isr128:
	cli
	push $0
	push $0x80
	jmp isr_common

# save cpu state, call interrupt handler
# and then restore state
isr_common:
	/* pusha */
	/* pushl %ds */
	/* pushl %es */
	/* pushl %fs */
	/* pushl %gs */
	movw $0x10, %ax # load kernel data segment descriptor
	/* movw %ax, %ds */
	/* movw %ax, %es */
	/* movw %ax, %fs */
	/* movw %ax, %gs */
	movl %esp, %eax # push current stack pointer
	push %rax
	mov $interrupt_handler, %rax
	call *%rax
	pop %rax
	/* popw %gs */
	/* popw %fs */
	/* popw %es */
	/* popw %ds */
	/* popa */
	addl $8, %esp
	iret


# Interrupt requests
.global irq0  # timer
.global irq1  # keyboard
.global irq2  # cascade
.global irq3  # com2
.global irq4  # com1
.global irq5  # lpt2
.global irq6  # floppy disk
.global irq7  # lpt1
.global irq8  # cmos real-time clock
.global irq9  # legacy scsi / nic
.global irq10 # scsi / nic
.global irq11 # scsi / nic
.global irq12 # ps2 mouse
.global irq13 # fpu / coprocessor / inter-processor
.global irq14 # primary ata hard disk
.global irq15 # secondary ata hard disk

irq0:
	cli
	push $0
	push $32
	jmp irq_common

irq1:
	cli
	push $0
	push $33
	jmp irq_common

irq2:
	cli
	push $0
	push $34
	jmp irq_common

irq3:
	cli
	push $0
	push $35
	jmp irq_common

irq4:
	cli
	push $0
	push $36
	jmp irq_common

irq5:
	cli
	push $0
	push $37
	jmp irq_common

irq6:
	cli
	push $0
	push $38
	jmp irq_common

irq7:
	cli
	push $0
	push $39
	jmp irq_common

irq8:
	cli
	push $0
	push $40
	jmp irq_common

irq9:
	cli
	push $0
	push $41
	jmp irq_common

irq10:
	cli
	push $0
	push $42
	jmp irq_common

irq11:
	cli
	push $0
	push $43
	jmp irq_common

irq12:
	cli
	push $0
	push $44
	jmp irq_common

irq13:
	cli
	push $0
	push $45
	jmp irq_common

irq14:
	cli
	push $0
	push $46
	jmp irq_common

irq15:
	cli
	push $0
	push $47
	jmp irq_common

irq_common:
	/* pusha */
	/* pushw %ds */
	/* pushw %es */
	/* pushw %fs */
	/* pushw %gs */
    pushw $0
    pushw $0
    pushw $0
    pushw $0
	/* movw $0x10, %ax # load kernel data segment descriptor */
	/* movw %ax, %ds */
	/* movw %ax, %es */
	/* movw %ax, %fs */
	/* movw %ax, %gs */
	mov %rsp, %rax # push current stack pointer
	push %rax
	mov $irq_handler, %rax
	call *%rax
	pop %rax
    popw %cx
    popw %cx
    popw %cx
    popw %cx
	/* popw %gs */
	/* popw %fs */
	/* popw %es */
	/* popw %ds */
	/* popa */
	addl $8, %esp # discard irq number and error code
	iret

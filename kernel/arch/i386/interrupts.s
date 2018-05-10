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
	pushl $0 # error code 0 == no error code
	pushl $0 # interrupt number
	jmp isr_common

# debug
isr1:
	cli
	pushl $0
	pushl $1
	jmp isr_common

# non-maskable interrupt
isr2:
	cli
	pushl $0
	pushl $2
	jmp isr_common

# breakpoint
isr3:
	cli
	pushl $0
	pushl $3
	jmp isr_common

# into detected overflow
isr4:
	cli
	pushl $0
	pushl $4
	jmp isr_common

# out of bounds
isr5:
	cli
	pushl $0
	pushl $5
	jmp isr_common

# invalid opcode
isr6:
	cli
	pushl $0
	pushl $6
	jmp isr_common

# no coprocessor
isr7:
	cli
	pushl $0
	pushl $7
	jmp isr_common

# double fault
isr8:
	cli
	pushl $8 # NOTE! no error code pushed
	jmp isr_common

# coprocessor segment overrun
isr9:
	cli
	pushl $0
	pushl $9
	jmp isr_common

# bad tss
isr10:
	cli
	pushl $10
	jmp isr_common

# segment not present 
isr11:
	cli
	pushl $11
	jmp isr_common

# stack fault
isr12:
	cli
	pushl $12
	jmp isr_common

# general protection fault
isr13:
	cli
	pushl $13
	jmp isr_common

# page fault
isr14:
	cli
	pushl $14
	jmp isr_common

## unknown interrupt
isr15:
	cli
	pushl $0
	pushl $15
	jmp isr_common

# coprocessor fault
isr16:
	cli
	pushl $0
	pushl $16
	jmp isr_common

isr17:
	cli
	pushl $17
	jmp isr_common

isr18:
	cli
	pushl $0
	pushl $18
	jmp isr_common

isr19:
	cli
	pushl $0
	pushl $19
	jmp isr_common

isr20:
	cli
	pushl $0
	pushl $20
	jmp isr_common

isr128:
	cli
	pushl $0
	pushl $0x80
	jmp isr_common

# save cpu state, call interrupt handler 
# and then restore state
isr_common:
	pusha
	pushw %ds
	pushw %es
	pushw %fs
	pushw %gs
	movw $0x10, %ax # load kernel data segment descriptor
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs
	movl %esp, %eax # push current stack pointer
	pushl %eax
	mov $interrupt_handler, %eax
	call *%eax
	popl %eax
	popw %gs
	popw %fs
	popw %es
	popw %ds
	popa
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
	pushl $0
	pushl $32
	jmp irq_common

irq1:
	cli
	pushl $0
	pushl $33
	jmp irq_common

irq2:
	cli
	pushl $0
	pushl $34
	jmp irq_common

irq3:
	cli
	pushl $0
	pushl $35
	jmp irq_common

irq4:
	cli
	pushl $0
	pushl $36
	jmp irq_common

irq5:
	cli
	pushl $0
	pushl $37
	jmp irq_common

irq6:
	cli
	pushl $0
	pushl $38
	jmp irq_common

irq7:
	cli
	pushl $0
	pushl $39
	jmp irq_common

irq8:
	cli
	pushl $0
	pushl $40
	jmp irq_common

irq9:
	cli
	pushl $0
	pushl $41
	jmp irq_common

irq10:
	cli
	pushl $0
	pushl $42
	jmp irq_common

irq11:
	cli
	pushl $0
	pushl $43
	jmp irq_common

irq12:
	cli
	pushl $0
	pushl $44
	jmp irq_common

irq13:
	cli
	pushl $0
	pushl $45
	jmp irq_common

irq14:
	cli
	pushl $0
	pushl $46
	jmp irq_common

irq15:
	cli
	pushl $0
	pushl $47
	jmp irq_common

irq_common:
	pusha
	pushw %ds
	pushw %es
	pushw %fs
	pushw %gs
	movw $0x10, %ax # load kernel data segment descriptor
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs
	movl %esp, %eax # push current stack pointer
	pushl %eax
	mov $irq_handler, %eax
	call *%eax
	popl %eax
	popw %gs
	popw %fs
	popw %es
	popw %ds
	popa
	addl $8, %esp # discard irq number and error code
	iret

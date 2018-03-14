#include <kernel/tty.h>
#include <kernel/kprint.h>
#include <kernel/kpanic.h>
#include <kernel/mmu.h>

#include <stdint.h>
#include <stdio.h>

struct regs_t {
	uint16_t gs, fs, es, ds;
	uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; /* pusha */
	uint32_t isr_num, err_num;
	uint32_t eip, cs, eflags, useresp, ss; /*  pushed by cpu */
};

const char *interrupts[] = {
	"division by zero", 
	"debug exception",
	"non-maskable interrupt",
	"breakpoint",
	"into detected overflow",
	"out of bounds",
	"invalid opcode",
	"no coprocessor",
	"double fault",
	"coprocessor segment overrun",
	"bad tss",
	"segment not present",
	"stack fault",
	"general protection fault",
	"page fault",
	"unknown interrupt",
	"coprocessor fault",
	"alignment check",
	"machine check",
	"simd floating point",
	"virtualization",
};

extern void interrupt_handler(struct regs_t *cpu_state)
{
	if (cpu_state->isr_num == 0xe) {
		vmm_pf_handler(cpu_state->err_num);
		return;
	}

	const char *err = "gpf";

	if (cpu_state->isr_num <= 20) {
		err = interrupts[cpu_state->isr_num];
	} 

	kpanic(err);
}

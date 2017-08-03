#ifndef __IDT_H__
#define __IDT_H__

#include <stdint.h>

struct idt_ptr_t {
	uint16_t limit;
	uint32_t base;
} __attribute__((packed));

struct idt_entry_t {
	uint16_t offset0_15;
	uint16_t select;
	uint8_t zero;
	uint8_t type;
	uint16_t offset16_31;
} __attribute__((packed));

/* see isr_common in arch/i386/interrupts.s for more details */
struct regs_t {
	uint16_t gs, fs, es, ds;
	uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; /* pusha */
	uint32_t isr_num, err_num;
	uint32_t eip, cs, eflags, useresp, ss; /*  pushed by cpu */
};

#define IDT_TABLE_SIZE 256
#define IDT_ENTRY_SIZE (sizeof(struct idt_ptr_t))

struct idt_ptr_t idt_ptr;
struct idt_entry_t IDT[IDT_TABLE_SIZE];

void idt_init(void);
void interrupt_handler(struct regs_t *cpu_state);

#endif /* end of include guard: __IDT_H__ */

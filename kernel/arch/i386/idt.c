#include <kernel/idt.h>
#include <kernel/kprint.h>

#include <stdint.h>
#include <string.h>
#include <stdio.h>

/* defined in arch/i386/interrupts.s */
extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr128(); /* 0x80 */

void idt_set_gate(uint32_t offset, uint16_t select, uint8_t type, struct idt_entry_t *entry)
{
	entry->offset0_15  = offset & 0xffff;
	entry->offset16_31 = (offset >> 16) & 0xffff;
	entry->select      = select;
	entry->zero        = 0;
	entry->type        = type;
}

void idt_init(void)
{
	idt_ptr.limit = IDT_ENTRY_SIZE * 256 - 1;
	idt_ptr.base  = (uint32_t)&IDT;

	memset(&IDT, 0, IDT_ENTRY_SIZE * IDT_TABLE_SIZE);

	idt_set_gate((uint32_t)isr0,  0x08, 0x8e, &IDT[0]);
	idt_set_gate((uint32_t)isr1,  0x08, 0x8e, &IDT[1]);
	idt_set_gate((uint32_t)isr2,  0x08, 0x8e, &IDT[2]);
	idt_set_gate((uint32_t)isr3,  0x08, 0x8e, &IDT[3]);
	idt_set_gate((uint32_t)isr4,  0x08, 0x8e, &IDT[4]);
	idt_set_gate((uint32_t)isr5,  0x08, 0x8e, &IDT[5]);
	idt_set_gate((uint32_t)isr6,  0x08, 0x8e, &IDT[6]);
	idt_set_gate((uint32_t)isr7,  0x08, 0x8e, &IDT[7]);
	idt_set_gate((uint32_t)isr8,  0x08, 0x8e, &IDT[8]);
	idt_set_gate((uint32_t)isr9,  0x08, 0x8e, &IDT[9]);
	idt_set_gate((uint32_t)isr10, 0x08, 0x8e, &IDT[10]);
	idt_set_gate((uint32_t)isr11, 0x08, 0x8e, &IDT[11]);
	idt_set_gate((uint32_t)isr12, 0x08, 0x8e, &IDT[12]);
	idt_set_gate((uint32_t)isr13, 0x08, 0x8e, &IDT[13]);
	idt_set_gate((uint32_t)isr14, 0x08, 0x8e, &IDT[14]);
	idt_set_gate((uint32_t)isr15, 0x08, 0x8e, &IDT[15]);
	idt_set_gate((uint32_t)isr16, 0x08, 0x8e, &IDT[16]);
	idt_set_gate((uint32_t)isr17, 0x08, 0x8e, &IDT[17]);
	idt_set_gate((uint32_t)isr18, 0x08, 0x8e, &IDT[18]);
	idt_set_gate((uint32_t)isr19, 0x08, 0x8e, &IDT[19]);
	idt_set_gate((uint32_t)isr20, 0x08, 0x8e, &IDT[20]);
	idt_set_gate((uint32_t)isr128, 0x08, 0xee, &IDT[128]);

	asm volatile ("lidt (%0)" :: "r"(&idt_ptr));

	kdebug("IDT initialized! Start address 0x%x", &IDT);
}

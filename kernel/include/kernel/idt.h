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

#define IDT_TABLE_SIZE 256
#define IDT_ENTRY_SIZE (sizeof(struct idt_ptr_t))

struct idt_ptr_t idt_ptr;
struct idt_entry_t IDT[IDT_TABLE_SIZE];

void idt_init(void);
void idt_set_gate(uint32_t offset, uint16_t select, uint8_t type, struct idt_entry_t *entry);

#endif /* end of include guard: __IDT_H__ */

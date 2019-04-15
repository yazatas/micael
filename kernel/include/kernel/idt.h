#ifndef __IDT_H__
#define __IDT_H__

/* #include <stdint.h> */
#include <sys/types.h>

struct idt_ptr_t {
	uint16_t limit;
#ifdef __x86_64__
    uint64_t base;
#else
	uint32_t base;
#endif
} __attribute__((packed));

struct idt_entry_t {
	uint16_t offset0_15;
	uint16_t select;
#ifdef __x86_64__
    uint8_t ist:3;
    uint8_t reserved1:5;
#else
	uint8_t zero;
#endif
	uint8_t type;
	uint16_t offset16_31;
#ifdef __x86_64__
    uint32_t offset32_63;
    uint32_t reserved2;
#endif
} __attribute__((packed));

#define IDT_TABLE_SIZE 256
#define IDT_ENTRY_SIZE (sizeof(struct idt_ptr_t))

/* defined in arch/ * /idt.c */
extern struct idt_entry_t *IDT;

void idt_init(void);
void idt_set_gate(unsigned long offset, uint16_t select, uint8_t type, struct idt_entry_t *entry);

#endif /* end of include guard: __IDT_H__ */

#include <kernel/gdt.h>
#include <kernel/kprint.h>

#include <string.h>
#include <stdio.h>

extern uint32_t stack_bottom;

static void gdt_set_gate(uint32_t base, uint32_t limit, uint8_t access,
                   uint8_t gran, struct gdt_entry_t *entry)
{
	entry->base_low    = base & 0xffff;
	entry->base_middle = (base >> 16) & 0xff;
	entry->base_high   = (base >> 24) & 0xff;

	entry->limit_low   = limit & 0xffff;
	entry->granularity = (limit >> 16) & 0xffff;
	entry->granularity |= gran & 0xf0;
	entry->access      = access;
}

void gdt_init(void)
{
	gdt_ptr.limit = sizeof(GDT) - 1;
	gdt_ptr.base  = (uint32_t)GDT;

	uint32_t tss_base  = (uint32_t)&tss_ptr;
	uint32_t tss_limit = sizeof(struct tss_ptr_t);

	gdt_set_gate(0, 0, 0, 0, &GDT[0]);                      /* null segment */
	gdt_set_gate(0, 0xfffff, 0x9a, 0xcf, &GDT[1]);          /* code segment */
	gdt_set_gate(0, 0xfffff, 0x92, 0xcf, &GDT[2]);          /* data segment */
	gdt_set_gate(0, 0xfffff, 0xfa, 0xcf, &GDT[3]);          /* user mode code segment */
	gdt_set_gate(0, 0xfffff, 0xf2, 0xcf, &GDT[4]);          /* user mode data segment */
	gdt_set_gate(tss_base, tss_limit, 0x89, 0x40, &GDT[5]); /* tss segment */

	memset(&tss_ptr, 0, sizeof(struct tss_ptr_t));

	tss_ptr.ss0  = SEG_KERNEL_DATA;         /* kernel data segment offset */
	tss_ptr.esp0 = (uint32_t)&stack_bottom; /* kernel stack */

	gdt_flush(gdt_ptr);
	kdebug("GDT and TSS initialized! Start addresses 0x%x 0x%x", &GDT, &tss_ptr);

	kdebug("TSS initialized! Start address 0x%x", &tss_ptr);
	tss_flush(&tss_ptr);
}

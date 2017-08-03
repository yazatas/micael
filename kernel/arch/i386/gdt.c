#include <kernel/gdt.h>
#include <string.h>
#include <stdio.h>


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
	gdt_ptr.limit = (GDT_ENTRY_SIZE * 5) - 1;	
	gdt_ptr.base  = (uint32_t)&GDT;

	gdt_set_gate(0, 0, 0, 0, &GDT[0]);                /* null segment */
	gdt_set_gate(0, 0xffffffff, 0x9a, 0xcf, &GDT[1]); /* code segment */
	gdt_set_gate(0, 0xffffffff, 0x92, 0xcf, &GDT[2]); /* data segment */
	gdt_set_gate(0, 0xffffffff, 0x7a, 0xcf, &GDT[3]); /* user mode code segment */
	gdt_set_gate(0, 0xffffffff, 0x72, 0xcf, &GDT[4]); /* user mode data segment */

	/* TODO: move this to boot.s */
	asm volatile ("lgdtl (gdt_ptr)");
	gdt_flush((uint32_t)&gdt_ptr);
}

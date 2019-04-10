#include <arch/x86_64/gdt.h>
#include <kernel/gdt.h>
#include <kernel/kprint.h>
#include <kernel/util.h>

extern unsigned long boot_stack;
/* extern unsigned long gdt_ptr; */
extern unsigned long *gdt_table_start;
extern struct tss_ptr_t tss_ptr;

void gdt_set_gate(uint32_t base, uint32_t limit, uint8_t access,
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
    /* on x86_64, the GDT has been initialized during boot and we only need to 
     * initialize the TSS here */

    /* gdt_set_gate(0, 0, GDT_64BIT, GDT_PRESENT | GDT_CPL0   | GDT_EXEC, */ 
    /*         (struct gdt_entry_t *)&gdt_table_start[1]); */
    /* gdt_set_gate(0, 0, GDT_64BIT, GDT_PRESENT | GDT_CPL0   | GDT_RDWR, */
    /*         (struct gdt_entry_t *)&gdt_table_start[2]); */
    /* gdt_set_gate(0, 0, GDT_64BIT, GDT_PRESENT | GDT_CPL3   | GDT_EXEC, */
    /*         (struct gdt_entry_t *)&gdt_table_start[3]); */
    /* gdt_set_gate(0, 0, GDT_64BIT, GDT_PRESENT | GDT_CPL3   | GDT_RDWR, */
    /*         (struct gdt_entry_t *)&gdt_table_start[4]); */
    /* /1* gdt_set_gate(0, 0, GDT_64BIT, GDT_PRESENT | GDT_ACCESS | GDT_EXEC, *1/ */
    /* /1*         (struct gdt_entry_t *)&gdt_table_start[5]); *1/ */

    /* gdt_flush(gdt_flush); */

	/* gdt_set_gate(0, 0xfffff, 0x9a, 0xcf, &GDT[1]);          /1* code segment *1/ */
	/* gdt_set_gate(0, 0xfffff, 0x92, 0xcf, &GDT[2]);          /1* data segment *1/ */
	/* gdt_set_gate(0, 0xfffff, 0xfa, 0xcf, &GDT[3]);          /1* user mode code segment *1/ */
	/* gdt_set_gate(0, 0xfffff, 0xf2, 0xcf, &GDT[4]);          /1* user mode data segment *1/ */
	/* gdt_set_gate(tss_base, tss_limit, 0x89, 0x40, &GDT[5]); /1* tss segment *1/ */

	/* kmemset(&tss_ptr, 0, sizeof(struct tss_ptr_t)); */
	/* tss_ptr.ss0  = SEG_KERNEL_DATA;       /1* kernel data segment offset *1/ */
	/* tss_ptr.esp0 = (uint32_t)&boot_stack; /1* kernel stack *1/ */
	/* gdt_flush(gdt_ptr); */
	/* tss_flush(&tss_ptr); */
}

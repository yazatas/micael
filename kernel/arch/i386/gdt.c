#include <kernel/gdt.h>
#include <string.h>
#include <stdio.h>

struct gdt_desc_t gmts[GDT_TABLE_SIZE];
struct gdt_reg_t gdt_reg;

static void init_gdt_desc(uint32_t base, uint32_t limit, uint8_t access,
                   uint8_t other, struct gdt_desc_t *desc)
{
	desc->limit0_15  = limit & 0xffff;
	desc->base0_15   = base & 0xffff;
	desc->base16_23  = (base >> 16) & 0xff;
	desc->access     = access;
	desc->limit16_19 = (base >> 16) & 0xf;
	desc->other      = other & 0xf;
	desc->base24_31  = (base >> 24) & 0xff;
}

void gdt_init(void)
{
	init_gdt_desc(0x0, 0x0, 0x0, 0x0, &gmts[0]);       /* null */
	init_gdt_desc(0x0, 0xfffff, 0x9a, 0x0d, &gmts[1]); /* code */
	init_gdt_desc(0x0, 0xfffff, 0x92, 0x0d, &gmts[2]); /* data */
	/* TODO: task state segment */

	gdt_reg.limit = GDT_ENTRY_SIZE * GDT_TABLE_SIZE;
	gdt_reg.base  = GDT_BASE;

	memcpy((void*)gdt_reg.base, gmts, gdt_reg.limit);

	/* load registry */
	asm volatile("lgdtl (gdt_reg)");

	/* init segments */
	asm volatile(" movw $0x10, %ax \n\
			       movw %ax, %ds  \n\
				   movw %ax, %es  \n\
				   movw %ax, %fs  \n\
				   movw %ax, %gs  \n\
				   ljmp $0x8, $next \n\
				   next:            \n");
}

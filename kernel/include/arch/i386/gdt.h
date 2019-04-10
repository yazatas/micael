#ifndef __ARCH_I386_GDT_H__
#define __ARCH_I386_GDT_H__

struct tss_ptr_t {
	uint32_t prev_tss; /* for hw task switching, not supported */
  	uint32_t esp0;     /* The stack pointer to load when we change to kernel mode */
  	uint32_t ss0;      /* The stack segment to load when we change to kernel mode */

  	/* unused */
  	uint32_t esp1, ss1, esp2, ss2, cr3, eip, eflags;
  	uint32_t eax,  ecx, edx,  ebx, esp, ebp, esi, edi, trap;
  	uint32_t es,   cs,  ss,   ds,  fs,  gs,  ldt, iomap_base;
} __attribute__((packed));

#define GDT_BASE       0x00000800
#define GDT_TABLE_SIZE 6 // TODO +1 to include tss
#define GDT_ENTRY_SIZE (sizeof(struct gdt_entry_t))

struct gdt_entry_t GDT[GDT_TABLE_SIZE];
struct gdt_ptr_t gdt_ptr;

void arch_tss_flush();
void arch_gdt_flush();

void arch_gdt_init(void);

#endif /* __ARCH_I386_GDT_H__ */

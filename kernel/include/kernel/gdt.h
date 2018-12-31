#ifndef __GDT_H__
#define __GDT_H__

#include <stdint.h>

#define DPL_KERNEL 0x00
#define DPL_USER   0x03

#define CPL_KERNEL 0x00
#define CPL_USER   0x03

#define SEG_KERNEL_CODE 0x08
#define SEG_KERNEL_DATA 0x10
#define SEG_USER_CODE   0x1b
#define SEG_USER_DATA   0x23

struct gdt_ptr_t {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

struct gdt_entry_t {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
	uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed)); 

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
struct tss_ptr_t tss_ptr;

extern void tss_flush();
extern void gdt_flush();

void gdt_init(void);

#endif /* end of include guard: __GDT_H__ */

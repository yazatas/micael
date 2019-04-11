#ifndef __ARCH_I386_GDT_H__
#define __ARCH_I386_GDT_H__

#define GDT_BASE       0x00000800
#define GDT_TABLE_SIZE 6 // TODO +1 to include tss
#define GDT_ENTRY_SIZE (sizeof(struct gdt_entry_t))

struct gdt_entry_t GDT[GDT_TABLE_SIZE];
struct gdt_ptr_t gdt_ptr;
struct tss_ptr_t tss_ptr;

#endif /* __ARCH_I386_GDT_H__ */

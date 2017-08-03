#ifndef __GDT_H__
#define __GDT_H__

#include <stdint.h>

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

#define GDT_BASE       0x00000800
#define GDT_TABLE_SIZE 5
#define GDT_ENTRY_SIZE (sizeof(struct gdt_entry_t))

struct gdt_entry_t GDT[GDT_TABLE_SIZE];
struct gdt_ptr_t gdt_ptr;

extern void gdt_flush();

void gdt_init(void);

#endif /* end of include guard: __GDT_H__ */

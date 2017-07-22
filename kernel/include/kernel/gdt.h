#ifndef __GDT_H__
#define __GDT_H__

#include <stdint.h>

struct gdt_reg_t {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

struct gdt_desc_t {
    uint16_t limit0_15;
    uint16_t base0_15;
    uint8_t  base16_23;
    uint8_t  access;
    uint8_t  limit16_19:4;
    uint8_t  other:4;
    uint8_t  base24_31;
} __attribute__((packed));

#define GDT_BASE       0x00000800
#define GDT_TABLE_SIZE 3
#define GDT_ENTRY_SIZE (sizeof(struct gdt_desc_t))

extern struct gdt_desc_t gmts[GDT_TABLE_SIZE];
extern struct gdt_reg_t gdt_reg;

void gdt_init(void);

#endif /* end of include guard: __GDT_H__ */

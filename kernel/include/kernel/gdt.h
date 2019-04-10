#ifndef __GDT_H__
#define __GDT_H__

#include <sys/types.h>

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

void tss_flush();
void gdt_flush();

void gdt_init(void);

#endif /* end of include guard: __GDT_H__ */

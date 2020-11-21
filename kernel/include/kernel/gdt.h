#ifndef __GDT_H__
#define __GDT_H__

#include <sys/types.h>
#include <kernel/asm.h>
#include <kernel/common.h>
#include <kernel/compiler.h>

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
#ifdef __amd64__
    uint64_t base;
#else
    uint32_t base;
#endif
} __attribute__((packed));

struct gdt_entry_t {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __packed;

struct tss_ptr_t {
#ifdef __amd64__
    uint32_t rsrvd;
    uint32_t rsp0_low, rsp0_high;

    uint32_t rsp1_low, rsp1_high;
    uint32_t rsp2_low, rsp2_high;

    uint32_t rsrvd_2,  rsrvd_3;

    uint32_t ist1_low, ist1_high;
    uint32_t ist2_low, ist2_high;
    uint32_t ist3_low, ist3_high;
    uint32_t ist4_low, ist4_high;
    uint32_t ist5_low, ist5_high;
    uint32_t ist6_low, ist6_high;
    uint32_t ist7_low, ist7_high;

    uint32_t rsrvd_4,  rsrvd_5;
    uint16_t iopb,     rsrvd_6;
#else
    uint32_t prev_tss; /* for hw task switching, not supported */
    uint32_t esp0;     /* The stack pointer to load when we change to kernel mode */
    uint32_t ss0;      /* The stack segment to load when we change to kernel mode */

    /* unused */
    uint32_t esp1, ss1, esp2, ss2, cr3, eip, eflags;
    uint32_t eax,  ecx, edx,  ebx, esp, ebp, esi, edi, trap;
    uint32_t es,   cs,  ss,   ds,  fs,  gs,  ldt, iomap_base;
#endif
} __packed;

void gdt_init(void);
void tss_init(void);
void tss_update_rsp(unsigned long rsp);

#endif /* end of include guard: __GDT_H__ */

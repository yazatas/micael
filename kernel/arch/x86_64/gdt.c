#include <kernel/gdt.h>
#include <kernel/kprint.h>
#include <kernel/util.h>

extern unsigned long boot_stack;

static uint64_t GDT[5] = { 0 };
struct gdt_ptr_t gdt_ptr;
struct tss_ptr_t tss_ptr;

void gdt_init(void)
{
    GDT[0] = 0UL;
    GDT[1] = 0x00a0980000000000UL; /* kernel code */
    GDT[2] = 0x00c0920000000000UL; /* kernel data */
    GDT[3] = 0x00a0f80000000000UL; /* user code */
    GDT[4] = 0x00c0f20000000000UL; /* user data */

    gdt_ptr.limit = sizeof(GDT) - 1;
    gdt_ptr.base  = (uint64_t)GDT;

    gdt_flush(&gdt_ptr);
}

void tss_init(void)
{
}

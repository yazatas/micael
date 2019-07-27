#include <drivers/vbe.h>
#include <drivers/ioapic.h>
#include <drivers/lapic.h>
#include <kernel/acpi.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/pic.h>
#include <kernel/kprint.h>
#include <kernel/kpanic.h>
#include <kernel/tick.h>
#include <kernel/util.h>
#include <mm/heap.h>
#include <mm/mmu.h>
#include <fs/binfmt.h>
#include <fs/fs.h>
#include <sched/task.h>
#include <sched/sched.h>
#include <drivers/tty.h>
#include <drivers/ps2.h>
#include <drivers/vga.h>
#include <errno.h>
#include <stdint.h>

extern uint8_t _boot_end;
extern uint8_t _boot_start;
extern uint8_t _ll_end;
extern uint8_t _trampoline_load;

void init_bsp(void *arg)
{
    /* initialize console for pre-mmu prints */
    vga_init();

    /* initialize all low-level stuff (GDT, IDT, IRQ, TSS, and keyboard) */
    gdt_init(); idt_init(); irq_init(); tss_init(); ps2_init();

    /* initialize archictecture-specific MMU, the boot memory allocator.
     * Use boot memory allocator to initialize PFA, SLAB and Heap 
     *
     * VBE can be initialized after MMU */
    mmu_init(arg);
    vbe_init();

    /* parse ACPI tables and initialize the Local APIC of BSP and all I/O APICs */
    acpi_initialize();
    acpi_parse_madt();
    lapic_initialize();

    /* enable Local APIC timer so tick_wait() works */
    enable_irq();

    /* initialize inode and dentry caches, mount pseudo rootfs and devfs */
    vfs_init();

    if (vfs_install_rootfs("initramfs", arg) < 0)
        kpanic("failed to install rootfs!");

    /* initialize tty to /dev/tty1 */
    if (tty_init() == NULL)
        kpanic("failed to init tty1");

    /* create init and idle tasks and start the scheduler */
    sched_init();
    sched_start();

    for (;;);
}

void init_ap(void *arg)
{
    (void)arg;

    asm volatile ("mov $0x1337, %r11");

    for (;;);
}

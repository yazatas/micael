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
extern uint8_t _trampoline_start;
extern uint8_t _trampoline_end;
extern uint8_t _ll_end;
extern uint8_t _trampoline_load;

void init_bsp(void *arg)
{
    /* initialize console for pre-mmu prints */
    vga_init();

    /* initialize all low-level stuff (GDT, IDT, IRQ, etc.) */
    gdt_init(); idt_init(); irq_init();

    /* initialize keyboard */
    ps2_init();

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

    /* Enable Local APIC timer so tick_wait() works */
    enable_irq();

    kdebug("starting smp initialization...");
    /* Initialize SMP trampoline and wake up all APs one by one 
     * The SMP trampoline is located at 0x55000 and the trampoline switches
     * the AP from real mode to protected mode and calls _start (in boot.S)
     *
     * 0x55000 is below 0x100000 so the memory is indetity mapped */
    size_t trmp_size = (size_t)&_trampoline_end - (size_t)&_trampoline_start;

    kmemcpy((uint8_t *)0x55000, &_trampoline_start, trmp_size);
    kdebug("copy done");

    for (int i = 1; i < 4; ++i) {
        kdebug("send init");
        lapic_send_init(i);
        tick_wait(tick_ms_to_ticks(10));

        kdebug("send sipi");
        lapic_send_sipi(i, 0x55);
        tick_wait(tick_ms_to_ticks(1));

        kdebug("Waiting for CPU %u to register itself...", i);
        for (;;);
    }

    for (;;);

#if 0
    /* initialize inode and dentry caches, mount pseudo rootfs and devfs */
    vfs_init();

    if (vfs_install_rootfs("initramfs", mbinfo) < 0)
        kpanic("failed to install rootfs!");

    /* initialize tty to /dev/tty1 */
    if (tty_init() == NULL)
        kpanic("failed to init tty1");

    /* create init and idle tasks and start the scheduler */
    sched_init();
    sched_start();
#endif

    for (;;);
}

void init_ap(void *arg)
{
    asm volatile ("mov $0x1337, %r11");

    for (;;);
}

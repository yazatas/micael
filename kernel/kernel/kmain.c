#include <drivers/vbe.h>
#include <drivers/ioapic.h>
#include <drivers/lapic.h>
#include <kernel/acpi.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/isr.h>
#include <kernel/pic.h>
#include <kernel/kprint.h>
#include <kernel/kpanic.h>
#include <kernel/percpu.h>
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

/* defined by the linker */
extern uint8_t _percpu_start, _percpu_end;
extern uint8_t _kernel_physical_end;
extern uint8_t _trampoline_start;
extern uint8_t _trampoline_end;

void init_bsp(void *arg)
{
    /* initialize console for pre-mmu prints */
    vga_init();

    /* initialize all low-level stuff (GDT, IDT, IRQ) */
    gdt_init(); idt_init(); irq_init();

    /* initialize archictecture-specific MMU, the boot memory allocator.
     * Use boot memory allocator to initialize PFA, SLAB and Heap 
     *
     * VBE can be initialized only after MMU */
    mmu_init(arg);
    vbe_init();

    /* parse ACPI tables and initialize the Local APIC of BSP and all I/O APICs */
    acpi_initialize();
    acpi_parse_madt();
    ioapic_initialize_all();
    lapic_initialize();

    /* initialize SMP trampoline for the APs */
    size_t trmp_size = (size_t)&_trampoline_end - (size_t)&_trampoline_start;
    kmemcpy((uint8_t *)0x55000, &_trampoline_start, trmp_size);

    /* initialize the percpu areas for all processors */
    unsigned long kernel_end = (unsigned long)&_kernel_physical_end;
    unsigned long pcpu_start = ROUND_UP(kernel_end, PAGE_SIZE);
    size_t pcpu_size         = (unsigned long)&_percpu_end - (unsigned long)&_percpu_start;

    for (size_t i = 0; i < lapic_get_cpu_count(); ++i) {
        kmemcpy(
            (uint8_t *)pcpu_start + i * pcpu_size,
            (uint8_t *)&_percpu_start,
            pcpu_size
        );
    }

    /* initialize global percpu state and GS base for BSP
     * TSS can be initialized after percpu */
    percpu_init(0);
    tss_init();

    /* enable Local APIC timer so tick_wait() works */
    enable_irq();

    /* initialize inode and dentry caches, mount pseudo rootfs and devfs */
    vfs_init();

    if (vfs_install_rootfs("initramfs", arg) < 0)
        kpanic("failed to install rootfs!");

    /* initialize /dev/kdb and /dev/tty1 */
    if (ps2_init() != 0 || tty_init() == NULL )
        kpanic("failed to init tty1 or keyboard");

    /* create init and idle tasks and start the scheduler */
    sched_init();
    sched_start();

    for (;;);
}

void init_ap(void *arg)
{
    (void)arg;

    gdt_init();
    idt_init();
    lapic_initialize();
    percpu_init(lapic_get_init_cpu_count() - 1);
    tss_init();

    /* Initialize the idle task for this CPU and start it.
     *
     * BSP is waiting for us to jump to idle task and when we do that,
     * it will start the next CPU */
    sched_init_cpu();
    sched_start();

    for (;;);
}

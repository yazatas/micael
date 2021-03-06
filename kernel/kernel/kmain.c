#include <drivers/gfx/vbe.h>
#include <drivers/gfx/vga.h>
#include <drivers/ioapic.h>
#include <drivers/lapic.h>
#include <drivers/bus/pci.h>
#include <drivers/device.h>
#include <kernel/acpi/acpi.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/irq.h>
#include <kernel/mp.h>
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
#include <drivers/console/tty.h>
#include <drivers/console/ps2.h>
#include <errno.h>
#include <stdint.h>

/* defined by the linker */
extern uint8_t _percpu_start, _percpu_end;
extern uint8_t _kernel_physical_end;
extern uint8_t _trampoline_start;
extern uint8_t _trampoline_end;

void init_bsp(void *arg)
{
    /* Initialize IRQ subsystem and install default IRQ handlers */
    irq_init();

    /* initialize console for pre-mmu prints */
    vga_init();

    /* initialize all low-level stuff (GDT, IDT, IRQ) */
    gdt_init(); idt_init(); pic_init();

    /* initialize archictecture-specific MMU, the boot memory allocator.
     * Use boot memory allocator to initialize PFA, SLAB and Heap */
    mmu_init(arg);

    /* parse symbol and string tables from the image */
    multiboot2_parse_elf(arg);

    /* parse ACPI tables and initialize the Local APIC of BSP all I/O APICs */
    acpi_init();
    ioapic_initialize_all();
    lapic_initialize();

    /* initialize the vfs subsystem so that new devices can be registered to devfs */
    vfs_init();

    /* Initialize the device/driver subsystem
     * and register drivers for all supported devices */
    dev_init();

    /* initialize the PCI bus(es) and after PCI
     * the VBE to get the address of the linear frame buffer */
    pci_init();
    vbe_init();

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

    /* install rootfs from initramfs */
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
    tick_init_timer();

    /* Initialize the idle task for this CPU and start it.
     *
     * BSP is waiting for us to jump to idle task and when we do that,
     * it will start the next CPU */
    sched_init_cpu();
    sched_start();

    for (;;);
}

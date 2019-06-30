#include <drivers/vbe.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/irq.h>
#include <kernel/kprint.h>
#include <kernel/kpanic.h>
#include <kernel/util.h>
#include <mm/heap.h>
#include <mm/mmu.h>
#include <mm/page.h>
#include <fs/binfmt.h>
#include <fs/fs.h>
#include <sched/task.h>
#include <sched/sched.h>
#include <drivers/timer.h>
#include <drivers/tty.h>
#include <drivers/ps2.h>
#include <drivers/vga.h>
#include <errno.h>
#include <stdint.h>
#include <kernel/kassert.h>

void kmain(void *arg)
{
    /* initialize all low-level stuff (GDT, IDT, IRQ, etc.) */
    gdt_init(); idt_init(); irq_init();

    /* initialize console for pre-mmu prints */
    vga_init();

    /* initialize keyboard */
    ps2_init();

    /* building memory map may require multiboot info */
    mmu_init(arg);

    /* VBE must be initialized after MMU, because it uses
     * mmu functions to get the from from VGA memory */
    vbe_init();

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

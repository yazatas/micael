#include <drivers/vbe.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/irq.h>
#include <kernel/kprint.h>
#include <kernel/kpanic.h>
#include <kernel/util.h>
#include <mm/cache.h>
#include <mm/heap.h>
#include <mm/mmu.h>
#include <fs/binfmt.h>
#include <fs/fs.h>
#include <sched/task.h>
#include <sched/sched.h>
#include <drivers/timer.h>
#include <drivers/tty.h>
#include <drivers/ps2.h>
#include <errno.h>
#include <stdint.h>

void kmain(multiboot_info_t *mbinfo)
{
    /* initialize GDT, IDT and IRQs */
    gdt_init(); idt_init(); irq_init();

    /* initialize keyboard */
    ps2_init();

    /* building memory map requires multiboot info */
    mmu_init(mbinfo);

    /* VBE must be initialized after MMU, because it uses
     * mmu functions to get the from from VGA memory */
    vbe_init();

    /* initialize inode and dentry caches, mount pseudo rootfs and devfs */
    vfs_init();

    if (vfs_install_rootfs("initramfs", mbinfo) < 0)
        kpanic("failed to install rootfs!");

    /* initialize tty to /dev/tty1 */
    if (tty_init() == NULL)
        kpanic("failed to init tty1");

    path_t *path = NULL;
    file_t *file = NULL;

    enable_irq();

    if ((path = vfs_path_lookup("/dev/tty1", 0))->p_flags == LOOKUP_STAT_SUCCESS) {
        if ((file = file_open(path->p_dentry, O_RDWR)) != NULL) {
            file_write(file, 0, 14, "Hello, world!\n");

            char buf[10] = { 0 };

            if (file_read(file, 0, 9, buf) == 9)
                kdebug("'%s'", buf);
            else
                kdebug("error: %s", buf);

        } else {
            kdebug("Failed to open file: %s", kstrerror(errno));
        }
    }

    /* create init and idle tasks and start the scheduler */
    /* sched_init(); */
    /* sched_start(); */

    for (;;);
}

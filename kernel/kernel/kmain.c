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
#include <drivers/keyboard.h>
#include <errno.h>
#include <stdint.h>

void kmain(multiboot_info_t *mbinfo)
{
    gdt_init(); idt_init(); irq_init();
    kb_install();

    /* building memory map requires multiboot info */
    mmu_init(mbinfo);

    /* VBE must be initialized after MMU, because it uses
     * mmu functions to get the from from VGA memory */
    vbe_init();

    /* initialize inode and dentry caches, mount pseudo rootfs and devfs */
    vfs_init();

    if (vfs_install_rootfs("initramfs", mbinfo) < 0)
        kpanic("failed to install rootfs!");

    path_t *path = NULL;
    char *paths[8] = {
        "/sbin/init", "/sbin/test", "/sbin/dsh",
        "/usr/bin/echo",  "/bin/usr/cat", "/bin/echo",
        "/usr/bin/cat", "/bin/cat"
    };

    for (int i = 0; i < 8; ++i) {
        if ((path = vfs_path_lookup(paths[i], 0))->p_dentry == NULL)
            kprint("\t%s NOT FOUND!\n", paths[i]);
        else
            kprint("\t%s FOUND!\n", paths[i]);
    }

    /* create init and idle tasks and start the scheduler */
    /* sched_init(); */
    /* sched_start(); */

    for (;;);
}

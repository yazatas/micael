#ifndef __INITRD_H__
#define __INITRD_H__

#include <fs/vfs.h>
#include <fs/multiboot.h>

vfs_status_t initrd_init(multiboot_info_t *mbinfo);

#endif /* __INITRD_H__ */

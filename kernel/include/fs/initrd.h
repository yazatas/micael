#ifndef __INITRD_H__
#define __INITRD_H__

#include <fs/multiboot.h>
#include <fs/fs.h>

fs_t *initrd_create(void *args);
void  initrd_destroy(fs_t *fs);

#endif /* __INITRD_H__ */

#ifndef __INITRD_H__
#define __INITRD_H__

#include <fs/multiboot.h>

#define HEADER_MAGIC  0x13371338
#define FILE_MAGIC    0xdeadbeef
#define FNAME_MAXLEN  64

typedef struct disk_header {
    uint8_t  file_count;
    uint32_t disk_size;
    uint32_t magic;
} disk_header_t;

typedef struct file_header {
    char file_name[FNAME_MAXLEN];
    uint32_t file_size;
    uint32_t magic;
} file_header_t;

uint32_t initrd_lookup(const char *path);
/* uint32_t initrd_read(file_t *file, uint32_t offset, uint32_t size, uint8_t *buffer); */
void initrd_open(const char *path, uint8_t mode);

/* fs_t *initrd_create(void); */
/* void initrd_destroy(fs_t *fs); */

/* returns the start address of XXX */
disk_header_t *initrd_init(multiboot_info_t *mbinfo);

#endif /* __INITRD_H__ */

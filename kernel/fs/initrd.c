#include <fs/vfs.h>
#include <fs/initrd.h>
#include <mm/kheap.h>

static uint32_t initrd_read(file_t *file, uint32_t offset, uint32_t size, uint8_t *buffer)
{
    return 0;
}

static uint32_t initrd_write(file_t *file, uint32_t offset, uint32_t size, uint8_t *buffer)
{
    return 0;
}

static void initrd_open(file_t *file, uint8_t read, uint8_t write)
{
}

static void initrd_close(file_t *file)
{
}

static dentry_t *initrd_read_dir(file_t *file, uint32_t index)
{
    return NULL;
}

static file_t *initrd_find_dir(file_t *file, char *name)
{
    return NULL;
}

static fs_t *initrd_create(void)
{
    return NULL;
}

static void initrd_destory(fs_t *fs)
{
    kfree(fs);
}

/* TODO: return file system object instead */
vfs_status_t inird_init(multiboot_info_t *mbinfo)
{
    fs_t *fs = kmalloc(sizeof(fs_t));

    fs->name    = "initrd";
    fs->create  = initrd_create;
    fs->destroy = initrd_destory;

    return VFS_OK;
}



#include <fs/vfs.h>
#include <kernel/kprint.h>
#include <fs/initrd.h>

void vfs_init(multiboot_info_t *mbinfo)
{
    initrd_init(mbinfo);
}

/* TODO: return error or set fs_errno?? */
void vfs_open(file_t *node, uint8_t read, uint8_t write)
{
    if (node && node->open) {
        node->open(node, read, write);
    }
}

/* TODO: return error or set fs_errno?? */
void vfs_close(file_t *node)
{
    if (node && node->close) {
        node->close(node);
    }
}

uint32_t vfs_read(file_t *node, uint32_t offset, uint32_t size, uint8_t *buffer)
{
    if (node && node->read) {
        return node->read(node, offset, size, buffer);
    }
    return VFS_CALLBACK_MISSING;
}

uint32_t vfs_write(file_t *node, uint32_t offset, uint32_t size, uint8_t *buffer)
{
    if (node && node->write) {
        return node->write(node, offset, size, buffer);
    }
    return VFS_CALLBACK_MISSING;
}

dentry_t *vfs_read_dir(file_t *node, uint32_t index)
{
    if (node && node->read_dir) {
        return node->read_dir(node, index);
    }
    return NULL;
}

file_t *vfs_find_dir(file_t *node, char *name)
{
    if (node && node->read_dir) {
        return node->find_dir(node, name);
    }
    return NULL;
}

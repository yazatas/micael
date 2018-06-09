#ifndef __VFS_H__
#define __VFS_H__

#include <fs/multiboot.h>
#include <lib/list.h>
#include <kernel/common.h>

#define VFS_NAME_MAX_LEN       128

enum vfs_types {
    VFS_TYPE_FILE        = 1 << 0,
    VFS_TYPE_DIRECTORY   = 1 << 1,
    VFS_TYPE_PIPE        = 1 << 2,
    VFS_TYPE_SYMLINK     = 1 << 3,
    VFS_TYPE_CHARDEVICE  = 1 << 4,
    VFS_TYPE_BLOCKDEVICE = 1 << 5,
    VFS_TYPE_MOUNTPOINT  = 1 << 6
};

#define VFS_IS_FILE(node) ((node->flags & VFS_TYPE_DIRECTORY) != 0)
#define VFS_IS_DIR(node)  ((node->inode->flags & VFS_TYPE_FILE) != 0)

enum vfs_errors {
    VFS_CALLBACK_MISSING = -1,
};

typedef int32_t off_t;

typedef struct inode {
    uint32_t mask;        /* permissions */
    uint32_t uid;
    uint32_t gid;
    uint32_t flags;
    uint32_t inode;
    uint32_t size;
    uint32_t impl;        /* TODO: ??? */
} inode_t;


typedef struct dentry {
    char name[VFS_NAME_MAX_LEN];

    struct dentry *parent;
    struct dentry *children;
    list_head_t *siblings;

    inode_t *inode;
} dentry_t;

typedef struct file {
    char name[VFS_NAME_MAX_LEN];

    dentry_t *dentry;
    inode_t *inode;

    uint32_t (*read)(struct file  *, uint32_t, uint32_t, uint8_t *);
    uint32_t (*write)(struct file *, uint32_t, uint32_t, uint8_t *);
    void     (*open)(struct file  *, uint8_t,  uint8_t);
    void     (*close)(struct file *);
    dentry_t *(*read_dir)(struct file *, uint32_t);
    struct file *(*find_dir)(struct file *, char *);

    /* list_head_t list; */
    struct file *ptr; // Used by mountpoints and symlinks.
} file_t;

void vfs_init(multiboot_info_t *mbinfo);

void vfs_close(file_t *node);
void vfs_open(file_t *node, uint8_t read, uint8_t write);

uint32_t vfs_read(file_t *node,  uint32_t offset, uint32_t size, uint8_t *buffer);
uint32_t vfs_write(file_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);

dentry_t *vfs_read_dir(file_t *node, uint32_t index);
file_t   *vfs_find_dir(file_t *node, char *name);

#endif /* end of include guard: __VFS_H__ */

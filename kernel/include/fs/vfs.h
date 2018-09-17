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

enum vfs_modes {
    VFS_READ   = 1 << 0,
    VFS_WRITE  = 1 << 1,
    VFS_RW     = VFS_READ | VFS_WRITE
};

typedef enum vfs_status {
    VFS_OK               =  0,
    VFS_GENERAL_ERROR    = -1,
    VFS_CALLBACK_MISSING = -2,
    VFS_FS_INIT_FAILED   = -3,
} vfs_status_t;

typedef int32_t off_t;

typedef struct fs fs_t;
typedef struct inode inode_t;
typedef struct file file_t;
typedef struct dentry dentry_t;
typedef struct mount mount_t;

/* TODO:  WHAT IS A MOUNTPOINT */
mount_t *root_fs;

struct super_ops {
    inode_t *(*alloc_inode)(fs_t *fs);
    void (*delete_inode)(fs_t *fs, inode_t *ino);
    void (*write_inode)(fs_t *fs, inode_t *ino);
};

struct file_ops {
    uint32_t (*read)(file_t *, uint32_t, uint32_t, uint8_t *);
    uint32_t (*write)(file_t *, uint32_t, uint32_t, uint8_t *);
    void     (*open)(file_t *, uint8_t,  uint8_t);
    void     (*close)(file_t *);
    off_t    (*seek)(file_t *);
};

struct fs {
    char *name;

    fs_t *(*create)(void);
    void (*destroy)(fs_t *);

    struct super_ops s_ops;
};

struct mount {
    uint32_t dev_id;
    fs_t *fs;
    dentry_t *mountpoint;
};

struct inode {
    fs_t *fs;

    uint32_t i_uid;
    uint32_t i_gid;
    uint32_t i_ino;
    uint32_t i_size;

    uint32_t impl;        /* TODO: ??? */
    uint32_t flags;
    uint32_t mask;        /* permissions */
};

struct dentry {
    uint32_t  d_flags;
    dentry_t *d_parent;
    inode_t  *d_inode;
    char d_name[VFS_NAME_MAX_LEN]; /* name of the directory */
};

struct file {
    dentry_t *f_dentry;
    inode_t  *f_inode;
    mount_t  *f_mnt;

    char f_path[VFS_NAME_MAX_LEN];

    uint32_t (*read)(file_t *, uint32_t, uint32_t, uint8_t *);
    uint32_t (*write)(file_t *, uint32_t, uint32_t, uint8_t *);
    void     (*open)(file_t *, uint8_t,  uint8_t);
    void     (*close)(file_t *);
    dentry_t *(*read_dir)(file_t *, uint32_t);
    file_t *(*find_dir)(file_t *, char *);

    struct file_ops *f_ops;
};


/* TODO:  is this needed?? */
void vfs_init(multiboot_info_t *mbinfo);

void vfs_close(file_t *node);
void vfs_open(file_t *node, uint8_t read, uint8_t write);

uint32_t vfs_read(file_t  *node, uint32_t offset, uint32_t size, uint8_t *buffer);
uint32_t vfs_write(file_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);

dentry_t *vfs_read_dir(file_t *node, uint32_t index);
file_t   *vfs_find_dir(file_t *node, char *name);

/* TODO:  */
void vfs_register_fs();
void vfs_unregister_fs();

#endif /* end of include guard: __VFS_H__ */

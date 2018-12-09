#ifndef __VFS_H__
#define __VFS_H__

#include <stdbool.h>
#include <sys/types.h>
#include <fs/multiboot.h>
#include <lib/hashmap.h>
#include <lib/list.h>
#include <kernel/common.h>

#define VFS_NAME_MAX_LEN  128
#define DENTRY_HM_LEN      16

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
typedef struct fs_type fs_type_t;
typedef struct inode inode_t;
typedef struct file file_t;
typedef struct dentry dentry_t;
typedef struct mount mount_t;
typedef struct fs_context fs_context_t;

struct inode_ops {
    inode_t *(*alloc_inode)(fs_t *fs);
    inode_t *(*lookup_inode)(fs_t *fs, void *); /* TODO:  */
    dentry_t *(*lookup_dentry)(fs_t *fs, dentry_t *, const char *); /* TODO: relocate this?? */
    void (*free_inode)(fs_t *fs, inode_t *ino);
    void (*write_inode)(fs_t *fs, inode_t *ino);
    void (*read_inode)(fs_t *fs, inode_t *ino); /* TODO: ??? */
};

struct file_ops {
    ssize_t  (*read)(file_t  *, off_t, size_t, uint8_t *);
    ssize_t  (*write)(file_t *, off_t, size_t, uint8_t *);
    file_t  *(*open)(fs_t *, inode_t *, uint8_t);
    void     (*close)(file_t *);
    off_t    (*seek)(file_t *, off_t);
};

struct fs_type {
    const char *fs_name;
    uint32_t fs_dev_id;

    fs_t *(*fs_create)(void *);
    void  (*fs_destroy)(fs_t *);
};

/* this is the file system used by the device */
struct fs {
    void *private;
    fs_type_t *fs;
    dentry_t *root;
};

struct mount {
    uint32_t dev_id;
    fs_t *fs;
    dentry_t *mountpoint;

    list_head_t mountpoints;
};

/* TODO: pad this to boundary multiple of 2 */
struct dentry {
    uint32_t  d_flags;

    dentry_t *d_parent;
    inode_t  *d_inode;
    hashmap_t *children;

    void *private; /* implementation-specific data */

    char d_name[VFS_NAME_MAX_LEN]; /* name of the directory */
};

struct inode {
    uint32_t i_uid;
    uint32_t i_gid;
    uint32_t i_ino;
    uint32_t i_size;

    uint32_t flags;
    uint32_t mask;    /* permissions */
    void *private;    /* implementation-specific data */

    struct inode_ops *i_ops;
};

struct file {
    dentry_t *f_dentry;
    inode_t  *f_inode;
    mount_t  *f_mnt;

    char f_path[VFS_NAME_MAX_LEN];

    struct file_ops *f_ops;
};

struct fs_context {
    bool unused;
};

void vfs_init(void *args);

fs_t *vfs_register_fs(const char *name , const char *mnt, void *arg);

int vfs_unregister_fs(fs_t *fs);

void vfs_list_mountpoints(void);

dentry_t *vfs_lookup(const char *path);

/* ********************************************************** */

void vfs_close(file_t *node);
void vfs_open(file_t *node, uint8_t read, uint8_t write);

file_t   *vfs_open_file(inode_t *inode);
void      vfs_close_file(file_t *file);
uint32_t  vfs_read(file_t  *node, uint32_t offset, uint32_t size, uint8_t *buffer);
uint32_t  vfs_write(file_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);

#endif /* end of include guard: __VFS_H__ */

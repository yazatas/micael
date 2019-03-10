#ifndef __VFS_H__
#define __VFS_H__

#include <fs/file.h>
#include <fs/super.h>
#include <lib/list.h>

enum FILE_MODES {
    T_IFMT  = 1 << 0,
    T_IFBLK = 1 << 1,
    T_IFCHR = 1 << 2,
    T_IFIFO = 1 << 3,
    T_IFREG = 1 << 4,
    T_IFDIR = 1 << 5,
    T_IFLNK = 1 << 6
};

enum FILE_ACCESS_MODES {
    O_EXEC   = 1 << 0,
    O_RDONLY = 1 << 1,
    O_RDWR   = 1 << 2,
    O_WRONLY = 1 << 3,
    O_APPEND = 1 << 4,
    O_CREAT  = 1 << 5,
    O_TRUNC  = 1 << 6
};

enum LOOKUP_FLAGS {
    LOOKUP_PARENT = 1 << 0,
};

typedef struct fs_type {
    char *fs_name;

    superblock_t *(*get_sb)(struct fs_type *, char *, int, void *);
    int (*kill_sb)(superblock_t *);
} fs_type_t;

typedef struct mount {
    superblock_t *mnt_sb; /* superblock of the mounted filesystem */
    dentry_t *mnt_mount;  /* dentry of the mountpoint */
    dentry_t *mnt_root;   /* dentry of the root dir of the mounted filesystem */

    char *mnt_devname;    /* device name of the filesystem (e.g. /dev/sda1) */
    char *mnt_type;       /* type of the mounted filesystem */

    list_head_t mnt_list; /* list of mountpoints */
} mount_t;

typedef struct path {
    /* TODO: what all should this struct have */
    dentry_t *p_dentry;
    int size;
} path_t;

/* - initialize emptry rootfs
 * - register all known filesystem
 * - initialize char and block device drivers
 * - initialize binfmt
 * - create dentries for /, /sys, /tmp, /dev and /proc 
 * - mount pseudo filesystems
 *   - devfs  to /dev
 *   - tmpfs  to /tmp
 *   - procfs to /proc
 *   - sysfs  to /sys */
void vfs_init(void);

/* register filesystem 
 * after registeration the filesystem can be mounted 
 *
 * return 0 on success
 * return -EINVAL if some of the given values are invalid
 * return -EEXIST if the filesystem has already been registered 
 * (see what hm_insert returns to get the full list of possible return values) */
int vfs_register_fs(fs_type_t *fs);

/* unregister filesystem (remove it from the fs_types hashmap) 
 *
 * return 0 on success, 
 * return -EINVAL if the type is invalid 
 * return -EBUSY if the filesystem is in use (mounted) */
int vfs_unregister_fs(char *type);

/* mount filesystem of type "type" to "target" from "source" using "flags" 
 *
 * @param source: points to devfs (f.ex. /dev/sda1)
 * @param target: points to the mount point of file system
 *                target must NOT contain any filesystem when vfs_mount() is called
 * @param type:   name of the mounted filesystem
 * @flags:        flags used for mounting (such as VFS_REMOUNT) 
 *
 * return 0 on success
 * return -EINVAL if some of the given parameters are invalid
 * return -ENOSYS if filesystem of type "type" has not been registered
 * return -EEXIST if "target" already has a filesystem mounted on it
 * return -ENOENT if "target" or "source" does not exist
 * return -ENOMEM if allocating mount_t object failed */
int vfs_mount(char *source, char *target, char *type, uint32_t flags);

/* install new root file system of type "type" 
 * for now this is just a temporary hack to get initrd to work
 * Release Calypso adds support for real block devices and that
 * release will remove this hack and give support for proper initrd too
 *
 * Basically this function just installs filesystem "initramfs" to current 
 * root (root_fs). After that user can execute programs from f.ex. /sbin/init
 *
 * return 0 on success 
 * return -ENOSYS if filesystem has not been registered 
 * return -EBUSY if root_fs already has a filesystem installed on it 
 * (see what initramfs_get_sb sets as errno to get the full list of possible return values) */
int vfs_install_rootfs(char *type, void *data);

/* blablabla 
 *
 *
 * NOTE: returned path must be explicitly freed by calling vfs_path_release()
 *
 * vfs_path_lookup() always returns a path_t object but on error or if the path 
 * did not resolve to anything (e.g. nothing was found) nthe path_t->p_dentry
 * is NULL and errno is set.
 *
 * If something was indeed found, path->p_dentry points to the object in question */
path_t *vfs_path_lookup(char *path, int flags);

/* 
 * return 0 on success 
 * return -EINVAL on error */
int vfs_path_release(path_t *path);

void vfs_free_fs_context(fs_ctx_t *ctx);

#endif /* end of include guard: __VFS_H__ */

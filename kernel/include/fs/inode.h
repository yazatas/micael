#ifndef __INODE_H__
#define __INODE_H__

#include <lib/list.h>
#include <sys/types.h>
#include <stdint.h>

typedef struct cdev       cdev_t;
typedef struct bdev       bdev_t;
typedef struct path       path_t;
typedef struct inode      inode_t;
typedef struct dentry     dentry_t;
typedef struct file_ops   file_ops_t;
typedef struct inode_ops  inode_ops_t;
typedef struct superblock superblock_t;

struct inode_ops {
    int (*create)(dentry_t *parent, dentry_t *dntr, int mode, path_t *path);
    inode_t *(*lookup)(dentry_t *parent, char *name);
    int (*link)(dentry_t *, dentry_t *, dentry_t *);
    int (*unlink)(dentry_t *dir, dentry_t *dntr);
    int (*symlink)(dentry_t *dir, dentry_t *dntr, char *symname);
    int (*mkdir)(dentry_t *dir, dentry_t *dentry, int mode);
    int (*rmdir)(dentry_t *dir, dentry_t *dentry);
    int (*mknod)(dentry_t *dir, dentry_t *dntr, int mode, dev_t rdev);
    int (*rename)(dentry_t *old_dir, dentry_t *old_dentry, dentry_t *new_dir, dentry_t *new_dentry);
    int (*follow_link)(inode_t *ino, path_t *path);
    int (*put_link)(dentry_t *dntr, path_t *path);
    int (*truncate)(inode_t *ino);
    int (*permission)(inode_t *ino, int mask, path_t *path);
};

struct inode {
    int i_uid;
    int i_gid;
    int i_ino;
    int i_size;

    int i_count;        /* reference counter */

    uint32_t i_flags;   /* inode flags */
    uint32_t i_mask;    /* permissions */
    void *i_private;    /* implementation-specific data */

    superblock_t *i_sb; /* pointer to superblock */

    inode_ops_t *i_iops; /* pointer to inode operations */
    file_ops_t  *i_fops; /* pointer to file operations */

    cdev_t *i_cdev;     /* pointer to cdev if inode is a char device */
    bdev_t *i_bdev;     /* pointer to bdev if inode is a block device */

    list_head_t i_list;  /* list of all inodes */
    list_head_t i_dirty; /* list of dirty inodes */
};

int inode_init(void);

/* allocate empty inode object */
inode_t *inode_generic_alloc(uint32_t flags);
int      inode_generic_dealloc(inode_t *ino);

inode_t *inode_lookup(dentry_t *ino, char *name);

#endif /* __INODE_H__ */

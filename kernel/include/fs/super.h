#ifndef __SUPER_H__
#define __SUPER_H__

#include <lib/list.h>
#include <fs/block.h>

/* TODO: remove and include proper files */
typedef struct fs_type fs_type_t;
typedef struct inode inode_t;
typedef struct dentry dentry_t;
typedef uint32_t dev_t;

typedef struct superblock superblock_t;
typedef struct super_ops  super_ops_t;

struct super_ops {
    inode_t *(*alloc_inode)(superblock_t *); /* allocate inode object */
    int (*destroy_inode)(inode_t *);         /* destroy object inode */
    int (*read_inode)(inode_t *);            /* fill the the fields of inode object (using i_ino) */
    int (*dirty_inode)(inode_t *);           /* mark the inode as dirty (used by journaling file systems) */
    int (*write_inode)(inode_t *);           /* write the inode to disk */
    int (*put_inode)(inode_t *);             /* release inode (TODO: needed?) */
    int (*delete_inode)(inode_t *);          /* delete inode from memory and disk */

    int (*put_super)(superblock_t *);        /* release superblock */
    int (*write_super)(superblock_t *);      /* update super on the disk */
    int (*sync_fs)(superblock_t *);          /* flush the filesystem to disk */
};

/* TODO: why does this struct have two lists? */
struct superblock {
    list_head_t s_list;
    dev_t s_dev;
    fs_type_t *s_type;

    uint32_t s_blocksize;

    uint32_t s_flags; /* mount flaags */
    uint32_t s_magic; /* file system magic (TODO: remove?) */

    dentry_t *s_root; /* dentry of the root directory */

    int s_count; /* reference counter */
    int s_dirty; /* marked if superblock needs flushing */

    super_ops_t *s_ops; /* superblock operations */

    list_head_t s_ino;       /* list of all inodes */
    list_head_t s_ino_dirty; /* list of dirty inodes */

    bdev_t *s_bdev; /* pointer to block device driver descriptor */

    void *s_private; /* file system specific info */

    char *s_id; /* name of the block device containing the superblock */

    list_head_t s_instances; /* TODO */
};

/* for pseudo filesystems with no mountpoint (pipefs)*/
superblock_t *
super_get_sb_pseudo(fs_type_t *type, char *dev, int flags,
                    void *data, int (*fill_super)(superblock_t *));

/* for pseudo filesystems with a single mountpoint (procfs, devfs) */
superblock_t *
super_get_sb_single(fs_type_t *type, char *dev, int flags,
                    void *data, int(*fill_super)(superblock_t *));

/* for pseudo filesystesms with multiple mounpoints (tmpfs) */
superblock_t *
super_get_sb_nodev(fs_type_t *type, char *dev, int flags,
                   void *data, int (*fill_super)(superblock_t *));

/* for normal block-based filesystems */
superblock_t *
super_get_sb_bdev(fs_type_t *type, char *dev, int flags,
                  void *data, int (*fill_super)(superblock_t *));

#endif /* __SUPER_H__ */

#ifndef __INODE_H__
#define __INODE_H__

#include <stdint.h>

typedef struct inode     inode_t;
typedef struct inode_ops inode_ops_t;
typedef struct file_ops  file_ops_t;

struct inode_ops {
    /* inode_t *(*alloc_inode)(fs_t *fs); */
    /* inode_t *(*lookup_inode)(fs_t *fs, void *); /1* TODO:  *1/ */
    /* dentry_t *(*lookup_dentry)(fs_t *fs, dentry_t *, const char *); /1* TODO: relocate this?? *1/ */
    /* void (*free_inode)(fs_t *fs, inode_t *ino); */
    /* void (*write_inode)(fs_t *fs, inode_t *ino); */
    /* void (*read_inode)(fs_t *fs, inode_t *ino); /1* TODO: ??? *1/ */
    int value;
};

struct inode {
    uint32_t i_uid;
    uint32_t i_gid;
    uint32_t i_ino;
    uint32_t i_size;

    int i_count;        /* reference counter */

    uint32_t i_flags;   /* inode flags */
    uint32_t i_mask;    /* permissions */
    void *i_private;    /* implementation-specific data */

    inode_ops_t *i_ops;
    file_ops_t  *f_ops;
};

inode_t *inode_alloc();
int      inode_dealloc(inode_t *ino);

#endif /* __INODE_H__ */

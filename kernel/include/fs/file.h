#ifndef __FILE_H__
#define __FILE_H__


#include <fs/dentry.h>
#include <kernel/common.h>
#include <sys/types.h>

typedef struct file_ops file_ops_t;
typedef struct dentry dentry_t;
typedef struct file file_t;

struct file_ops {
    ssize_t  (*read)(file_t  *, off_t, size_t, void *);
    ssize_t  (*write)(file_t *, off_t, size_t, void *);
    file_t  *(*open)(dentry_t *, uint8_t);
    void     (*close)(file_t *);
    int      (*seek)(file_t *, off_t);
};

struct file {
    dentry_t *f_dentry;

    void *private;
    size_t refcount;
    off_t offset;

    struct file_ops *f_ops;
};

struct fs_context {
    int count;      /* number of processes sharing this struct */
    dentry_t *root; /* dentry of the root directory */
    dentry_t *pwd;  /* dentry of current working directory */
};

struct file_context {
    int count;        /* number of processes sharing this struct */
    int numfd;        /* number of file descriptors */
    struct file **fd; /* pointer to an array of file objects */
};

file_t *vfs_fget(int fd);
int vfs_fput(file_t *file);

file_t *vfs_fget_light(int fd);
int vfs_fput_light(file_t *file);

file_t   *vfs_open_file(dentry_t *dentry);

/* TODO: comment */
void     vfs_close_file(file_t *file);

/* read "size" bytes from file to "buffer" starting at "offset"
 *
 * return number of bytes read on success and -errno on error */
ssize_t  vfs_read(file_t  *file, off_t offset, size_t size, void *buffer);

/* write "size" bytes to "file" from "buffer" starting at "offset"
 *
 * return number of bytes written on success and -errno on error */
ssize_t  vfs_write(file_t *file, off_t offset, size_t size, void *buffer);

/* TODO: comment */
int      vfs_seek(file_t  *file, off_t off);

#endif /* __FILE_H__ */

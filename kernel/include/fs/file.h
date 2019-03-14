#ifndef __FILE_H__
#define __FILE_H__

#include <kernel/common.h>
#include <sys/types.h>

typedef struct file file_t;
typedef struct dentry dentry_t;
typedef struct fs_context fs_ctx_t;
typedef struct file_ops file_ops_t;
typedef struct file_context file_ctx_t;

struct file_ops {
    ssize_t  (*read)(file_t  *, off_t, size_t, void *);
    ssize_t  (*write)(file_t *, off_t, size_t, void *);
    file_t  *(*open)(dentry_t *, int);
    int      (*close)(file_t *);
    int      (*seek)(file_t *, off_t);
};

struct file {
    dentry_t *f_dentry;

    void *f_private;
    size_t f_count;
    off_t f_pos;
    int f_mode; /* TODO: mode_t? */

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

void file_init(void);

file_t *file_generic_alloc(void);
int file_generic_dealloc(file_t *file);

/* generic seek operation. Filesystems can use this
 * if the file seeking doesn't require more than
 * than updating the "f_pos" field of the file object
 *
 * return 0 on success
 * return -EINVAL if "file" is NULL
 * return -ESPIPE if the seek is illegal (off bounds) */
int file_generic_seek(file_t *file, off_t off);

/* TODO: comment */
file_t *file_fget(int fd);

/* TODO: comment */
int file_fput(file_t *file);

/* TODO: comment */
file_t *file_fget_light(int fd);

/* TODO: comment */
int file_fput_light(file_t *file);

/* TODO: comment */
file_t *file_open(dentry_t *dentry, int mode);

/* TODO: comment */
void file_close(file_t *file);

/* TODO: comment */
ssize_t file_read(file_t  *file, off_t offset, size_t size, void *buffer);

/* TODO: comment */
ssize_t file_write(file_t *file, off_t offset, size_t size, void *buffer);

#endif /* __FILE_H__ */

#include <drivers/ps2.h>
#include <drivers/tty.h>
#include <drivers/vbe.h>
#include <fs/devfs.h>
#include <fs/file.h>
#include <kernel/util.h>
#include <mm/heap.h>
#include <errno.h>

void tty_putc(char c)
{
    vbe_put_char(c);
}

void tty_puts(char *data)
{
    vbe_put_str(data);
}

static char *__tty_get_name(void)
{
    /* TODO: ??? */
    return strdup("tty1");
}

static file_t *__tty_open(dentry_t *dntr, int mode)
{
    if (!dntr) {
        errno = EINVAL;
        return NULL;
    }

    if (mode & (O_EXEC | O_CREAT | O_TRUNC)) {
        errno = ENOTSUP;
        return NULL;
    }

    file_t *file = file_generic_alloc();

    if (!file)
        return NULL;

    file->f_ops  = dntr->d_inode->i_fops;
    file->f_mode = mode;

    dntr->d_inode->i_count++;

    return file;
}

static int __tty_close(file_t *file)
{
    return file_generic_dealloc(file);
}

static ssize_t __tty_write(file_t *file, off_t offset, size_t size, void *buf)
{
    (void)offset;

    if (!file || !buf)
        return -EINVAL;

    if (file->f_mode & O_RDONLY)
        return -ENOTSUP;

    for (size_t i = 0; i < size; ++i) {
        vbe_put_char(((char *)buf)[i]);
    }

    return (ssize_t)size;
}

static ssize_t __tty_read(file_t *file, off_t offset, size_t size, void *buf)
{
    if (!file || !buf)
        return -EINVAL;

    if (file->f_mode & O_WRONLY)
        return -ENOTSUP;

    for (size_t i = 0; i < size; ++i) {
        char c = ps2_read_next();

        if (c == '\n')
            break;

        ((char *)buf)[i] = c;
        kprint("%c", ((char *)buf)[i]);
    }
    kprint("\n");

    return size;
}

tty_t *tty_init(void)
{
    int ret         = 0;
    tty_t *tty      = NULL;
    file_ops_t *ops = NULL;

    if ((tty = kmalloc(sizeof(tty_t))) == NULL)
        return NULL;

    if ((ops = kmalloc(sizeof(file_ops_t))) == NULL)
        return NULL;

    ops->read  = __tty_read;
    ops->write = __tty_write;
    ops->open  = __tty_open;
    ops->close = __tty_close;
    ops->seek  = NULL;

    tty->name = __tty_get_name();

    if ((tty->dev = cdev_alloc(tty->name, ops, 0)) == NULL)
        goto error;

    if ((ret = devfs_register_cdev(tty->dev, tty->name)) < 0) {
        errno = -ret;
        goto error_cdev;
    }

    return tty;

error_cdev:
    cdev_dealloc(tty->dev);

error:
    kfree(ops);
    kfree(tty);

    return NULL;
}

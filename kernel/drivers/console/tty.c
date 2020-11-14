#include <drivers/gfx/vbe.h>
#include <drivers/gfx/vga.h>
#include <drivers/console/ps2.h>
#include <drivers/console/tty.h>
#include <fs/devfs.h>
#include <fs/file.h>
#include <kernel/percpu.h>
#include <kernel/util.h>
#include <mm/heap.h>
#include <sync/spinlock.h>
#include <errno.h>
#include <stdbool.h>

/* Use the VGA routines by default.
 * When VBE is initialized, new routines will be installed */
void (*__tty_putc)(char)   = vga_put_char;
void (*__tty_puts)(char *) = vga_put_str;

void tty_putc(char c)
{
    __tty_putc(c);
}

void tty_puts(char *data)
{
    __tty_puts(data);
}

void tty_install_putc(void (*_tty_putc)(char c))
{
    if (_tty_putc)
        __tty_putc = _tty_putc;
}

void tty_install_puts(void (*_tty_puts)(char *s))
{
    if (_tty_puts)
        __tty_puts = _tty_puts;
}

static char *__tty_get_name(void)
{
    /* TODO: ??? */
    return kstrdup("tty1");
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

    path_t *path = NULL;

    if ((path = vfs_path_lookup("/dev/kbd", LOOKUP_OPEN))->p_status != LOOKUP_STAT_SUCCESS) {
        kdebug("/dev/kbd not found!");
        errno = ENOENT;
        return NULL;
    }

    if ((file->f_private = file_open(path->p_dentry, O_RDONLY)) == NULL) {
        kdebug("Failed to open /dev/kbd");
        vfs_path_release(path);
        return NULL;
    }

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

    static spinlock_t lock = 0;
    spin_acquire(&lock);

    for (size_t i = 0; i < size; ++i) {
        vbe_put_char(((char *)buf)[i]);
    }
    spin_release(&lock);

    return (ssize_t)size;
}

static ssize_t __tty_read(file_t *file, off_t offset, size_t size, void *buf)
{
    (void)offset;

    if (!file || !file->f_private || !buf)
        return -EINVAL;

    if (file->f_mode & O_WRONLY)
        return -ENOTSUP;

    size_t i = 0;
    char c   = 0;

    while (c != '\n' && i < size) {
        file_read(file->f_private, 0, 1, &c);

        if (c == '\n') {
            kprint("\n");
            break;
        }

        if (c == '\b') {
            if (i != 0) {
                ((char *)buf)[--i] = '\0';
                kprint("\b");
            }
            continue;
        }

        ((char *)buf)[i] = c;
        kprint("%c", ((char *)buf)[i]);
        i++;
    }

    return (ssize_t)i;
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

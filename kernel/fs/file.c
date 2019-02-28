#include <fs/file.h>
#include <fs/fs.h>
#include <errno.h>

/* TODO: move these to file.c?? */
file_t *vfs_open_file(dentry_t *dntr)
{
    if (!dntr) {
        errno = EINVAL;
        return NULL;
    }

    if (!dntr->d_inode->f_ops->open) {
        errno = ENOSYS;
        return NULL;
    }

    return dntr->d_inode->f_ops->open(dntr, O_RDWR);
}

void vfs_close_file(file_t *file)
{
    if (!file) {
        errno = EINVAL;
        return;
    }

    if (!file->f_ops->close) {
        errno = ENOSYS;
        return;
    }

    file->f_ops->close(file);
}

int vfs_seek(file_t *file, off_t off)
{
    if (!file)
        return -EINVAL;

    if (!file->f_ops->seek)
        return -ENOSYS;

    return file->f_ops->seek(file, off);
}

ssize_t vfs_read(file_t *file, off_t offset, size_t size, void *buffer)
{
    if (!file || !buffer)
        return -EINVAL;

    if (!file->f_ops->read)
        return -ENOSYS;

    return file->f_ops->read(file, offset, size, buffer);
}

ssize_t vfs_write(file_t *file, off_t offset, size_t size, void *buffer)
{
    if (!file || !buffer)
        return -EINVAL;

    if (!file->f_ops->write)
        return -ENOSYS;

    return file->f_ops->write(file, offset, size, buffer);
}

#if 0
The kernel provides an fget( ) function to be invoked when the kernel starts using a
file object. This function receives as its parameter a file descriptor fd . It returns the
address in current->files->fd[fd] (that is, the address of the corresponding file
object), or NULL if no file corresponds to fd . In the first case, fget( ) increases the file
object usage counter f_count by 1.
The kernel also provides an fput( ) function to be invoked when a kernel control
path finishes using a file object. This function receives as its parameter the address of
a file object and decreases its usage counter, f_count. Moreover, if this field becomes
0, the function invokes the release method of the file operations (if defined),
decreases the i_writecount field in the inode object (if the file was opened for writ-
ing), removes the file object from the superblock’s list, releases the file object to the
slab allocator, and decreases the usage counters of the associated dentry object and
of the filesystem descriptor (see the later section “Filesystem Mounting).
The fget_light() and fput_light() functions are faster versions of fget() and fput(
): the kernel uses them when it can safely assume that the current process already
owns the file object—that is, the process has already previously increased the file
object’s reference counter. For instance, they are used by the service routines of the
#endif


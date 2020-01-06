#include <fs/file.h>
#include <fs/pipe.h>
#include <kernel/common.h>
#include <kernel/kprint.h>
#include <kernel/util.h>
#include <mm/heap.h>
#include <mm/slab.h>
#include <sched/sched.h>
#include <sync/spinlock.h>
#include <sync/wait.h>
#include <errno.h>

struct pipe {
    file_t *file;                 /* underlying file object of the pipe */

    spinlock_t lock;              /* spinlock protecting the pipe */

    void *mem;                    /* buffer holding the pipe data */
    size_t size;                  /* size of the pipe in bytes */
    size_t ptr;                   /* read/write ptr of the pipe */

    size_t nreaders;              /* number of readers on this pipe */
    size_t nwriters;              /* number of writers on this pipe */

    wait_queue_head_t wq_readers; /* wait queue for readers, writer will call wq_wakeup() */
    wait_queue_head_t wq_writers; /* wait queue for writers, reader will call wq_wakeup() */
};

static mm_cache_t *p_cache = NULL;

static ssize_t __read(file_t *file, off_t offset, size_t size, void *buffer)
{
    (void)offset;

    if (!file || !file->f_private || !buffer || size == 0)
        return -EINVAL;

    pipe_t *pipe = file->f_private;
    size_t nread = 0;

    spin_acquire(&pipe->lock);

    while (READ_ONCE(pipe->ptr) == 0) {
        task_t *cur = sched_get_active();
        wq_wait_event(&pipe->wq_readers, cur, &pipe->lock);
    }

    nread = (size < pipe->ptr) ? size : pipe->ptr;
    kmemcpy(buffer, pipe->mem, nread);

    if (size >= pipe->ptr) {
        pipe->ptr = 0;
    } else {
        kmemmove(pipe->mem, (uint8_t *)pipe->mem + size, pipe->ptr - nread);
        pipe->ptr = size + pipe->ptr - nread;
    }

    spin_release(&pipe->lock);

    return nread;
}

static ssize_t __write(file_t *file, off_t offset, size_t size, void *buffer)
{
    (void)offset;

    if (!file || !file->f_private || !buffer || size == 0)
        return -EINVAL;

    pipe_t *pipe = file->f_private;

    spin_acquire_irq(&pipe->lock);

    /* TODO: make this a while loop */

    if (pipe->size - pipe->ptr < size) {
        task_t *cur = sched_get_active();
        wq_wait_event(&pipe->wq_writers, cur, &pipe->lock);

        /* wq_wait_event() will block (putting "cur" to sleep) until
         * some other task will trigger wq_wakeup() on pipe->wq_writers
         *
         * We give pipe->lock to wq_wait_event() which means that it must
         * take care of releasing the lock before we're put to sleep. */
    }

    kmemcpy((uint8_t *)pipe->mem + pipe->ptr, buffer, size);
    pipe->ptr += size;

    /* pipe now contains unread data, wake up all tasks waiting on the pipe */
    wq_wakeup(&pipe->wq_readers);
    spin_release_irq(&pipe->lock);

    return size;
}

static file_t *__open(dentry_t *dntr, int mode)
{
    if (!dntr || !dntr->d_private) {
        errno = EINVAL;
        return NULL;
    }

    pipe_t *pipe = dntr->d_private;

    spin_acquire(&pipe->lock);

    if (mode == O_WRONLY)
        pipe->nwriters++;
    else if (mode == O_RDONLY)
        pipe->nreaders++;
    else
        kdebug("invalid mode given for pipe_open()");
        /* TODO: error handling? */

    spin_release(&pipe->lock);
    return NULL;
}

static int __close(file_t *file)
{
    if (!file)
        return -EINVAL;

    pipe_t *pipe = file->f_private;

    spin_acquire(&pipe->lock);

    if (file->f_mode == O_WRONLY)
        pipe->nwriters--;
    else if (file->f_mode == O_RDONLY)
        pipe->nreaders--;
    else
        kdebug("invalid mode given for pipe_open()");
        /* TODO: error handling? */

    spin_release(&pipe->lock);

    return 0;
}

int pipe_init(void)
{
    if ((p_cache = mmu_cache_create(sizeof(pipe_t), 0)) == NULL) {
        kdebug("Failed to allocate SLAB cache for pipe!");
        return -ENOMEM;
    }

    return 0;
}

pipe_t *pipe_create(size_t size)
{
    if (!p_cache || size == 0) {
        errno = EINVAL;
        return NULL;
    }

    pipe_t *pipe = mmu_cache_alloc_entry(p_cache, 0);

    if (!pipe)
        return NULL;

    if ((pipe->file = file_generic_alloc()) == NULL)
        return NULL;

    pipe->file->f_ops->open  = __open;
    pipe->file->f_ops->close = __close;
    pipe->file->f_ops->read  = __read;
    pipe->file->f_ops->write = __write;

    /* save the pipe object to both file and dentry private data */
    pipe->file->f_private           = pipe;
    pipe->file->f_dentry->d_private = pipe;

    pipe->mem  = kmalloc(size);
    pipe->size = size;
    pipe->lock = 0;

    wq_init_head(&pipe->wq_readers);
    wq_init_head(&pipe->wq_writers);

    return pipe;
}

int pipe_open(pipe_t *pipe, int mode)
{
    if (!pipe)
        return -EINVAL;

    pipe->file->f_mode = mode;
    (void)pipe->file->f_ops->open(pipe->file->f_dentry, mode);

    return 0;
}

int pipe_close(pipe_t *pipe)
{
    if (!pipe)
        return -EINVAL;

    return pipe->file->f_ops->close(pipe->file);
}

ssize_t pipe_read(pipe_t *pipe, void *buffer, size_t size)
{
    if (!pipe || !buffer || size == 0)
        return -EINVAL;

    return pipe->file->f_ops->read(pipe->file, 0, size, buffer);
}

ssize_t pipe_write(pipe_t *pipe, void *buffer, size_t size)
{
    if (!pipe || !buffer || size == 0)
        return -EINVAL;

    return pipe->file->f_ops->write(pipe->file, 0, size, buffer);
}

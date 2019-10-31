#include <kernel/wait.h>
#include <lib/ringbuffer.h>
#include <mm/heap.h>
#include <mm/slab.h>
#include <sync/spinlock.h>
#include <errno.h>

struct ringbuffer {
    uint8_t *data;
    size_t size;

    size_t w_ptr;
    size_t r_ptr;

    spinlock_t w_spin;
    spinlock_t r_spin;
};

static mm_cache_t *rb_cache = NULL;

static inline void __update_w_ptr(ringbuffer_t *rb)
{
    rb->w_ptr++;

    if (rb->w_ptr >= rb->size)
        rb->w_ptr = 0;
}

static inline void __update_r_ptr(ringbuffer_t *rb)
{
    rb->r_ptr++;

    if (rb->r_ptr >= rb->size)
        rb->r_ptr = 0;
}

ringbuffer_t *rb_create(size_t size)
{
    if (rb_cache == NULL) {
        if ((rb_cache = mmu_cache_create(sizeof(ringbuffer_t), MM_NO_FLAG)) == NULL) {
            kdebug("Failed to create SLAB cache for ringbuffer!");

            errno = ENOMEM;
            return NULL;
        }
    }

    if (size == 0) {
        errno = EINVAL;
        return NULL;
    }

    ringbuffer_t *rb = NULL;

    if ((rb = kmalloc(sizeof(ringbuffer_t))) == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    if ((rb->data = kmalloc(size)) == NULL) {
        kfree(rb);
        errno = ENOMEM;
        return NULL;
    }

    rb->size  = size;
    rb->w_ptr = 0;
    rb->r_ptr = 0;

    return rb;
}

int rb_destroy(ringbuffer_t *rb)
{
    if (!rb)
        return -EINVAL;

    if (rb->data)
        kfree(rb->data);
    kfree(rb);

    return 0;
}

ssize_t rb_read(ringbuffer_t *rb, uint8_t *buffer, size_t size)
{
    if (!rb || !rb->data || !buffer || size == 0)
        return -EINVAL;

    size_t read = 0;
    size_t start = rb->r_ptr;

    spin_acquire(&rb->r_spin);

    while (read < size) {
        buffer[read++] = rb->data[rb->r_ptr];
        __update_r_ptr(rb);
    }

    spin_release(&rb->r_spin);

    return size;
}

ssize_t rb_write(ringbuffer_t *rb, uint8_t *buffer, size_t size)
{
    if (!rb || !rb->data || !buffer || size == 0)
        return -EINVAL;

    size_t written = 0;
    size_t start   = rb->w_ptr;

    spin_acquire(&rb->w_spin);

    while (written < size) {
        rb->data[rb->w_ptr] = buffer[written++];
        __update_w_ptr(rb);
    }

    spin_release(&rb->w_spin);

    return size;
}

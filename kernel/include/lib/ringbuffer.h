#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__

#include <sys/types.h>

typedef struct ringbuffer ringbuffer_t;

/* typedef int ssize_t; */

/* Create new ring buffer holding "size" bytes
 *
 * Return pointer to ringer buffer on success
 * Return NULL on error and set errno to
 *   EINVAL if "size" is 0
 *   ENOMEM if malloc() failed */
ringbuffer_t *rb_create(size_t size);

/* Destroy cache and underlying memory
 *
 * Return 0 success
 * Return -EINVAL if "rb" is NULL
 * Return -EBUSY if wait queues are not empty */
int rb_destroy(ringbuffer_t *rb);

/* Read "size" bytes from ringbuffer "rb" to "buffer"
 *
 * Return the bytes read on success ("size")
 * Return -EINVAL if one of the parameters are invalid */
ssize_t rb_read(ringbuffer_t *rb, uint8_t *buffer, size_t size);

/* Write "size" bytes from "buffer" to ringbuffer "rb"
 *
 * Return bytes written on success ("size")
 * Return -EINVAL if one of the parameters are invalid */
ssize_t rb_write(ringbuffer_t *rb, uint8_t *buffer, size_t size);

#endif /* __RING_BUFFER_H__ */

#ifndef __PIPE_H__
#define __PIPE_H__

typedef struct pipe pipe_t;

/* Initialize the SLAB cache for pipes
 *
 * Return 0 on success
 * Return -ENOMEM if SLAB couldn't be allocated */
int pipe_init(void);

/* Create a new anonymous pipe of "size" bytes
 *
 * Return pointer to pipe on success
 * Return NULL on error and set errno to:
 *   EINVAL if "size" is 0
 *   ENOMEM if new pipe object cannot be created */
pipe_t *pipe_create(size_t size);

/* Destroy pipe and release all memory
 *
 * Return 0 on success
 * Return -EINVAL if "pipe" is NULL
 * Return -EBUSY if the # of users of pipe > 1 */
void pipe_destroy(pipe_t *pipe);

/* Open the pipe objected pointed to by "pipe"
 * "flags" must be either O_RDONLY or O_WRONLY but not both
 *
 * Return 0 on success
 * Return -EINVAL if "mode" is invalid */
int pipe_open(pipe_t *pipe, int mode);

/* Close the pipe
 *
 * Return 0 on success
 * Return -EINVAL if "pipe" is NULL */
int pipe_close(pipe_t *pipe);

/* Read "size" bytes from "pipe" to "buffer"
 *
 * pipe_read() will block if there are no bytes in the pipe
 *
 * pipe_read() will also block if there are no writers for the pipe
 *
 * pipe_read() will return at most "size" bytes, it will NOT
 * block even if the pipe didn't have "size" bytes
 *
 * pipe_read() drains the pipe ie. it becomes available for
 * writing if it was full.
 *
 * Return the number of bytes read on success
 * Return -EINVAL if one of the arguments is invalid */
ssize_t pipe_read(pipe_t *pipe, void *buffer, size_t size);

/* Write "size" bytes from "buffer" to "pipe"
 *
 * If "pipe" is full, pipe_write() will block until it more
 * space becomes available.
 *
 * pipe_write() does an atomic write ie. it either fully writes
 * the contents of "buffer" to "pipe" or not at all.
 *
 * Because the writes are atomic, a write to pipe must not be
 * larger than the capacity of pipe. This means that if the pipe's
 * capacity is 128 but caller wants to write 256 bytes, he must use
 * two calls to pipe_write()
 *
 * Return the number of bytes written written on success ("size)
 * Return -EINVAL if one the parameters is invalid
 * Return -ENOMEM if "size" is larger than pipe's capacity */
ssize_t pipe_write(pipe_t *pipe, void *buffer, size_t size);

#endif /* __PIPE_H__ */

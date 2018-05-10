#ifndef __LIBC_SYSCALLS_H__
#define __LIBC_SYSCALLS_H__

#include <stddef.h>

int read(int fd, char *buf, size_t size);
int write(int fd, char *buf, size_t size);

#endif /* end of include guard: __LIBC_SYSCALLS_H__ */

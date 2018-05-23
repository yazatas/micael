#ifndef __UNISTD_H__
#define __UNISTD_H__

#include <stddef.h>

typedef int pid_t;

pid_t fork(void);
int read(int fd, char *buf, size_t size);
int write(int fd, char *buf, size_t size);

#endif /* end of include guard: __UNISTD_H__ */

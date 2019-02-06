#ifndef __ERRNO_H__
#define __ERRNO_H__

int errno;

#define ENOSPC 1
#define ENOENT 2
#define EINVAL 3
#define EEXIST 4
#define ENOMEM 5
#define EBUSY  6
#define E2BIG  7  /* Argument list too long */
#define ESPIPE 29 /* Illegal seek */
#define ENOSYS 38 /* Funcion not implemented */


#endif /* __ERRNO_H__ */

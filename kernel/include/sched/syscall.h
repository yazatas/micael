#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include <kernel/common.h>

uint32_t syscall_handler(void *ctx);

#endif /* end of include guard: __SYSCALL_H__ */

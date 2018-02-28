#ifndef __KPRINT_H__
#define __KPRINT_H__

#include <stdarg.h>

/* not pretty if more than one line is printed */
#define kdebug(fmt, ...) kprint("[%s] "fmt"\n", __func__, ##__VA_ARGS__)

void kprint(const char *fmt, ...);

#endif /* end of include guard: __KPRINT_H__ */

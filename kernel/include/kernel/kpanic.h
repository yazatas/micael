#ifndef __KPANIC_H__
#define __KPANIC_H__

#include <stdarg.h>

/* TODO: global error number better than string */
void kpanic(const char *err);

#endif /* end of include guard: __KPANIC_H__ */

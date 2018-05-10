#ifndef _STDIO_H
#define _STDIO_H 1

#include <sys/cdefs.h>

#define EOF   (-1)
#define stdin  0
#define stdout 1
#define stderr 2

#ifdef __cplusplus
extern "C" {
#endif

int printf(const char *fmt, ...);
int fprintf(int fd, const char *fmt, ...);
int putchar(int c);
int puts(const char* fmt);

#ifdef __cplusplus
}
#endif

#endif

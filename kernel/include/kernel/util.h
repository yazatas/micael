#ifndef __STR_UTIL_H__
#define __STR_UTIL_H__

#include <stddef.h>

void *memcpy(void *restrict dstptr, const void *restrict srcptr, size_t size);
void *memset(void *buf, int c, size_t size);
void *memmove(void *dstptr, const void *srcptr, size_t size);

char *strdup(const char *s);
char *strchr(const char *s, int c);
char *strsep(char **str, const char *delim);
char *strncpy(char *restrict dest, const char *restrict src, size_t n);
char *strncat(char *s1, char *s2, size_t len);
char *strscat(char *s1, char *s2);

int strcmp(const char *s1, const char *s2);
int strscmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t len);
size_t strlen(const char *str);

#endif /* __STR_UTIL_H__ */

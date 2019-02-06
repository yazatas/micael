#include <kernel/util.h>
#include <stdbool.h>

size_t strlen(const char *str)
{
    size_t len = 0;

    while (str[len] != '\0') {
        len++;
    }

    return len;
}

void *memcpy(void *restrict dstptr, const void *restrict srcptr, size_t size)
{
    unsigned char *dst = dstptr;
    const unsigned char *src = srcptr;

    for (size_t i = 0; i < size; ++i) {
        dst[i] = src[i];
    }

    return dstptr;
}

void *memset(void *buf, int c, size_t size)
{
    unsigned char *ptr = buf;

    while (size--) {
        *ptr++ = c;
    }

    return buf;
}

void *memmove(void *dstptr, const void *srcptr, size_t size)
{
    uint8_t *dst = dstptr;
    const uint8_t *src = srcptr;

    if (dst < src) {
        for (size_t i = 0; i < size; i++)
            dst[i] = src[i];
    } else {
        for (size_t i = size; i != 0; i--)
            dst[i-1] = src[i-1];
    }
    return dstptr;
}

char *strdup(const char *s)
{
    size_t n  = strlen(s), i;
    char *new = kmalloc(n + 1);

    if (!new)
        return NULL;

    for (i = 0; i < n; ++i) {
        new[i] = s[i];
    }
    new[i] = '\0';

    return new;
}

int strncmp(const char *s1, const char *s2, size_t len)
{
    size_t i;

    for (i = 0; s1[i] != '\0' && s2[i] != '\0' && i < len; ++i) {
        if (s1[i] < s2[i])
            return -1;
        else if (s1[i] > s2[i])
            return 1;
    }

    return !(s1[i] == s2[i]);
}

int strcmp(const char *s1, const char *s2)
{
    for (size_t i = 0; s1[i] != '\0' || s2[i] != '\0'; ++i) {
        if (s1[i] < s2[i])
            return -1;
        else if (s1[i] > s2[i])
            return 1;
    }
    return 0;
}

char *strncpy(char *restrict dest, const char *restrict src, size_t n)
{
    size_t i;

    for (i = 0; i < n && src[i] != '\0'; ++i)
        dest[i] = src[i];
    for (; i < n; ++i)
        dest[i] = '\0';

    return dest;
}

char *strchr(const char *s, int c)
{
    for (size_t i = 0; s[i] != '\0'; ++i) {
        if (s[i] == c)
            return (char *)&s[i];
    }
    return NULL;
}

static bool char_in_delim(char c, const char *delim)
{
    while (*delim != '\0') {
        if (*delim++ == c)
            return true;
    }
    return false;
}

static char *find_first_non_delim(const char *str, const char *delim)
{
    while (char_in_delim(*str, delim))
        str++;
    return (char *)str;
}

char *strsep(char **str, const char *delim)
{
    char *s, *tok, *c;

    if (*str == NULL)
        return NULL;

    s = tok = find_first_non_delim(*str, delim);
    
    while (*s != '\0') {
        c = (char *)delim;

        while (*c != '\0') {
            if (*s == *c) {
                *s = '\0';
                *str = s + 1;
                return tok;
            }
            c++;
        }
        s++;
    }

    *str = NULL;
    return tok;
}

#ifndef __TYPES_H__
#define __TYPES_H__

#include <stddef.h>
#include <stdint.h>

#if 0
#ifdef __x86_64__
typedef uint64_t unsigned long int;
typedef uint32_t unsigned int;

typedef uint64_t size_t;
typedef int64_t ssize_t;
typedef off_t int64_t;
#else
typedef unsigned long int uint32_t;
typedef unsigned int uint32_t;

typedef size_t   uint64_t ;
typedef ssize_t  int64_t;
#endif
#endif

typedef uint32_t dev_t;
typedef int32_t  off_t;

#endif /* __TYPES_H__ */

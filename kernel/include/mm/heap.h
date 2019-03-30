#ifndef __KERNEL_HEAP_H__
#define __KERNEL_HEAP_H__

#include <stddef.h>
#include <stdint.h>

void *kmalloc(size_t size);
void *kzalloc(size_t size);
void *kcalloc(size_t nmemb, size_t size);
void *krealloc(void *ptr, size_t size);
void  kfree(void *ptr);

void heap_initialize(uint32_t *heap_start);

#endif /* end of include guard: __KERNEL_HEAP_H__ */

#ifndef __KHEAP_H__
#define __KHEAP_H__

#include <stddef.h>
#include <stdint.h>

void *kmalloc(size_t size);
void *kcalloc(size_t nmemb, size_t size);
void *krealloc(void *ptr, size_t size);
void  kfree(void *ptr);

void kheap_initialize(uint32_t *heap_start);

#endif /* end of include guard: __KHEAP_H__ */

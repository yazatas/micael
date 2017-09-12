#ifndef __KHEAP_H__
#define __KHEAP_H__

void kheap_init();

void *kmalloc(size_t size);
void *kcalloc(size_t nmemb, size_t size);
void *krealloc(void *ptr, size_t size);

void kfree(void *ptr);
void traverse(); /* TODO: remove */

#endif /* end of include guard: __KHEAP_H__ */

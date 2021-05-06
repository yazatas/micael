#ifndef __KERNEL_HEAP_H__
#define __KERNEL_HEAP_H__

#include <stddef.h>
#include <stdint.h>

void *kmalloc(size_t size, int flags);
void *kzalloc(size_t size);
void *kcalloc(size_t nmemb, size_t size);
void *krealloc(void *ptr, size_t size);
void  kfree(void *ptr);

/* Initialize heap using boot memory allocator
 * This is only a temporary initialization and once
 * the page frame allocator is up and runningj,
 * the heap can be reinitialized, this time permanently
 *
 * Return 0 on success
 * Return -EINVAL if the physical->virtual conversion failed
 * Return -ENOMEM if no memory was allocated for heap */
int mmu_heap_preinit(void);

/* Reinitialize the kernel heap using page frame allocator
 *
 * Return 0 on success
 * Return -EINVAL if the physical->virtual conversion failed
 * Return -ENOMEM if no memory was allocated for heap */
int mmu_heap_init(void);

/* TODO: deprecated, remove */
void heap_initialize(uint32_t *heap_start);

#endif /* end of include guard: __KERNEL_HEAP_H__ */

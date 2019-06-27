#ifndef __SLAB_H__
#define __SLAB_H__

#include <mm/types.h>

typedef struct mm_cache mm_cache_t;

/* Initialize the SLAB cache temporarily for booting
 * Allocate 2 pages of memory from the bootmem
 *
 * Return 0 on success */
int mmu_slab_preinit(void);

/* Reinitialize the SLAB cache but this time for good
 * Allocate 5 pages of initial memory
 *
 * Return 0 on success
 * Return -ENOMEM if memory allocation fails */
int mmu_slab_init(void);

/* Create new cache object for objects of size "size"
 *
 * Return pointer to new cache on success
 * Return NULL on error and set errno to:
 *  EINVAL if the "size" is not valid
 *  ENOMEM if kmalloc fails */
mm_cache_t *mmu_cache_create(size_t size, mm_flags_t flags);

/* Destroy the cache pointed to by "cache"
 *
 * Return 0 on success
 * Return -EINVAL if cache is invalid
 * Return -EBUSY if cache has active entries */
int mmu_cache_destroy(mm_cache_t *cache);

/* Alloc entry from the cache
 * The size defined during cache creation will determine the
 * size of the memory block returned to caller
 *
 * Return pointer to memory on success
 * Return NULL on error and set errno to:
 *  EINVAL if one of the parameters is invalid
 *  ENOMEM if no memory for entry was allocated */
void *mmu_cache_alloc_entry(mm_cache_t *c, mm_flags_t flags);

/* Free entry pointed to by pointer "entry"
 * The memory will be placed into a free list
 *
 * Return 0 on success
 * Return -EINVAL if either of the parameters is NULL */
int mmu_cache_free_entry(mm_cache_t *cache, void *entry);

#endif /* __SLAB_H__ */

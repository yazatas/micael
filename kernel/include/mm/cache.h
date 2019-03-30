#ifndef __CACHE_H__
#define __CACHE_H__

#include <stdint.h>
#include <stddef.h>

enum {
    C_NOFLAGS = 0 << 0,

    /* allocate page of memory but don't cache it */
    C_NOCACHE  = 1 << 0,

    /* chunks in the free_chunks may be scattared around memory.
     * If caller wants to force allocations to be spatially close
     * to one another, he should pass this flag to cache_alloc_entry() */
    C_FORCE_SPATIAL = 1 << 1,
} CACHE_CONTROL_FLAGS;

typedef struct cache_fixed cache_t;

/* initializes the cache by allocating CACHE_INITIAL_SIZE
 * cache entries setting up all the necessary variables 
 *
 * return 0 on success and negative errno on error */
int cache_init(void);

/* allocate 4KB (one page) block of memory
 * return pointer to memory block on success and NULL on error
 *
 * If you don't have cache to hold the page (or don't want the page to be cached),
 * pass C_NOCACHE as flag */
void *cache_alloc_page(uint32_t flags);
void  cache_dealloc_page(void *ptr, uint32_t flags);

/* create cache for fixed size elements (used for inodes, dentries, threads etc)
 * cache_create returns cache object on success and NULL on error & and sets errno */
cache_t *cache_create(size_t size, uint32_t flags);

/* destroy cache (release all used memory back to page cache)
 * return 0 on success and -errno on error */
int cache_destroy(cache_t *cache);

/* allocate entry (fixed size) from cache
 *
 * This caching mechanism should be used if the spatial locality of data is important
 * (values are stored very tightly together). This is good if f.ex. caller is allocating
 * space for process' threads. Caller should use C_FORCE_SPATIAL to enforce that
 * memory is close with previous/following allocations. Otherwise the allocator tries to
 * use random memory chunks from free list that may be two pages away from each other */
void *cache_alloc_entry(cache_t *cache, uint32_t flags);

/* deallocate entry (release the memory back to cache) */
void cache_dealloc_entry(cache_t *cache, void *entry, uint32_t flags);

/* print contents of list 
 *
 * type == 0: print free list
 * type == 1: print used list
 * type == 2: print dirty list */
void cache_print_list(int type);

#endif /* __CACHE_H__ */

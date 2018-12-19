#ifndef __CACHE_H__
#define __CACHE_H__

#include <stdint.h>

enum {
    C_NOFLAGS = 0 << 0,

    /* allocate page of memory but don't cache it */
    C_NOCACHE  = 1 << 0,

    /* TODO: ??? */
    C_ENTRYDEL = 1 << 1
} CACHE_CONTROL_FLAGS;


/* initializes the cache by allocating CACHE_INITIAL_SIZE
 * cache entries setting up all the necessary variables 
 *
 * return 0 on success and negative errno on error */
int cache_init(void);

/* allocate 4KB (one page) block of memory
 * return pointer to memory block on success and NULL on error
 *
 * If you don't have cache to hold the page, pass C_NOCACHE */
void *cache_alloc_page(uint32_t flags);
void *cache_alloc_mem(size_t size, uint32_t flags);

void cache_dealloc_page(void *ptr, uint32_t flags);
void cache_dealloc_mem(void *ptr, size_t mem, uint32_t flags);

/* print contents of list 
 *
 * type == 0: print free list
 * type == 1: print used list
 * type == 2: print dirty list */
void cache_print_list(int type);

#endif /* __CACHE_H__ */

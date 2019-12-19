#include <kernel/common.h>
#include <kernel/kprint.h>
#include <lib/list.h>
#include <mm/bootmem.h>
#include <mm/heap.h>
#include <mm/mmu.h>
#include <mm/page.h>
#include <mm/slab.h>
#include <errno.h>

#define C_FORCE_SPATIAL 0

struct cache_free_chunk {
    void *mem;
    list_head_t list;
};

typedef struct cache_fixed_entry {
    /* TODO: what is numfree?? */
    size_t num_free;

    void *next_free; /* pointer to next free slot in mem */
    void *mem;       /* pointer to memory block */

    list_head_t list;
} cfe_t;

struct mm_cache {
    size_t item_size;
    size_t capacity; /* number of total items (# of pages * (PAGE_SIZE / item_size)) */

    /* TODO:  */
    struct cache_fixed_entry *free_list;

    /* list of cache entries, each entry is work of 4KB of memory  */
    struct cache_fixed_entry *used_list;

    /* TODO:  */
    struct cache_free_chunk *free_chunks;
};

/* list of free pages that can be allocated for any cache
 * If this list is empty, alloc_fixed_entry will be used to
 * allocate space for cache.
 *
 * When a cache is destroyed, the pages are place here */
static list_head_t free_list;

static struct cache_fixed_entry *alloc_fixed_entry(size_t capacity)
{
    (void)capacity; // TODO ?

    if (free_list.next != NULL) {
        cfe_t *e = container_of(free_list.next, struct cache_fixed_entry, list);
        list_remove(&e->list);
        return e;
    }

    /* kdebug("allocating more memory!"); */

    unsigned long mem = mmu_bootmem_alloc_page();
    void *mem_v       = mmu_p_to_v(mem);

    cfe_t *e = kzalloc(sizeof(cfe_t));

    e->mem       = mem_v;
    e->next_free = mem_v;
    e->num_free  = PAGE_SIZE;

    return e;
}

int mmu_slab_preinit(void)
{
    list_init_null(&free_list);

    /* alloc two pages for booting */
    for (int i = 0; i < 2; ++i) {
        unsigned long mem = mmu_bootmem_alloc_page();
        void *mem_v       = mmu_p_to_v(mem);

        cfe_t *e = kzalloc(sizeof(cfe_t));

        e->mem       = mem_v;
        e->next_free = mem_v;
        e->num_free  = PAGE_SIZE;

        list_init_null(&e->list);
        list_append(&free_list, &e->list);
    }

    kdebug("SLAB initialized for booting!");

    return 0;
}

int mmu_slab_init(void)
{
    list_init_null(&free_list);

    /* allocate 5 pages of initial memory for SLAB */
    unsigned long mem;
    void *mem_v;
    cfe_t *e;

    for (int i = 0; i < 5; ++i) {
        if ((mem = mmu_page_alloc(MM_ZONE_NORMAL)) == INVALID_ADDRESS) {
            kdebug("Failed to allocate memory");
            return -errno;
        }

        mem_v = mmu_p_to_v(mem);
        e     = kzalloc(sizeof(cfe_t));

        e->mem       = mem_v;
        e->next_free = mem_v;
        e->num_free  = PAGE_SIZE;

        list_init_null(&e->list);
        list_append(&free_list, &e->list);
    }

    return 0;
}

mm_cache_t *mmu_cache_create(size_t size, mm_flags_t flags)
{
    (void)flags;

    mm_cache_t *c = NULL;

    if (size > PAGE_SIZE || size == 0) {
        errno = EINVAL;
        return NULL;
    }

    if ((c = kzalloc(sizeof(mm_cache_t))) == NULL)
        return NULL;

    c->item_size = MULTIPLE_OF_2(size);
    c->capacity  = PAGE_SIZE / c->item_size;

    c->free_list = alloc_fixed_entry(c->capacity);
    c->used_list = NULL;

    /* kdebug("one page holds %u items of size %u (%u)", */
    /*         c->capacity, c->item_size, MULTIPLE_OF_2(c->item_size)); */

    return c;
}

int mmu_cache_destroy(mm_cache_t *cache)
{
    if (!cache)
        return -EINVAL;

    if (cache->used_list != NULL) {
        kdebug("cache still in use, unable to destroy it!");
        return -EBUSY;
    }

    if (cache->free_list != NULL) {
        /* TODO: free pages */

    }

    kfree(cache);
    return 0;
}

void *mmu_cache_alloc_entry(mm_cache_t *c, mm_flags_t flags)
{
    if (!c) {
        errno = EINVAL;
        return NULL;
    }

    /* if there are any free chunks left, try to use them first and return early */
    if (c->free_chunks != NULL && (flags & C_FORCE_SPATIAL) == 0) {
        void *ret = c->free_chunks->mem;

        /* is there only one free chunk? */
        if (c->free_chunks->list.next == NULL) {
            /* kdebug("only one free: 0x%x", ret); */
            c->free_chunks = NULL;
        } else {
            list_head_t *next_chunk = c->free_chunks->list.next;
            list_remove(&c->free_chunks->list);
            c->free_chunks = container_of(next_chunk, struct cache_free_chunk, list);
            /* kdebug("ret 0x%x | next 0x%x", ret, c->free_chunks->mem); */
        }

        return ret;
    }

    void *ret = NULL;

    if (c->free_list == NULL) {
        c->free_list = alloc_fixed_entry(c->capacity);
        c->capacity += PAGE_SIZE / c->item_size;
    }

    ret = c->free_list->next_free;

    /* if last element, set free_list to NULL and append cache_entry to used list
     * else set next_free to point to next free element in c->free_list->mem */
    if (c->free_list->num_free-- == 0) {
        /* kdebug("allocated last element from free list"); */

        if (c->used_list == NULL)
            c->used_list = c->free_list;
        else
            list_append(&c->used_list->list, &c->free_list->list);

        c->free_list = NULL;
    } else {
        /* kdebug("old free 0x%x | new free 0x%x (item size %u)", */
        /*         c->free_list->next_free, */
        /*         (uint8_t *)c->free_list->next_free + c->item_size, */
        /*         c->item_size); */

        c->free_list->next_free = (uint8_t *)c->free_list->next_free + c->item_size;
    }

    return ret;
}

int mmu_cache_free_entry(mm_cache_t *cache, void *entry)
{
    if (cache == NULL || entry == NULL)
        return -EINVAL;

    struct cache_free_chunk *cfc = kmalloc(sizeof(struct cache_free_chunk));

    list_init_null(&cfc->list);
    cfc->mem = entry;

    /* kdebug("freeing memory at address 0x%x", cfc->mem); */

    if (cache->free_chunks == NULL)
        cache->free_chunks = cfc;
    else
        list_append(&cache->free_chunks->list, &cfc->list);

    return 0;
}

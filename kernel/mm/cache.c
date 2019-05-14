#include <kernel/common.h>
#include <kernel/kprint.h>
#include <lib/list.h>
#include <mm/cache.h>
#include <mm/heap.h>
#include <mm/mmu.h>

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#define CACHE_INITIAL_SIZE 48
#define CACHE_MAX_ENTRIES  512

enum {
    CE_DIRTY = 1 << 0,
} CACHE_ENTRY_FLAGS;

struct cache_free_chunk {
    void *mem;
    list_head_t list;
};

struct cache_fixed_entry {
    size_t num_free;

    void *next_free; /* pointer to next free slot in mem */
    void *mem;       /* pointer to memory block */

    list_head_t list;
};

struct cache_fixed {
    size_t item_size;
    size_t capacity; /* number of total items (# of pages * (PAGE_SIZE / item_size)) */

    /* TODO:  */
    struct cache_fixed_entry *free_list;

    /* list of cache entries, each entry is work of 4KB of memory  */
    struct cache_fixed_entry *used_list;

    /* TODO:  */
    struct cache_free_chunk  *free_chunks;
};

/* page cache stuff */
struct cache_entry {
    void *mem;
    page_t phys_addr;

    uint32_t flags;
    uint32_t uniq;
    size_t ref_count;

    list_head_t list;
};

static struct page_cache {
    size_t len;

    struct cache_entry *free_list;  /* list of free pages ripe for allocation */
    size_t len_free_list;

    struct cache_entry *used_list;  /* list of pages currently in use */
    size_t len_used_list;

    struct cache_entry *dirty_list; /* list of dirty pages (flush to disk needed) */
    size_t len_dirty_list;
} cache = {
    .len = CACHE_INITIAL_SIZE,
    
    .free_list     = NULL,
    .len_free_list = 0,

    .used_list     = NULL,
    .len_used_list = 0,

    .dirty_list     = NULL,
    .len_dirty_list = 0
};

static void cache_list_remove(struct cache_entry **list,
                              struct cache_entry *entry)
{
    if (*list == entry) {
        list_head_t *tmp_list = entry->list.next;

        if (tmp_list == NULL)
            *list = NULL;
        else
            *list = container_of(tmp_list, struct cache_entry, list);
    }

    list_remove(&entry->list);
}

static void cache_list_insert(struct cache_entry **head,
                              struct cache_entry *entry)
{
    if (!(*head)) {
        *head = entry;
        list_init_null(&entry->list);
    } else {
        list_append(&(*head)->list, &entry->list);
    }
}

static struct cache_fixed_entry *alloc_fixed_entry(size_t capacity)
{
    struct cache_fixed_entry *cfe = NULL;

    if ((cfe = kmalloc(sizeof(struct cache_fixed_entry))) == NULL)
        return NULL;

    cfe->mem       = cache_alloc_page(C_NOFLAGS);
    cfe->next_free = cfe->mem;
    cfe->num_free  = capacity;

    list_init_null(&cfe->list);

    return cfe;
}

static struct cache_entry *alloc_entry(void)
{
    struct cache_entry *ce = kmalloc(sizeof(struct cache_entry));

    if (!ce) {
        errno = ENOMEM;
        return NULL;
    }

    static int i = 1337;

    ce->ref_count = 0;
    ce->phys_addr = mmu_alloc_page();
    ce->mem       = mmu_alloc_addr(1);
    ce->uniq      = i++;

    cache.len++;

    if (ce->mem == NULL) {
        kdebug("failed to allocate virtual address for page!");
        errno = ENOMEM;
        return NULL;
    }

    mmu_map_page((void *)ce->phys_addr, ce->mem, MM_PRESENT | MM_READWRITE);
    list_init_null(&ce->list);

    return ce;
}

/* first remove this entry from any list head and
 * the deallocate all its memory */
static void dealloc_entry(struct cache_entry *entry)
{
    if (!entry)
        return;

    struct cache_entry *list_head  = NULL;
    list_head_t *next_entry        = NULL;

    if (cache.free_list == entry) {
        list_head = cache.free_list;
        next_entry = &cache.free_list->list;
    } else if (cache.used_list == entry) {
        list_head = cache.used_list;
        next_entry = &cache.used_list->list;
    } else if (cache.dirty_list == entry) {
        list_head = cache.dirty_list;
        next_entry = &cache.dirty_list->list;
        /* TODO: flush dirty block to disk (if necessary) */
    }

    if (list_head != NULL && next_entry != NULL) {
        kdebug("what is supposed to happen here?");
        /* TODO: ??? */
    }

    list_remove(&entry->list);
    mmu_free_addr(entry->mem, 1);
    mmu_free_page(entry->phys_addr);
    kfree(entry);
}

int cache_init(void)
{
    struct cache_entry *ce;

    if ((ce = alloc_entry()) == NULL)
        return -errno;

    cache.free_list     = ce;
    cache.len_free_list = 1;

    for (size_t i = 1; i < CACHE_INITIAL_SIZE; ++i) {
        if ((ce = alloc_entry()) == NULL)
            return -errno;

        list_append(&cache.free_list->list, &ce->list);
        cache.len_free_list++;
    }

    return 0;
}

void *cache_alloc_page(uint32_t flags)
{
    (void)flags;

    list_head_t *next_ce    = NULL;
    struct cache_entry *ce  = NULL,
                       *tmp = NULL;

    if (cache.free_list != NULL) {
        ce      = cache.free_list;
        next_ce = cache.free_list->list.next;

        if (next_ce != NULL)
            tmp = container_of(next_ce, struct cache_entry, list);

        /* kdebug("returning page with uniq %u", ce->uniq); */

        list_remove(&ce->list);
        cache.free_list = tmp;
        cache.len_free_list--;
    }

    if (ce == NULL && cache.len < CACHE_MAX_ENTRIES) {
        ce = alloc_entry();
        /* kdebug("ALLOCATIN NEW BLOCK %u", ce->uniq); */
    }

    if (ce) {
        /* kdebug("returning 0x%x 0x%x", ce->phys_addr, ce->mem); */
        cache_list_insert(&cache.used_list, ce);

        ce->ref_count++;
        cache.len_used_list++;

        return ce->mem;
    }

    return NULL;
}

/* FIXME: how to obtain cache_entry in constant time? */
void cache_dealloc_page(void *ptr, uint32_t flags)
{
    (void)flags;

    if (!ptr)
        return;

    bool found = false;
    struct cache_entry *ce  = cache.used_list;
    list_head_t *next_entry = &cache.used_list->list;

    do {
        if (ce->mem == ptr) {
            found = true;
            break;
        }

        next_entry = next_entry->next;
        ce = container_of(next_entry, struct cache_entry, list);
    } while (next_entry != NULL);

    if (!found) {
        kdebug("failed to find corresponding cache entry for 0x%x", ptr);
        return;
    }

    kprint("uniq %u ref_count %u phys_addr 0x%x mem 0x%x\n",
            ce->uniq, ce->ref_count, ce->phys_addr, ce->mem);

    ce->ref_count--;
    cache.len_used_list--;
    cache.len_free_list++;

    cache_list_remove(&cache.used_list, ce);

    if (ce->flags & CE_DIRTY)
        cache_list_insert(&cache.dirty_list, ce);
    else
        cache_list_insert(&cache.free_list, ce);
}

void cache_print_list(int type)
{
    struct cache_entry *ce  = NULL;
    list_head_t *next_entry = NULL;
    const char *list_name   = NULL;
    size_t len;

    switch (type) {
        case 0:
            ce = cache.free_list;
            next_entry = &cache.free_list->list;
            list_name = "free";
            len = cache.len_free_list;
            break;
        case 1:
            ce = cache.used_list;
            next_entry = &cache.used_list->list;
            list_name = "used";
            len = cache.len_used_list;
            break;
        case 2:
            ce = cache.dirty_list;
            next_entry = &cache.dirty_list->list;
            list_name = "dirty";
            len = cache.len_dirty_list;
            break;
        default:
            return;
    }

    kprint("%s list's entries:\n", list_name);

    for (size_t i = 0; i < len; ++i) {
        kprint("\tuniq %u ref_count %u phys_addr 0x%x mem 0x%x\n",
                ce->uniq, ce->ref_count, ce->phys_addr, ce->mem);

        next_entry = next_entry->next;
        ce = container_of(next_entry, struct cache_entry, list);
    }
}

cache_t *cache_create(size_t size, uint32_t flags)
{
    (void)flags;

    cache_t *c = NULL;

    if (size > PAGE_SIZE || size == 0) {
        errno = EINVAL;
        return NULL;
    }

    if ((c = kcalloc(1, sizeof(cache_t))) == NULL)
        return NULL;

    c->item_size = MULTIPLE_OF_2(size);
    c->capacity  = PAGE_SIZE / c->item_size;

    c->free_list = alloc_fixed_entry(c->capacity);
    c->used_list = NULL;

    /* kdebug("one page holds %u items of size %u (%u)", */
    /*         c->capacity, c->item_size, MULTIPLE_OF_2(c->item_size)); */

    return c;
}

int cache_destroy(cache_t *c)
{
    if (!c)
        return -EINVAL;

    if (c->used_list != NULL) {
        kdebug("cache still in use, unable to destroy it!");
        return -EEXIST;
    }

    if (c->free_list == NULL) {
        kdebug("cache doesn't hold any memory");
        return -EINVAL;
    }

    /* list_head_t *next_entry; */
    /* struct cache_entry *ce = c->free_list; */
    /* do { */
    /*     next_entry = ce->list.next; */
    /*     dealloc_entry(ce); */
    /*     ce = container_of(next_entry, struct cache_entry, list); */
    /* } while (ce->list.next); */

    return 0;
}

void *cache_alloc_entry(cache_t *c, uint32_t flags)
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
        kdebug("allocated last element from free list");

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

void cache_dealloc_entry(cache_t *c, void *entry, uint32_t flags)
{
    (void)flags;

    struct cache_free_chunk *cfc = kmalloc(sizeof(struct cache_free_chunk));

    list_init_null(&cfc->list);
    cfc->mem = entry;

    /* kdebug("freeing memory at address 0x%x", cfc->mem); */

    if (c->free_chunks == NULL)
        c->free_chunks = cfc;
    else
        list_append(&c->free_chunks->list, &cfc->list);
}

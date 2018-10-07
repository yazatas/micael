#include <lib/hashmap.h>
#include <lib/list.h>
#include <mm/cache.h>
#include <mm/kheap.h>
#include <mm/vmm.h>

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#define CACHE_INITIAL_SIZE 3
#define CACHE_MAX_ENTRIES  512

enum {
    CE_DIRTY = 1 << 0,
} CACHE_ENTRY_FLAGS;

struct cache_entry {
    void *mem;
    page_t phys_addr;

    uint32_t flags;
    uint32_t uniq;
    size_t ref_count;

    /* TODO: who owns this cache entry?? */

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

static struct cache_entry *alloc_entry(void)
{
    struct cache_entry *ce = kmalloc(sizeof(struct cache_entry));

    if (!ce) {
        errno = ENOMEM;
        return NULL;
    }

    static int i = 1337;

    ce->ref_count = 0;
    ce->phys_addr = vmm_alloc_page();
    ce->mem       = vmm_alloc_addr(1);
    ce->uniq      = i++;

    cache.len++;

    if (ce->mem == NULL) {
        kdebug("failed to allocate virtual address for page!");
        errno = ENOMEM;
        return NULL;
    }

    vmm_map_page((void *)ce->phys_addr, ce->mem, P_PRESENT | P_READWRITE);
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
        /* TODO: ??? */
    }

    list_remove(&entry->list);
    vmm_free_addr(entry->mem, 1);
    vmm_free_page(entry->phys_addr);
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

void *cache_alloc(uint32_t flags)
{
    list_head_t *next_ce    = NULL;
    struct cache_entry *ce  = NULL,
                       *tmp = NULL;

    if (cache.free_list != NULL) {
        ce      = cache.free_list;
        next_ce = cache.free_list->list.next;

        if (next_ce != NULL)
            tmp = container_of(next_ce, struct cache_entry, list);

        kdebug("returning block with block %u", ce->uniq);

        list_remove(&ce->list);
        cache.free_list = tmp;
        cache.len_free_list--;
    }

    if (ce == NULL && cache.len < CACHE_MAX_ENTRIES) {
        ce = alloc_entry();
        kdebug("ALLOCATIN NEW BLOCK %u", ce->uniq);
    }

    if (ce) {
        kdebug("returning 0x%x 0x%x", ce->phys_addr, ce->mem);
        cache_list_insert(&cache.used_list, ce);

        ce->ref_count++;
        cache.len_used_list++;

        return ce->mem;
    }

    return NULL;
}

/* FIXME: how to obtain cache_entry in constant time? */
void cache_dealloc(void *ptr, uint32_t flags)
{
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

    /* if (ce->flags & CE_DIRTY) */
    /*     cache_list_insert(&cache.dirty_list, ce); */
    /* else */
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

#include <fs/dcache.h>
#include <lib/hashmap.h>
#include <lib/list.h>
#include <mm/cache.h>
#include <kernel/util.h>

#define DCACHE_MAX_SIZE   512
#define DENTRIES_PER_PAGE  22
#define CACHE_DENTRY_SIZE  (sizeof(dentry_t) + sizeof(struct hm_item))

/* this is the item that's saved in the hashmap. Storing only 
 * the dentry is not ideal because that makes the deallocation
 * quite hard and inefficient (we'd have to loop through all pages
 * to find the corresponding dentry) 
 *
 * de points to the beginning of page (to the "header") */
struct hm_item {
    struct dcache_entry *de;
    dentry_t dntr;
};

/* dcache_entry consumes exactly one page of memory (4KB)
 * it stores DENTRIES_PER_PAGE dentries 
 *
 * you can think of dcache_entry as a header (at the beginning of page)
 * followed by DENTRIES_PER_PAGE hm_items 
 *
 * next_free should points to next free hm_item to speed up the allocation 
 * Because we keep only one pointer to next_free it's possible that it'll
 * have the value NULL even if there are free hm_item in the page 
 * (user freed dentry somewhere in the middle of the page -> next_free was
 * set to point to this freed hm_item and then user allocated another entry
 * -> consuming this free entry. If the page had more free items at the end
 *  the pointer was lost. Thus the allocation code, if it encouters a situation
 *  where next_free == NULL && num_free != 0, must loop through the page and 
 *  look for hm_item of which de is NULL */
struct dcache_entry {
    size_t num_free;
    list_head_t list;
    struct hm_item *next_free;
};

static struct dcache {
    hashmap_t *dentries;
    size_t capacity;
    size_t len;

    /* contains a list of cache entries which have
     * room left for more dentries 
     *
     * Ideally this should hold only one page at a time but
     * sometimes the user might free one dentry from used page.
     * This makes the page "free" again and thus it must be placed here
     * even though it only has room for one dentry */
    struct dcache_entry *free_list;
    
    /* list of cache entries full of dentries (no room left) */
    struct dcache_entry *used_list;

} dcache;

static struct dcache_entry *dcache_alloc_entry(void)
{
    struct dcache_entry *de = cache_alloc_page(C_NOFLAGS);

    memset(de, 0, sizeof(struct dcache_entry));

    dcache.capacity += DENTRIES_PER_PAGE;
    de->num_free     = DENTRIES_PER_PAGE;
    de->next_free    = (struct hm_item *)((uint8_t *)de + sizeof(struct dcache_entry));

    list_init_null(&de->list);

    return de;
}

/* hashmap has to be allocated right away because (right now at least)
 * it doesn't support dynamic sizing */
int dcache_init(void)
{
    dcache.dentries = hm_alloc_hashmap(DCACHE_MAX_SIZE, HM_KEY_TYPE_STR);
    dcache.capacity = DCACHE_MAX_SIZE;
    dcache.len      = 0;

    dcache.free_list = NULL;
    dcache.used_list = NULL;

    return 0;
}

/* TODO: rewrite to use the allocated page directly 
 * TODO: handle case where allocation happens at the middle
 *       of page and there is more room at the end of page */
dentry_t *dentry_alloc(const char *name)
{
    if (!dcache.free_list) {
        kdebug("allocating new dcache_entry!");
        dcache.free_list = dcache_alloc_entry();
    }

    struct hm_item hi, *tmp = dcache.free_list->next_free;
    memset(&hi, 0, sizeof(struct hm_item));

    hi.de = dcache.free_list;
    strncpy(hi.dntr.d_name, name, VFS_NAME_MAX_LEN);

    memcpy(dcache.free_list->next_free, &hi, sizeof(struct hm_item));
    hm_insert(dcache.dentries, (void *)name, dcache.free_list->next_free);

    dcache.len++;
    dcache.free_list->num_free--;

    if (dcache.free_list->num_free == 0) {
        if (dcache.used_list == NULL)
            dcache.used_list = dcache.free_list;
        else
            list_append(&dcache.used_list->list, &dcache.free_list->list);
        dcache.free_list = NULL;
    } else {
        dcache.free_list->next_free++;
    }

    /* allocate hashmap for dentry's children */
    tmp->dntr.children = hm_alloc_hashmap(DENTRY_HM_LEN, HM_KEY_TYPE_STR);

    return &tmp->dntr;
}

/* this actually doesn't free the memory
 * it just marks the dentry as free so it can be overwritten 
 *
 * TODO: check if it's ok to delete entry from cache 
 *       (inode should hold that info, right?) */
void dentry_dealloc(dentry_t *dntr)
{
    struct hm_item *hi = NULL;

    if (!dntr)
        return;

    if ((hi = hm_get(dcache.dentries, dntr->d_name)) == NULL) {
        kdebug("dentry '%s' does not exist in the hashmap", dntr->d_name);
        return;
    }

    if (hm_remove(dcache.dentries, dntr->d_name) < 0) {
        kdebug("failed to remove dentry from hashmap!");
        return;
    }

    dcache.len--;
    hi->de->num_free++;
    hi->de->next_free = hi;

    /* if this page was in the used list, move it back to free list */
    if (hi->de->num_free == 1) {
        if (hi->de->list.next == NULL && hi->de->list.prev == NULL)
            dcache.used_list = NULL;

        list_remove(&hi->de->list);

        if (dcache.free_list == NULL)
            dcache.free_list = hi->de;
        else
            list_append(&dcache.free_list->list, &hi->de->list);
    }
}

dentry_t *dentry_lookup(const char *dname)
{
    struct hm_item *hi = hm_get(dcache.dentries, (void *)dname);

    if (!hi)
        return NULL;

    return &hi->dntr;
}

#include <kernel/util.h>
#include <errno.h>
#include <stdbool.h>

#include <lib/hashmap.h>
#include <mm/heap.h>
#include <sync/spinlock.h>

#define BUCKET_MAX_LEN 12
#define KEY_SIZE       12

/* TODO fix possible null pointer dereferences!!! */

typedef struct hm_item {
    void *data;
    uint32_t key;
    bool occupied;
} hm_item_t;

struct hashmap {
    size_t len;
    size_t cap;
    spinlock_t lock;

    hm_item_t **elem;
    uint32_t (*hm_hash)(void *);
};

/*  https://stackoverflow.com/questions/664014/what-integer-
 *  hash-function-are-good-that-accepts-an-integer-hash-key 
 *
 *  "double hash" the key to prevent collicions as much as
 *  possible. Key gotten from block cache should be pretty unique 
 *  as there won't be that many devices running concurrently */
static uint32_t hm_hash_num(void *key)
{
    if (key == NULL)
        return UINT32_MAX;

    uint32_t tmp = *(uint32_t *)key;

    tmp = ((tmp >> 16) ^ tmp) * 0x45d9f3b;
    tmp = ((tmp >> 16) ^ tmp) * 0x45d9f3b;
    tmp = ((tmp >> 16) ^ tmp);

    return tmp;
}

/*  http://www.cse.yorku.ca/~oz/hash.html */
static uint32_t hm_hash_str(void *key)
{
    if (key == NULL)
        return UINT32_MAX;

    uint32_t hash = 5381;
    char *str     = key;
    int c;

    while ((c = *str++) != '\0') {
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}

/* "size" should be large enough so that rehashing
 * needn't to be performed as it's very expensive */
hashmap_t *hm_alloc_hashmap(size_t size, hm_key_type_t type)
{
    hashmap_t *hm;

    if ((hm = kmalloc(sizeof(hashmap_t), 0)) == NULL)
        return NULL;

    if ((hm->elem = kcalloc(size, sizeof(hm_item_t *))) == NULL)
        return NULL;

    hm->len  = 0;
    hm->lock = 0;
    hm->cap  = size;

    if (type == HM_KEY_TYPE_NUM) {
        hm->hm_hash = hm_hash_num;
    } else if (type == HM_KEY_TYPE_STR) {
        hm->hm_hash = hm_hash_str;
    } else {
        errno = EINVAL;
        return NULL;
    }

    return hm;
}

void hm_dealloc_hashmap(hashmap_t *hm)
{
    if (!hm)
        return;

    if (!hm->elem)
        goto free_hm;

    for (size_t i = 0; i < hm->len; ++i) {
        kfree(hm->elem[i]);
    }

    kfree(hm->elem);

free_hm:
    kfree(hm);
}

static int hm_find_free_bucket(hashmap_t *hm, uint32_t key)
{
    int index = key % hm->cap;

    for (size_t i = 0; i < BUCKET_MAX_LEN; ++i) {
        if (hm->elem[index] == NULL) {
            hm->elem[index] = kmalloc(sizeof(hm_item_t), 0);
            return index;
        }

        if (hm->elem[index]->occupied == false)
            return index;

        index = (index + 1) % hm->cap;
    }

    return -ENOSPC;
}

int hm_insert(hashmap_t *hm, void *ukey, void *elem)
{
    if (!hm || !elem)
        return -EINVAL;

    if (hm->len == hm->cap)
        return -ENOSPC;

    uint32_t key;
    int index;

    if ((key = hm->hm_hash(ukey)) == UINT32_MAX)
        return -EINVAL;

    spin_acquire(&hm->lock);

    index = hm_find_free_bucket(hm, key);

    if (index < 0) {
        spin_release(&hm->lock);
        return -ENOSPC;
    }

    hm->elem[index]->data     = elem;
    hm->elem[index]->key      = key;
    hm->elem[index]->occupied = true;
    hm->len++;

    spin_release(&hm->lock);
    return 0;
}

int hm_remove(hashmap_t *hm, void *ukey)
{
    if (!hm)
        return -EINVAL;

    uint32_t key;
    int index;

    if ((key = hm->hm_hash(ukey)) == UINT32_MAX)
        return -EINVAL;

    spin_acquire(&hm->lock);

    index = key % hm->cap;

    for (size_t i = 0; i < BUCKET_MAX_LEN; ++i) {
        if (hm->elem[index]->key == key) {
            hm->elem[index]->occupied = false;
            spin_release(&hm->lock);
            return 0;
        }

        index = (index + 1) % hm->cap;
    }

    spin_release(&hm->lock);
    return -ENOENT;
}

void *hm_get(hashmap_t *hm, void *ukey)
{
    if (!hm || !hm->elem)
        return NULL;

    uint32_t key;
    int index;

    if ((key = hm->hm_hash(ukey)) == UINT32_MAX)
        return NULL;

    index = key % hm->cap;

    for (size_t i = 0; i < BUCKET_MAX_LEN; ++i) {
        if (hm->elem[index] == NULL)
            return NULL;
        if (hm->elem[index]->occupied == false)
            return NULL;
        if (hm->elem[index]->key == key)
            return hm->elem[index]->data;

        index = (index + 1) % hm->cap;
    }

    return NULL;
}

size_t hm_get_size(hashmap_t *hm)
{
    return hm ? hm->len : 0;
}

size_t hm_get_capacity(hashmap_t *hm)
{
    return hm ? hm->cap : 0;
}

void hm_add_hash_func(hashmap_t *hm, uint32_t (*hm_hash_func)(void *))
{
    if (!hm || !hm_hash_func) {
        errno = EINVAL;
        return;
    }

    hm->hm_hash = hm_hash_func;
}

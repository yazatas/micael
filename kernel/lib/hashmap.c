#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <lib/hashmap.h>
#include <mm/kheap.h>

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

    hm_item_t **elem;
};

/*  https://stackoverflow.com/questions/664014/what-integer-
 *  hash-function-are-good-that-accepts-an-integer-hash-key 
 *
 *  "double hash" the key to prevent collicions as much as
 *  possible. Key gotten from block cache should be pretty unique 
 *  as there won't be that many devices running concurrently */
static void hm_rehash(uint32_t *key)
{
    if (key == NULL)
        return;

    *key = ((*key >> 16) ^ *key) * 0x45d9f3b;
    *key = ((*key >> 16) ^ *key) * 0x45d9f3b;
    *key = (*key  >> 16) ^ *key;
}

/* "size" should be large enough so that rehashing
 * needn't to be performed as it's very expensive */
hashmap_t *hm_alloc_hashmap(size_t size)
{
    hashmap_t *hm;

    if ((hm = kmalloc(sizeof(hashmap_t))) == NULL)
        return NULL;

    if ((hm->elem = kmalloc(size * sizeof(hm_item_t *))) == NULL)
        return NULL;

    hm->len = 0;
    hm->cap = size;

    return hm;
}

void hm_dealloc_hashmap(hashmap_t *hm)
{
    if (!hm)
        return;

    if (!hm->elem)
        goto free_hm;

    for (size_t i = 0; i < hm->len; ++i)
        kfree(hm->elem[i]);

    kfree(hm->elem);

free_hm:
    kfree(hm);
}

static int hm_find_free_bucket(hashmap_t *hm, uint32_t key)
{
    int index = key % hm->cap;

    for (size_t i = 0; i < BUCKET_MAX_LEN; ++i) {
        if (hm->elem[index] == NULL) {
            hm->elem[index] = kmalloc(sizeof(hm_item_t));
            return index;
        }

        if (hm->elem[index]->occupied == false)
            return index;

        index = (index + 1) % hm->cap;
    }

    return -ENOSPC;
}

int hm_insert(hashmap_t *hm, uint32_t key, void *elem)
{
    if (!hm || !elem)
        return -EINVAL;

    if (hm->len == hm->cap)
        return -ENOSPC;

    hm_rehash(&key);
    int index = hm_find_free_bucket(hm, key);

    if (index < 0)
        return -ENOSPC;

    hm->elem[index]->data     = elem;
    hm->elem[index]->key      = key;
    hm->elem[index]->occupied = true;
    hm->len++;

    return 0;
}

int hm_remove(hashmap_t *hm, uint32_t key)
{
    if (!hm)
        return -EINVAL;

    hm_rehash(&key);
    int index = key % hm->cap;

    for (size_t i = 0; i < BUCKET_MAX_LEN; ++i) {
        if (hm->elem[index]->key == key) {
            hm->elem[index]->occupied = false;
            return 0;
        }

        index = (index + 1) % hm->cap;
    }

    return -ENOENT;
}

void *hm_get(hashmap_t *hm, uint32_t key)
{
    if (!hm || !hm->elem)
        return NULL;

    hm_rehash(&key);
    int index = key % hm->cap;

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

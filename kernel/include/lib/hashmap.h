#ifndef __HASHMAP_H__
#define __HASHMAP_H__

#include <stddef.h>
#include <stdint.h>

typedef struct hashmap hashmap_t;

hashmap_t *hm_alloc_hashmap(size_t size, uint32_t (*hm_hash_func)(void *));
void       hm_dealloc_hashmap(hashmap_t *hm);

int hm_insert(hashmap_t *hm, void *ukey, void *elem);
int hm_remove(hashmap_t *hm, void *ukey);

void *hm_get(hashmap_t *hm, void *ukey);
size_t hm_get_size(hashmap_t *hm);
size_t hm_get_capacity(hashmap_t *hm);

#endif /* __HASHMAP_H__ */

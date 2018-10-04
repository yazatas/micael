#ifndef __HASHMAP_H__
#define __HASHMAP_H__

#include <stddef.h>
#include <stdint.h>

typedef struct hashmap hashmap_t;

hashmap_t *hm_alloc_hashmap(size_t size);
void       hm_dealloc_hashmap(hashmap_t *hm);

int hm_insert(hashmap_t *hm, uint32_t key, void *elem);
int hm_remove(hashmap_t *hm, uint32_t key);

void *hm_get(hashmap_t *hm, uint32_t key);

#endif /* __HASHMAP_H__ */

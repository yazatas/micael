#ifndef __B_HEAP_H__
#define __B_HEAP_H__

#include <sys/types.h>
#include <stdbool.h>

typedef struct bheap bheap_t;

/* Initialize new binary heap structure with "items" of initial elements
 * If "items" is 0, one page of memory will be allocated (4 KB)
 *
 * Return heap object on success
 * Return NULL on error */
bheap_t *bh_init(size_t items);

/* Insert new element to heap with priority "key"
 *
 * Return 0 on success
 * Return -EINVAL if "heap" is NULL
 * Return -EINVAL if "payload" is NULL */
int bh_insert(bheap_t *heap, int key, void *payload);

/* Remove elements from the heap that have a priority "key"
 *
 * Return the payload on success
 * Return NULL if heap is empty
 * Return NULL on error and set errno to:
 *   EINVAL if "heap" is NULL
 *   ENOENT if "key" does not exist in the heap */
void *bh_remove(bheap_t *heap, int key);

/* Remove element(s) from the heap by comparing against the payload
 * This is less brute force way of removing an element from the queue
 *
 * Arguments are pointer to heap object, the payload object we're looking for
 * and cc is a function pointer to custom comparator which should return true
 * if "arg" and payload in the heap match and false if they don't
 *
 * Return the payload on success
 * Remove NULL if heap is empty
 * Return NULL on error and set errno to:
 *   EINVAL if one of the parameters is NULL
 *   ENOENT if the payload was not found from the heap */
void *bh_remove_pld(bheap_t *heap, void *arg, bool (cc)(void *, void *));

/* Remove element from heap that has the highest priority
 *
 * Return the payload on success
 * Return NULL if heap is empty
 * Return NULL on error and set errno to:
 *   EINVAL if "heap" is NULL */
void *bh_remove_max(bheap_t *heap);

/* Return the key value of max element
 *
 * Return -EINVAL if "heap" is NULL
 * Return INT_MIN if heap is empty */
int bh_peek_max(bheap_t *heap);

#endif /* __B_HEAP_H__ */

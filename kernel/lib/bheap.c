#include <kernel/kprint.h>
#include <lib/bheap.h>
#include <mm/heap.h>
#include <mm/mmu.h>
#include <mm/page.h>
#include <errno.h>
#include <limits.h>

#define PARENT(i)   ((i - 1) / 2)
#define RIGHT(i)    ((2 * i) + 2)
#define LEFT(i)     ((2 * i) + 1)

struct elem {
    int key;
    void *payload;
} __attribute__((aligned(16)));

struct bheap {
    size_t size;        /* total number of elements */
    size_t capacity;    /* how much space left there is */
    struct elem *elems; /* actual elements of the heap */
};

#define EPP (4096 / sizeof(struct elem))

static inline void __swap(bheap_t *h, int i, int p)
{
    struct elem tmp = h->elems[p];

    h->elems[p].key     = h->elems[i].key;
    h->elems[p].payload = h->elems[i].payload;

    h->elems[i].key     = tmp.key;
    h->elems[i].payload = tmp.payload;
}

static inline void __max_heapify(bheap_t *h, int i)
{
    int l = LEFT(i);
    int r = RIGHT(i);
    int b = i;

    if (l < (int)h->size && h->elems[l].key > h->elems[i].key)
        b = l;

    if (r < (int)h->size && h->elems[r].key > h->elems[b].key)
        b = r;

    if (b != i) {
        __swap(h, i, b);
        __max_heapify(h, b);
    }
}

static inline void *__remove_elem(bheap_t *h, int i)
{
    h->elems[i].key = INT_MAX;

    while (i != 0 && h->elems[i].key > h->elems[PARENT(i)].key) {
        __swap(h, i, PARENT(i));
        i = PARENT(i);
    }

    return bh_remove_max(h);
}

bheap_t *bh_init(size_t items)
{
    bheap_t *heap = kmalloc(sizeof(bheap_t));

    if (!heap) {
        errno = ENOMEM;
        return NULL;
    }

    size_t npages  = (items / EPP + !!(items % EPP));

    heap->size     = 0;
    heap->capacity = npages * EPP;
    heap->elems    = mmu_p_to_v(mmu_block_alloc(MM_ZONE_NORMAL, npages, 0));

    return heap;
}

int bh_insert(bheap_t *heap, int key, void *payload)
{
    if (!heap || !payload)
        return -EINVAL;

    if (heap->size == heap->capacity)
        return -ENOSPC;

    heap->elems[heap->size].key     = key;
    heap->elems[heap->size].payload = payload;
    heap->size++;

    int i = heap->size - 1;

    while (i != 0 && heap->elems[i].key > heap->elems[PARENT(i)].key) {
        __swap(heap, i, PARENT(i));
        i = PARENT(i);
    }
}

/* I don't know if this function makes any sense but
 * because the inner working of the heap are not exposed to user 
 * and because a need to delete an item with specific key might arise,
 * I guess it's better to provide support for this kind of operation too */
void *bh_remove(bheap_t *heap, int key)
{
    if (!heap) {
        errno = EINVAL;
        return NULL;
    }

    if (heap->size == 0) {
        errno = ENOENT;
        return NULL;
    }

    for (size_t i = 0; i < heap->size; ++i) {
        if (heap->elems[i].key == key)
            return __remove_elem(heap, i);
    }

    errno = ENOENT;
    return NULL;
}

void *bh_remove_pld(bheap_t *heap, void *arg, bool (cc)(void *, void *))
{
    if (!heap || !arg || !cc) {
        errno = EINVAL;
        return NULL;
    }

    for (size_t i = 0; i < heap->size; ++i) {
        if (cc(arg, heap->elems[i].payload))
            return __remove_elem(heap, i);
    }

    errno = ENOENT;
    return NULL;
}

void *bh_remove_max(bheap_t *heap)
{
    if (!heap) {
        errno = EINVAL;
        return NULL;
    }

    if (heap->size == 0) {
        errno = ENOENT;
        return NULL;
    }

    if (heap->size == 1) {
        heap->size = 0;
        return heap->elems[0].payload;
    }

    void *payload = heap->elems[0].payload;

    heap->elems[0] = heap->elems[heap->size - 1];
    heap->size--;

    __max_heapify(heap, 0);

    return payload;
}

int bh_peek_max(bheap_t *heap)
{
    if (!heap)
        return -EINVAL;

    if (heap->size == 0)
        return INT_MIN;

    return heap->elems[0].key;
}

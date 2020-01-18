#include <kernel/kprint.h>
#include <kernel/kpanic.h>
#include <kernel/util.h>
#include <mm/bootmem.h>
#include <mm/heap.h>
#include <mm/mmu.h>
#include <mm/page.h>
#include <sync/spinlock.h>
#include <stdbool.h>
#include <sys/types.h>
#include <errno.h>

typedef struct meta {
    size_t size;
    uint8_t flags;
    struct meta *next;
    struct meta *prev;
} __attribute__((packed)) meta_t;

#define META_SIZE      sizeof(meta_t)
#define IS_FREE(x)     (x->flags & 0x1)
#define MARK_FREE(x)   (x->flags |= 0x1)
#define UNMARK_FREE(x) (x->flags &= ~0x1)
#define GET_BASE(x)    ((meta_t*)x - 1)

static meta_t        *kernel_base;
static unsigned long *HEAP_START;
static unsigned long *HEAP_BREAK;
static spinlock_t     lock = 0;

/* TODO: this needs a rewrite!!! 
 *
 * kernel page directory structures and the whole mmu 
 * has changes a lot since this was written so update
 * it to make compatible with today's mmu */

/* 4MB of initial memory is allocated on system startup, if that memory has run out
 * we must allocate new page table. I think this kind of abstraction should be mmu's
 * job to provide so don't write PT allocation here. Instead call mmu_alloc_pt */
static meta_t *morecore(size_t size)
{
    kpanic("morecore not implemented");

    size_t pgcount = size / 0x1000 + 1;
    meta_t *tmp = (meta_t*)HEAP_BREAK;

    for (size_t i = 0; i < pgcount; ++i) {
        /* mmu_map_page((void *)mmu_alloc_page(), HEAP_BREAK, MM_PRESENT | MM_READWRITE); */
        HEAP_BREAK = (unsigned long *)((uint8_t *)HEAP_BREAK + 0x1000);
    }
    /* mmu_flush_TLB(); */

    /* place the new block at the beginning of block list */
    tmp->size = pgcount * 0x1000 - META_SIZE;
    kernel_base->prev = tmp;
    tmp->next = kernel_base;
    tmp->prev = NULL;
    kernel_base = tmp;
    MARK_FREE(tmp);

    return tmp;
}

/* append b2 to b1 (b2 becomes invalid) */
static inline void merge_blocks(meta_t *b1, meta_t *b2)
{
    b1->size += b2->size + META_SIZE;
    b1->next = b2->next;
    if (b2->next)
        b2->next->prev = b1;
}

static void split_block(meta_t *b, size_t split)
{
    if (b->size <= split + META_SIZE)
        return;

    meta_t *tmp = (meta_t*)((uint8_t*)b + split + META_SIZE);
    tmp->size = b->size - split - META_SIZE;
    b->size = split;
    MARK_FREE(tmp);

    tmp->next = b->next;
    if (b->next)
        b->next->prev = tmp;
    tmp->prev = b;
    b->next = tmp;
}

static meta_t *find_free_block(size_t size)
{
    meta_t *b = kernel_base, *tmp = NULL;

    while (b && !(IS_FREE(b) && b->size >= size)) {
        b = b->next;
    }

    if (b != NULL) {
        UNMARK_FREE(b);
        split_block(b, size);
    }
    return b;
}

void *kmalloc(size_t size)
{
    spin_acquire(&lock);

    meta_t *b;

    if ((b = find_free_block(size)) == NULL) {
        kdebug("requesting more memory from kernel!");

        if ((b = morecore(size + META_SIZE)) == NULL) {
            kpanic("kernel heap exhausted");
        }

        split_block(b, size);
        UNMARK_FREE(b);
    }

    kmemset(b + 1, 0, size);

    spin_release(&lock);
    return b + 1;
}

void *kzalloc(size_t size)
{
    void *b = kmalloc(size);

    kmemset(b, 0, size);
    return b;
}

void *kcalloc(size_t nmemb, size_t size)
{
    void *b = kmalloc(nmemb * size);

    kmemset(b, 0, nmemb * size);
    return b;
}

void *krealloc(void *ptr, size_t size)
{
    /* check if merging adjacent blocks achieves the goal size */
    meta_t *b = GET_BASE(ptr), *tmp;
    size_t new_size = b->size;

    if (b->prev && IS_FREE(b->prev)) new_size += b->prev->size + META_SIZE;
    if (b->next && IS_FREE(b->next)) new_size += b->next->size + META_SIZE;

    if (new_size >= size) {
        if (b->prev != NULL && IS_FREE(b->prev)) {
            merge_blocks(b->prev, b);
            kmemmove(b->prev + 1, b + 1, b->size);
            b = b->prev;
        }

        if (b->next != NULL && IS_FREE(b->next)) {
            merge_blocks(b, b->next);
        }
        split_block(b, size);

    } else {
        tmp = find_free_block(size);
        if (!tmp) {
            tmp = morecore(size);
            split_block(tmp, size);
        }

        UNMARK_FREE(tmp);
        kmemcpy(tmp + 1, b + 1, b->size);
        kfree(ptr);
        b = tmp;
    }

    return b + 1;
}

/* mark the current block as free and 
 * merge adjacent free blocks if possible 
 *
 * hell breaks loose if ptr doesn't point to the beginning of memory block */
void kfree(void *ptr)
{
    if (!ptr)
        return;

    spin_acquire(&lock);

    meta_t *b = GET_BASE(ptr), *tmp;
    MARK_FREE(b);

    if (b->prev != NULL && IS_FREE(b->prev)) {
        merge_blocks(b->prev, b);
        b = b->prev;
    }

    if (b->next != NULL && IS_FREE(b->next))
        merge_blocks(b, b->next);

    spin_release(&lock);
}

static int __heap_init(void *heap_start, unsigned order)
{
    if (heap_start == NULL)
        return -EINVAL;

    size_t HEAP_SIZE = PAGE_SIZE * (1 << order);

    HEAP_START = heap_start;
    kernel_base = heap_start;
    kernel_base->size = HEAP_SIZE;
    kernel_base->next = NULL;
    kernel_base->prev = NULL;
    MARK_FREE(kernel_base);
    HEAP_BREAK = (unsigned long *)((unsigned long)HEAP_START + HEAP_SIZE);

    return 0;
}

int mmu_heap_preinit(void)
{
    /* Allocate one page of memory for booting */
    unsigned long mem = mmu_bootmem_alloc_page();

    if (mem == INVALID_ADDRESS) {
        kdebug("failed to allocate memory for heap!");
        return -ENOMEM;
    }

    return __heap_init(mmu_p_to_v(mem), 0);
}

int mmu_heap_init(void)
{
    unsigned long mem = mmu_block_alloc(MM_ZONE_NORMAL, 3);

    if (mem == INVALID_ADDRESS) {
        kdebug("failed to allocate memory for heap!");
        return -ENOMEM;
    }

    return __heap_init(mmu_p_to_v(mem), 4);
}

void heap_initialize(uint32_t *heap_start_v)
{
    HEAP_START = (void *)heap_start_v;

    kernel_base = (meta_t*)HEAP_START;
    kernel_base->size = 0x1000 - META_SIZE;
    kernel_base->next = kernel_base->prev = NULL;
    MARK_FREE(kernel_base);
    HEAP_BREAK = (void *)((unsigned long)HEAP_START + 0x1000);
}

#include <arch/amd64/mm/mmu.h>
#include <kernel/compiler.h>
#include <kernel/kassert.h>
#include <kernel/kprint.h>
#include <kernel/kpanic.h>
#include <kernel/util.h>
#include <mm/bootmem.h>
#include <mm/heap.h>
#include <mm/mmu.h>
#include <mm/page.h>
#include <mm/slab.h>
#include <sync/spinlock.h>
#include <stdbool.h>
#include <sys/types.h>
#include <errno.h>

#define SPLIT_THRESHOLD  8
#define HEAP_ARENA_SIZE  2 /* 1 << 2 */

typedef struct mm_chunk {
    size_t size;
    bool free;
    struct mm_chunk *next;
    struct mm_chunk *prev;
} __packed mm_chunk_t;

typedef struct mm_arena {
    size_t size;
    mm_chunk_t *base;
    spinlock_t lock;
    struct mm_arena *next;
    struct mm_arena *prev;
} __packed mm_arena_t;

static mm_arena_t __mem;
static mm_arena_t __high_prio;
static mm_cache_t *arena_cache;
static bool initialized = false;

void print_heap(void)
{
    size_t total  = 0;
    size_t free   = 0;
    size_t blocks = 0;
    mm_arena_t *iter = &__mem;

    while (iter) {

        mm_chunk_t *b = iter->base;

        while (b) {
            total  += b->size + sizeof(mm_chunk_t);
            blocks += 1;

            if (b->free)
                free += b->size;

            kprint("%x: %u %d\n", b, b->size, b->free);
            b = b->next;
        }

        iter = iter->next;
    }

    kprint("total blocks %u, total %u bytes, free %u bytes\n", blocks, total, free);
}

void __alloc_arena(void)
{
    if (!initialized)
        kpanic("__alloc_arena() not implemented for booting!");

    /* Allocate new arena and initialize it */
    mm_arena_t *arena = mmu_cache_alloc_entry(arena_cache, MM_HIGH_PRIO);
    unsigned long mem = mmu_block_alloc(MM_ZONE_NORMAL, HEAP_ARENA_SIZE, MM_HIGH_PRIO);

    arena->base = mmu_p_to_v(mem);
    arena->size = PAGE_SIZE * (1 << HEAP_ARENA_SIZE);
    arena->lock = 0;

    arena->base->free = 1;
    arena->base->next = NULL;
    arena->base->prev = NULL;
    arena->base->size = PAGE_SIZE * (1 << HEAP_ARENA_SIZE) - sizeof(mm_chunk_t);

    /* add the new arena at the end of the normal heap arena list */
    mm_arena_t *iter = &__mem;

    while (iter->next)
        iter = iter->next;

    spin_acquire(&__mem.lock);

    __mem.next  = arena;
    arena->prev = &__mem;

    spin_release(&__mem.lock);
}

mm_chunk_t *__split_block(mm_chunk_t *block, size_t size)
{
    if (block->size - size < sizeof(mm_chunk_t) + SPLIT_THRESHOLD)
        return block;

    mm_chunk_t *new = (mm_chunk_t *)((uint8_t *)block + sizeof(mm_chunk_t) + size);

    new->size   = block->size - size - sizeof(mm_chunk_t);
    new->free   = 1;
    block->size = size;

    if (block->next)
        block->next->prev = new;

    new->next   = block->next;
    block->next = new;
    new->prev   = block;

    return block;
}

mm_chunk_t *__find_free(mm_arena_t *arena, size_t size)
{
    mm_arena_t *iter = arena;

    while (iter) {
        spin_acquire(&iter->lock);
        mm_chunk_t *block = iter->base;

        while (block && !(block->free && block->size >= size))
            block = block->next;

        if (block) {
            /* TODO: lock current and next block (if exists) */
            /* TODO: release arena lock */

            void *ret = __split_block(block, size);
            spin_release(&iter->lock);
            return ret;
        }

        spin_release(&iter->lock);
        iter = iter->next;
    }

    return NULL;
}

mm_chunk_t *__merge_blocks_prev(mm_chunk_t *cur, mm_chunk_t *prev)
{
    if (!prev || !prev->free)
        return cur;

    mm_chunk_t *head = prev;

    head->size = prev->size + cur->size + sizeof(mm_chunk_t);
    head->next = cur->next;

    if (cur->next)
        cur->next->prev = head;

    return __merge_blocks_prev(prev, prev->prev);
}

void __merge_blocks_next(mm_chunk_t *cur, mm_chunk_t *next)
{
    if (!next || !next->free)
        return;

    cur->size += next->size + sizeof(mm_chunk_t);
    cur->next  = next->next;

    if (next->next)
        next->next->prev = cur;

    __merge_blocks_next(cur, cur->next);
}

void *kmalloc(size_t size, int flags)
{
    mm_chunk_t *block = NULL;

    if (flags & MM_HIGH_PRIO)
        block = __find_free(&__high_prio, size);
    else
        block = __find_free(&__mem, size);

    if (!block) {
        if (flags & MM_HIGH_PRIO)
            kpanic("high priority arena exhausted!");

        __alloc_arena();

        if (!(block = __find_free(&__mem, size)))
            return NULL;
    }

    if (flags & MM_ZERO)
        kmemset(block + 1, 0, size);

    block->free = 0;
    return block + 1;
}

void *kzalloc(size_t size)
{
    void *mem = kmalloc(size, 0);

    kmemset(mem, 0, size);
    return mem;
}

void *kcalloc(size_t nmemb, size_t size)
{
    return kzalloc(nmemb * size);
}

void kfree(void *mem)
{
    /* spin_acquire(&lock); */

    mm_chunk_t *block = (mm_chunk_t *)mem - 1;
    block->free = 1;
    /* kprint("free block %x\n", block); */
    /* block = __merge_blocks_prev(block, block->prev); */
    /* __merge_blocks_next(block, block->next); */

    /* spin_release(&lock); */
}

int mmu_heap_preinit(void)
{
    /* Allocate 16 KB of memory for booting */
    unsigned long mem = mmu_bootmem_alloc_block(2);

    if (mem == INVALID_ADDRESS) {
        kdebug("failed to allocate memory for heap!");
        return -ENOMEM;
    }

    __mem.base = mmu_p_to_v(mem);
    __mem.size = PAGE_SIZE * (1 << 2);
    __mem.lock = 0;

    __mem.base->size = PAGE_SIZE * (1 << 2) - sizeof(mm_chunk_t);
    __mem.base->free = 1;
    __mem.base->next = NULL;
    __mem.base->prev = NULL;

    return 0;
}

int mmu_heap_init(void)
{
    unsigned long mem = mmu_block_alloc(MM_ZONE_NORMAL, HEAP_ARENA_SIZE, 0);
    unsigned long hp  = mmu_block_alloc(MM_ZONE_NORMAL, 1, 0);

    kassert(mem != INVALID_ADDRESS);
    kassert(hp  != INVALID_ADDRESS);

    if (mem == INVALID_ADDRESS || hp == INVALID_ADDRESS)
        kpanic("Failed to allocate physical memory for heap");

    __mem.base = mmu_p_to_v(mem);
    __mem.size = PAGE_SIZE * (1 << HEAP_ARENA_SIZE);
    __mem.lock = 0;

    __mem.base->free = 1;
    __mem.base->next = NULL;
    __mem.base->prev = NULL;
    __mem.base->size = PAGE_SIZE * (1 << HEAP_ARENA_SIZE) - sizeof(mm_chunk_t);

    __high_prio.base = mmu_p_to_v(hp);
    __high_prio.size = PAGE_SIZE * (1 << 1);
    __high_prio.lock = 0;

    __high_prio.base->free = 1;
    __high_prio.base->next = NULL;
    __high_prio.base->prev = NULL;
    __high_prio.base->size = PAGE_SIZE * (1 << 1) - sizeof(mm_chunk_t);

    if (!(arena_cache = mmu_cache_create(sizeof(mm_arena_t), 0)))
        kpanic("Failed to allocate SLAB cache for heap arenas!");

    initialized = true;
    return 0;
}

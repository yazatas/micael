#include <fs/multiboot2.h>
#include <kernel/common.h>
#include <kernel/kpanic.h>
#include <kernel/kassert.h>
#include <lib/bitmap.h>
#include <lib/list.h>
#include <mm/bootmem.h>
#include <mm/cache.h>
#include <mm/heap.h>
#include <mm/page.h>
#include <mm/slab.h>
#include <errno.h>

#define ORDER_EMPTY(o) (o.list.next == NULL)

typedef struct mm_block {
    unsigned long start;
    unsigned long end;
    list_head_t list;
} mm_block_t;

/* first item of every block order list is "dummy" entry
 * which tells if this order has any blocks and if so,
 * points to the first block of the order */
typedef struct mm_zone {
    const char *name;
    size_t page_count;
    mm_block_t blocks[BUDDY_MAX_ORDER];
} mm_zone_t;

static mm_zone_t zone_dma;
static mm_zone_t zone_normal;
static mm_zone_t zone_high;
static mm_cache_t *mm_block_cache;

static inline mm_zone_t *__get_zone(unsigned long start, unsigned long end)
{
    if (end < MM_ZONE_DMA_END)
        return &zone_dma;

    if (start >= MM_ZONE_NORMAL_START && end < MM_ZONE_NORMAL_END)
        return &zone_normal;

    if (start >= MM_ZONE_HIGH_START && end < MM_ZONE_HIGH_END)
        return &zone_high;

    return NULL;
}

static int __zone_add_block(mm_zone_t *zone, unsigned long start, int order)
{
    mm_block_t *block = mmu_cache_alloc_entry(mm_block_cache, MM_NO_FLAG);

    if (block == NULL)
        return -errno;

    block->start = start;
    block->end   = start + PAGE_SIZE * (1 << order) - 1;

    list_init_null(&block->list);
    list_append(&zone->blocks[order].list, &block->list);

    return 0;
}

void mmu_zones_init(void *arg)
{
    zone_dma.name    = "MM_ZONE_DMA";
    zone_normal.name = "MM_ZONE_NORMAL";
    zone_high.name   = "MM_ZONE_HIGH";

    zone_dma.page_count    = 0;
    zone_normal.page_count = 0;
    zone_high.page_count   = 0;

    for (size_t i = 0; i < BUDDY_MAX_ORDER; ++i) {
        zone_dma.blocks[i].start    = 0;
        zone_dma.blocks[i].end      = 0;

        zone_normal.blocks[i].start = 0;
        zone_normal.blocks[i].end   = 0;

        zone_high.blocks[i].start   = 0;
        zone_high.blocks[i].end     = 0;

        list_init_null(&zone_dma.blocks[i].list);
        list_init_null(&zone_normal.blocks[i].list);
        list_init_null(&zone_high.blocks[i].list);
    }

    if ((mm_block_cache = mmu_cache_create(sizeof(mm_block_t), MM_NO_FLAG)) == NULL)
        kpanic("Failed to allocate mm_block_cache for PFA");

    /* initialize memory zones using the multiboot memory map */
    multiboot2_map_memory(arg, mmu_claim_range);

    /* We have now mapped all memory for the PFA but during boot
     * we allocated memory pages from boot memory manager.
     *
     * We must now "claim" these pages and set them as occupied
     * to prevent reallocation. The boot process is very straightforward right now:
     *  - alloc page for heap
     *  - alloc 2 pages for slab
     *  - alloc slab cache for pfa
     *  -> done
     *
     * This means that we must mark the these three pages (found at the start of DMA zone)
     * as occupied in the zone (and if they're part of a larger block, split the block) */
    int entries = 0;
    struct bootmem_mmap **map = mmu_bootmem_release(&entries);

    /* TODO: i need to rethink this.. */
}

static void __claim_range(mm_zone_t *zone, unsigned long start, unsigned long end)
{
    kassert(zone != NULL);

    unsigned long range_len = end - start;

    if (range_len < PAGE_SIZE)
        return;

    for (int order = BUDDY_MAX_ORDER - 1; order >= 0; --order) {
        size_t BLOCK_SIZE = (1 << order) * PAGE_SIZE;
        size_t num_blocks = range_len / BLOCK_SIZE;
        size_t rem_bytes  = range_len % BLOCK_SIZE;

        if (num_blocks > 0) {
            zone->page_count += (1 << order);

            /* kdebug("%u (%d) satisfies %u: %u, %u", BLOCK_SIZE, order, range_len, num_blocks, rem_bytes); */

            for (size_t b = 0; b < num_blocks; ++b) {
                __zone_add_block(zone, start, order);
                start        = start + (b + 1) * BLOCK_SIZE;
            }

            range_len = rem_bytes;
        }
    }
}

void mmu_claim_range(unsigned long address, size_t len)
{
    mm_zone_t *zone = __get_zone(address, address + len);

    if (zone != NULL) {
        /* round up and down to get usable boundaries */
        __claim_range(
            zone,
            ROUND_UP(address, PAGE_SIZE),
            ROUND_DOWN(address + len, PAGE_SIZE)
        );
    }

    /* There are two possibilities for the range to be NULL:
     * - the address range given was invalid (rarely the case)
     * - the address range given overlaps two (or more) zones
     *
     * We must check here which is the case and if it's the overlapping case,
     * deal with the error by splitting the range into two smaller ranges */
    if (address < MM_ZONE_NORMAL_START && address + len > MM_ZONE_DMA_END) {
        __claim_range(
            &zone_dma,
            ROUND_UP(address, PAGE_SIZE),
            MM_ZONE_DMA_END
        );

        __claim_range(
            &zone_normal,
            MM_ZONE_NORMAL_START,
            ROUND_DOWN(address + len, PAGE_SIZE)
        );
    }
}

void mmu_claim_page(unsigned long address)
{
    mmu_claim_range(address, PAGE_SIZE);
}

unsigned long mmu_page_alloc(unsigned memzone)
{
    return mmu_block_alloc(memzone, 0);
}

unsigned long mmu_block_alloc(unsigned memzone, unsigned order)
{
    /* TODO: this is temporary, pfa and bootmem need better cooperation but
     * right now I'll focus my attention to finalizing x86_64 support */
    kassert(memzone == MM_ZONE_NORMAL);

    if (order > BUDDY_MAX_ORDER ||
        (memzone & ~(MM_ZONE_DMA | MM_ZONE_NORMAL | MM_ZONE_HIGH)) != 0)
    {
        errno = EINVAL;
        return INVALID_ADDRESS;
    }

    /* Usually either MM_ZONE_DMA or MM_ZONE_NORMAL is requested, so try to
     * satisfy the request using MM_ZONE_NORMAL and save the DMA for when it's actually needed.
     *
     * If the MM_ZONE_NORMAL does not contain a block large enough, MM_ZONE_DMA can be used
     * though MM_ZONE_NORMAL should be prioritized above all else */
    mm_zone_t *zone = NULL;

    if (memzone & MM_ZONE_NORMAL)
        zone = &zone_normal;

    else if (memzone & MM_ZONE_DMA)
        zone = &zone_dma;

    else if (memzone & MM_ZONE_HIGH)
        zone = &zone_high;

    for (size_t o = order; o < BUDDY_MAX_ORDER; ++o) {
        if (ORDER_EMPTY(zone->blocks[o]))
            continue;

        /* There's a free block available in the order list caller requested
         * Hanle this as a special case because it's cleaner */
        /* if (o == order) { */
            mm_block_t *b = container_of(zone->blocks[o].list.next, mm_block_t, list);

            unsigned long start = b->start;

            list_remove(&b->list);
            (void)mmu_cache_free_entry(mm_block_cache, b);

            return start;
        /* } */
    }

    errno = ENOMEM;
    return INVALID_ADDRESS;
}

int mmu_block_free(unsigned long address, unsigned order)
{
    if (!PAGE_ALIGNED(address))
        return -EINVAL;

    mm_zone_t *zone = __get_zone(address, address + (1 << order) * PAGE_SIZE);

    if (zone == NULL)
        return -ENXIO;

    return __zone_add_block(zone, address, order);
}

int mmu_page_free(unsigned long address)
{
    return mmu_block_free(address, 0);
}

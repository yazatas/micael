#include <fs/multiboot2.h>
#include <kernel/common.h>
#include <kernel/kpanic.h>
#include <kernel/kassert.h>
#include <kernel/util.h>
#include <lib/bitmap.h>
#include <lib/list.h>
#include <mm/bootmem.h>
#include <mm/heap.h>
#include <mm/mmu.h>
#include <mm/page.h>
#include <mm/slab.h>
#include <errno.h>
#include <stdbool.h>

#define ORDER_EMPTY(o) (o.list.next == NULL)

typedef int (*add_block_t)(void *, unsigned long, unsigned);

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

static mm_zone_t  zone_dma;
static mm_zone_t  zone_normal;
static mm_zone_t  zone_high;
static mm_cache_t *mm_block_cache;
static page_t     *page_array;

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

static int __zone_add_block(void *param, unsigned long start, unsigned order)
{
    mm_zone_t *zone   = (mm_zone_t *)param;
    mm_block_t *block = mmu_cache_alloc_entry(mm_block_cache, MM_ZERO);

    if (block == NULL)
        return -errno;

    block->start = start;
    block->end   = start + PAGE_SIZE * (1 << order) - 1;

    list_init_null(&block->list);
    list_append(&zone->blocks[order].list, &block->list);
    zone->page_count += (1 << order);

    return 0;
}

static int __page_array_add_block(void *param, unsigned long start, unsigned order)
{
    kassert(param != NULL);

    unsigned type = *(unsigned *)param;
    unsigned pfn  = start >> PAGE_SHIFT;

    page_array[pfn].type  = type;
    page_array[pfn].order = order;
    page_array[pfn].first = 1;

    for (int i = 1; i < (1 << order); ++i) {
        page_array[pfn + i].type  = type;
        page_array[pfn + i].order = order;
        page_array[pfn + i].first = 0;
    }

    return 0;
}

static void __claim_range(unsigned long start, unsigned long end, add_block_t callback, void *cb_param)
{
    kassert(callback != NULL);
    kassert(cb_param != NULL);

    unsigned long range_len = end - start;

    if (range_len < PAGE_SIZE)
        return;

    for (int order = BUDDY_MAX_ORDER - 1; order >= 0; --order) {
        size_t BLOCK_SIZE = (1 << order) * PAGE_SIZE;
        size_t num_blocks = range_len / BLOCK_SIZE;
        size_t rem_bytes  = range_len % BLOCK_SIZE;

        if (num_blocks > 0) {
            for (size_t b = 0; b < num_blocks; ++b) {
                (void)callback(cb_param, start, order);
                start = start + (b + 1) * BLOCK_SIZE;
            }

            range_len = rem_bytes;
        }
    }
}

/* get next free entry from "zone" of "order" and remove it from the free list */
static mm_block_t *__get_free_entry(mm_zone_t *zone, unsigned order)
{
    kassert(zone != NULL);
    kassert(order < BUDDY_MAX_ORDER);

    mm_block_t *b = container_of(zone->blocks[order].list.next, mm_block_t, list);
    list_remove(&b->list);
    list_init_null(&b->list);

    return b;
}

/* Split the block of order "split_order" into smaller blocks and keep splitting until
 * we've reached a half of a block that satisfies the request "req_order" */
static unsigned long __split_block(mm_zone_t *zone, unsigned req_order, unsigned split_order)
{
    kassert(split_order != 0 && req_order < BUDDY_MAX_ORDER);

    mm_block_t *b = __get_free_entry(zone, split_order);

    unsigned long split_size  = (1 << (split_order - 1)) * PAGE_SIZE;
    unsigned long split_start = b->start + split_size;

    /* modify the end of the original block to create "new" smaller block */
    b->end -= split_size;
    list_append(&zone->blocks[--split_order].list, &b->list);

    while (req_order != split_order) {
        mm_block_t *tmp = mmu_cache_alloc_entry(mm_block_cache, MM_ZERO);

        tmp->start  = split_start;
        split_size  = (1 << (split_order - 1)) * PAGE_SIZE;
        split_start = tmp->start + split_size;

        tmp->end -= split_size;
        tmp->end = tmp->start + split_size - 1;
        list_append(&zone->blocks[--split_order].list, &tmp->list);
    }

    return split_start;
}

/* Allocate block of memory from requested zone
 *
 * This function can fail and it return INVALID_ADDRESS on error
 * and pointer to valid block of memory on succes */
static unsigned long __alloc_mem(unsigned memzone, unsigned order)
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

    for (unsigned o = order; o < BUDDY_MAX_ORDER; ++o) {
        if (ORDER_EMPTY(zone->blocks[o]))
            continue;

        /* There's a free block available in the order's list caller requested
         * Hanle this as a special case because it's cleaner */
        if (o == order) {
            mm_block_t *b = __get_free_entry(zone, o);

            unsigned long start = b->start;
            (void)mmu_cache_free_entry(mm_block_cache, b);

            return start;
        }

        return __split_block(zone, order, o);
    }

    errno = ENOMEM;
    return INVALID_ADDRESS;
}

/* claim only available/reclaimable memory for zones */
static void __claim_range_preinit(unsigned type, unsigned long address, size_t len)
{
    if (type != MULTIBOOT_MEMORY_AVAILABLE &&
        type != MULTIBOOT_MEMORY_ACPI_RECLAIMABLE)
        return;

    return mmu_claim_range(address, len);
}

/* claim all memory but update only the page array */
static void __claim_range_postinit(unsigned type, unsigned long address, size_t len)
{
    unsigned mm_type = MM_PT_INVALID;

    switch (type) {
        case MULTIBOOT_MEMORY_AVAILABLE:
        case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
            mm_type = MM_PT_FREE;
            break;

        case MULTIBOOT_MEMORY_RESERVED:
        case MULTIBOOT_MEMORY_NVS:
        case MULTIBOOT_MEMORY_BADRAM:
            mm_type = MM_PT_IN_USE;
            break;
    }

    __claim_range(
        ROUND_UP(address, PAGE_SIZE),
        ROUND_DOWN(address + len, PAGE_SIZE),
        __page_array_add_block,
        &mm_type
    );
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

    if ((mm_block_cache = mmu_cache_create(sizeof(mm_block_t), MM_NO_FLAGS)) == NULL)
        kpanic("Failed to allocate mm_block_cache for PFA");

    /* Memory is claimed twice: on the first run we're only interested in free memory
     * because we're initializing the zones. When zones and the page array have been initialized
     * we reclaim all memory but this time we're only updating the page array */
    multiboot2_map_memory(arg, __claim_range_preinit);

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
    /* TODO: i need to rethink this.. */
    int entries = 0;
    struct bootmem_mmap **map = mmu_bootmem_release(&entries);

    /* Create page array for all physical memory (current max: 2GB bytes)
     *
     * Each page maps 4096 bytes of memory so we need 2GB / 4096 == 0x800000
     * entries to hold the entire page array in memory and each entry is sizeof(page_t)
     * bytes of memory so in total we need 0x800000 * sizeof(page_t) which is 10MB of memory
     * but due to lack of granularity we need to allocate a much larger block
     *
     * Use the internal allocation function to allocate the block because currently
     * the page_array points to NULL */
    unsigned long pa_mem = __alloc_mem(MM_ZONE_NORMAL, 12);

    if (pa_mem == INVALID_ADDRESS)
        kpanic("failed to allocate memory for page array");

    page_array = mmu_p_to_v(pa_mem);

    /* initially mark all memory as invalid
     * (even the parts that multiboot2 memory doesn't contain) */
    kmemset(page_array, MM_PT_INVALID, (1 << 12) * PAGE_SIZE);

    /* Mark areas of page array to free/occupied using multiboot2 memory map */
    multiboot2_map_memory(arg, __claim_range_postinit);

    /* set the memory consumed by the page array as in use separately
     * because multiboot2_map_memory() has marked it as free */
    (void)__page_array_add_block(&(unsigned){ MM_PT_IN_USE }, pa_mem, 12);
}

void mmu_claim_range(unsigned long address, size_t len)
{
    mm_zone_t *zone = __get_zone(address, address + len);

    if (zone != NULL) {
        /* round up and down to get usable boundaries */
        __claim_range(
            ROUND_UP(address, PAGE_SIZE),
            ROUND_DOWN(address + len, PAGE_SIZE),
            __zone_add_block,
            zone
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
            ROUND_UP(address, PAGE_SIZE),
            MM_ZONE_DMA_END,
            __zone_add_block,
            &zone_dma
        );

        __claim_range(
            MM_ZONE_NORMAL_START,
            ROUND_DOWN(address + len, PAGE_SIZE),
            __zone_add_block,
            &zone_normal
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
    unsigned long address = __alloc_mem(memzone, order);

    if (address == INVALID_ADDRESS) {
        kpanic("out of memory");
        return INVALID_ADDRESS;
    }

    (void)__page_array_add_block(&(unsigned){ MM_PT_IN_USE }, address, order);

    return address;
}

int mmu_block_free(unsigned long address, unsigned order)
{
    if (!PAGE_ALIGNED(address))
        return -EINVAL;

    mm_zone_t *zone = __get_zone(address, address + (1 << order) * PAGE_SIZE);

    if (zone == NULL)
        return -ENXIO;

    (void)__page_array_add_block(&(unsigned){ MM_PT_FREE }, address, order);

    return __zone_add_block(zone, address, order);
}

int mmu_page_free(unsigned long address)
{
    return mmu_block_free(address, 0);
}

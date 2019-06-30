#include <fs/multiboot2.h>
#include <kernel/common.h>
#include <kernel/kassert.h>
#include <lib/bitmap.h>
#include <mm/bootmem.h>
#include <mm/heap.h>
#include <mm/types.h>

struct {
    struct {
        unsigned long start;
        size_t len;
        uint32_t mem[256]; /* hold 256 * 32 pages */
        bitmap_t bm;
    } ranges[5];

    size_t ptr;
} mem_info;

static void bootmem_claim_range(unsigned type, unsigned long addr, size_t len)
{
    if (type != MULTIBOOT_MEMORY_AVAILABLE &&
        type != MULTIBOOT_MEMORY_ACPI_RECLAIMABLE)
        return;

    if (mem_info.ptr >= 5)
        return;

    /* how many pages this range contains? */
    size_t npages = ROUND_DOWN(len, PAGE_SIZE) / PAGE_SIZE;

    if (npages == 0)
        return;

    kdebug("0x%x - 0x%x (%u, %u)", addr, addr + len, len, npages);

    mem_info.ranges[mem_info.ptr].start = ROUND_UP(addr, PAGE_SIZE);
    bm_unset_range(&mem_info.ranges[mem_info.ptr].bm, 0, npages);

    mem_info.ptr++;
}

int mmu_bootmem_init(void *arg)
{
    for (int i = 0; i < 5; ++i) {
        mem_info.ranges[i].bm.bits = mem_info.ranges[i].mem;
        mem_info.ranges[i].start   = INVALID_ADDRESS;
        mem_info.ranges[i].bm.len  = 256 * 32;

        bm_set_range(&mem_info.ranges[i].bm, 0, 256 * 32 - 1);
    }

    mem_info.ptr = 0;

    multiboot2_map_memory(arg, bootmem_claim_range);
}

struct bootmem_mmap **mmu_bootmem_release(int *num_entries)
{
    kassert(num_entries != NULL);

    bootmem_mmap_t **map = kmalloc(sizeof(bootmem_mmap_t *) * 5);

    for (size_t i = 0; i < 5; ++i) {
        int start = bm_find_first_unset(
            &mem_info.ranges[i].bm,
            0,
            mem_info.ranges[i].bm.len - 1
        );

        if (start == BM_NOT_FOUND_ERROR) {
            map[i] = NULL;
            continue;
        }

        /* the boot memory allocator is very simple, it allocates the pages in order so
         * I we allocate page 0x8000 now, the next page on the list is 0x9000 
         *
         * "start" marks the next available page. From that we can calculate the amount of pages in use */
        map[i] = kmalloc(sizeof(bootmem_mmap_t));

        map[i]->start  = mem_info.ranges[i].start;
        map[i]->npages = start;
        (*num_entries)++;
    }

    return map;
}

unsigned long mmu_bootmem_alloc_block(size_t npages)
{
    for (size_t i = 0; i < mem_info.ptr; ++i) {
        int start = bm_find_first_unset_range(
                &mem_info.ranges[i].bm,
                0,                              /* search range start */
                mem_info.ranges[i].bm.len - 1,  /* search range end */
                npages                          /* num unset elements */
        );

        if (start == BM_NOT_FOUND_ERROR)
            continue;

        /* range found, set it as occupied and return pointer to the start to user */
        if (npages == 1) {
            if (bm_set_bit(&mem_info.ranges[i].bm, start) != 0)
                return INVALID_ADDRESS;
        } else if (bm_set_range(&mem_info.ranges[i].bm, start, start + npages) != 0) {
            return INVALID_ADDRESS;
        }

        return mem_info.ranges[i].start + PAGE_SIZE * start;
    }

    return INVALID_ADDRESS;
}

unsigned long mmu_bootmem_alloc_page(void)
{
    return mmu_bootmem_alloc_block(1);
}

void mmu_bootmem_free(unsigned long addr, size_t npages)
{
    (void)addr, (void)npages;
}

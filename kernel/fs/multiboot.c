#include <kernel/kprint.h>
#include <kernel/common.h>
#include <fs/multiboot.h>
#include <mm/vmm.h>

size_t multiboot_map_memory(multiboot_info_t *mbinfo)
{
    multiboot_memory_map_t *mmap = (multiboot_memory_map_t *)mbinfo->mmap_addr;
    uint32_t entries = mbinfo->mmap_length / sizeof(multiboot_memory_map_t);
    uint32_t start, end, cur;
    size_t numpages = 0;

    for (size_t i = 0; i < entries; ++i) {
        if (mmap[i].type == MULTIBOOT_MEMORY_AVAILABLE) {
            end   = mmap[i].addr + mmap[i].len;

            start = ROUND_UP(mmap[i].addr, PAGE_SIZE);
            end   = ROUND_DOWN(end, PAGE_SIZE);

            kprint("\tmemory region boundaries:\n");
            kprint("\t\tstart address: 0x%x\n", start);
            kprint("\t\tend address:   0x%x\n", end);

            for (cur = start; cur <= end; cur += 0x1000) {
                vmm_claim_page(cur);
                numpages++;
            }
            kprint("\t\tmaps:          %u pages\n", numpages);
        }
    }

    return numpages;
}

#include <kernel/kprint.h>
#include <kernel/common.h>
#include <fs/multiboot.h>
#include <mm/mmu.h>

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
                mmu_claim_page(cur);
                numpages++;
            }
            kprint("\t\tmaps:          %u pages\n", numpages);
        }
    }

    return numpages;
}

size_t multiboot_load_modules(multiboot_info_t *mbi)
{
    /* multiboot_module_t *mods = (multiboot_module_t *)mbinfo->mods_addr; */
    /* uint32_t entries = mbinfo->mods_count; */
    /* kdebug("mod entries %u", entries); */
    /* return (size_t)entries; */


    multiboot_module_t *mod;
    size_t i;
    kdebug("mods_count = %d, mods_addr = 0x%x\n",
            (int) mbi->mods_count, (int) mbi->mods_addr);
    for (i = 0, mod = (multiboot_module_t *) mbi->mods_addr;
         i < mbi->mods_count;
         i++, mod++)
        kdebug(" mod_start = 0x%x, mod_end = 0x%x, cmdline = %s\n",
                (unsigned) mod->mod_start,
                (unsigned) mod->mod_end,
                (char *) mod->cmdline);

    return 0;
}

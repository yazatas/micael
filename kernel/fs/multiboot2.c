#include <fs/multiboot2.h>
#include <mm/mmu.h>
#include <sys/types.h>
#include <kernel/common.h>

size_t multiboot2_map_memory(unsigned long *address)
{
    struct multiboot_tag *tag;
    multiboot_memory_map_t *mmap;

    for (tag = (struct multiboot_tag *)(address + 8);
         tag->type != MULTIBOOT_TAG_TYPE_END;
         tag = (struct multiboot_tag *)((multiboot_uint8_t *)tag + ((tag->size + 7) & ~7)))
    {
        switch (tag->type) {
            case MULTIBOOT_TAG_TYPE_MMAP: {
                mmap = ((multiboot_tag_mmap_t *)tag)->entries;

                do {
                    unsigned long addr = (mmap->addr >> 32) | (mmap->addr & 0xffffffff);
                    unsigned long len  = (mmap->len  >> 32) | (mmap->len  & 0xffffffff);

                    switch (mmap->type) {
                        case MULTIBOOT_MEMORY_AVAILABLE:
                        case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE: {
                            unsigned long start = ROUND_UP(addr, PAGE_SIZE);
                            unsigned long end   = ROUND_DOWN((addr + len), PAGE_SIZE);

                            for (; start < len; start += PAGE_SIZE) {
                                mmu_claim_page(start);
                            }
                        }
                        break;
                    }

                    mmap = (multiboot_memory_map_t *)((unsigned long)mmap + ((multiboot_tag_mmap_t *)tag)->entry_size);
                } while ((multiboot_uint8_t *)mmap < (multiboot_uint8_t *)tag + tag->size);
            }
            break;
        }
    }
}

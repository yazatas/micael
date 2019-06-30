#ifndef __BOOTMEM_H__
#define __BOOTMEM_H__

#include <sys/types.h>

typedef struct bootmem_mmap {
    unsigned long start;
    size_t npages;
} bootmem_mmap_t;

/* Build temporary memory map using the multiboot2 memory map */
int mmu_bootmem_init(void *arg);

bootmem_mmap_t **mmu_bootmem_release(int *num_entries);

/* Allocate "npages" * PAGE_SIZE bytes of memory and
 * return pointer to the beginning of the range 
 *
 * Return INVALID_ADDRESS if allocation fails */
unsigned long mmu_bootmem_alloc_block(size_t npages);

/* Alloc PAGE_SIZE bytes and return the address of the physical page frame 
 *
 * Return INVALID_ADDRESS address if the allocation fails */
unsigned long mmu_bootmem_alloc_page(void);

/* Release the memory back to boot memory allocator */
void mmu_bootmem_free(unsigned long addr, size_t npages);

#endif /* __BOOTMEM_H__ */

#ifndef __MMU_H__
#define __MMU_H__

#include <mm/types.h>
#include <sys/types.h>

enum {
    MM_PRESENT    = 1,
    MM_READWRITE  = 1 << 1,
    MM_READONLY   = 0 << 1,
    MM_USER       = 1 << 2,
    MM_WR_THROUGH = 1 << 3,
    MM_D_CACHE    = 1 << 4,
    MM_ACCESSED   = 1 << 5,
    MM_SIZE_4MB   = 1 << 6,
#ifdef __x86_64__
    MM_2MB        = 1 << 7, // TODO
#endif
    MM_COW        = 1 << 9,
} MM_FLAGS;

/* Initialize the Memory Management Unit:
 *  - native MMU
 *  - page frame allocator
 *  
 * Native MMU mainly switches from the boot page directory to properly
 * initialized directory.
 *
 * Page frame allocator initializes kernel memory pools
 *
 * Return 0 on success and -errno on error TODO: list possible errors */
int mmu_init(void *arg);

/* Allocate empty page directory with kernel mapped to it
 * TODO:
 *
 * Return pointer to page directory on succes
 * Return NULL on error and set errno accordingly */
cr3_t *mmu_alloc_cr3();

/* Create a duplicate page directory of directory pointed to by "cr3"
 * Returned page directory is an exact copy of the parameter
 *
 * Return pointer to page directory on succes
 * Return NULL on error and set errno accordingly */
cr3_t *mmu_duplicate_cr3(cr3_t *cr3);

/* TODO:  */
void mmu_switch_cr3(cr3_t *cr3);

/* Deallocate the space consumed by page directory */
int mmu_dealloc_cr3(cr3_t *cr3);

/* Map "page" to page directory pointed to by "cr3" at "address"
 *
 * Return 0 success
 * Return -errno on error TODO: list possible errors */
/* int mmu_map_page(cr3_t *cr3, unsigned long address, page_t *page); */

/* Unmap page from page directory "cr3" at "address"
 *
 * Return 0 on success
 * Return -errno on error TODO: list possible errors */
int mmu_unmap_page(cr3_t *cr3, unsigned long address);

/* Same as mmu_unmap_page but the page is mapped from
 * currently running process' page directory
 *
 * Return 0 on success
 * Return -errno on error TODO: list possible errors */
int mmu_unmap_page_self(unsigned long address);

/* Map "page" to currently running process' address space at "address"
 *
 * Return 0 success
 * Return -errno on error TODO: list possible errors */
int mmu_map_page_self(unsigned long address, page_t *page);


// TODO REMOVE
void mmu_unmap_pages(size_t start, size_t end);
void *mmu_build_pagedir(void);
void mmu_free_page(page_t page);
void mmu_map_page(void *physaddr, void *virtaddr, uint32_t flags);
page_t mmu_alloc_page(void);
void *mmu_alloc_addr(size_t range);
void mmu_claim_page(uint32_t physaddr);
void mmu_free_addr(void *addr, size_t range);
void *mmu_duplicate_pdir(void);
void mmu_pf_handler(uint32_t error);
void mmu_map_range(void *physaddr, void *virtaddr, size_t range, uint32_t flags);
void mmu_list_pde(void);
void mmu_print_memory_map(void);
void *mmu_v_to_p(void *virtaddr);

static inline void mmu_flush_TLB(void)
{
    asm volatile ("nop");
}

#endif /* __MMU_H__ */

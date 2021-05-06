#ifndef __PAGE_H__
#define __PAGE_H__

#include <mm/types.h>

/* Initialize the memory zones according to memory map pointed to by "arg" */
void mmu_zones_init(void *arg);

/* Allocate one page of memory from requested memory zone
 * The size of returned chunk is [(1 << order) * PAGE_SIZE] bytes
 *
 * Return pointer to allocated page structure on success
 * Return INVALID_ADDRESS on error and set errno */
unsigned long mmu_block_alloc(unsigned memzone, unsigned order, int flags);

/* Allocate one page of memory from requested memory zone
 * This is the same as calling: mmu_block_alloc(zone, 0);
 *
 * Return pointer to allocated page structure on success
 * Return INVALID_ADDRESS on error and set errno */
unsigned long mmu_page_alloc(unsigned memzone, int flags);

/* Release page starting address "address"
 * Try to coalesce with buddy if possible
 *
 * Return 0 on success
 * Return -EINVAL if address is not page-aligned
 * Return -ENXIO if the address doesn't point to a valid zone */
int mmu_block_free(unsigned long address, unsigned order);

/* Release page starting address "address"
 * This is the same as calling: mmu_block_free(address, 0);
 *
 * Try to coalesce with buddy if possible
 *
 * Return 0 on success
 * Return -EINVAL if address is not page-aligned
 * Return -ENXIO if the address doesn't point to a valid zone */
int mmu_page_free(unsigned long address);

/* Compute correct zone for the page and mark that page as free
 * Try to coalesce adjancent blocks if possible
 *
 * This is the same as calling mmu_claim_range(address, PAGE_SIZE); */
void mmu_claim_page(unsigned long address);

/* Compute correct zone for the address range and mark that range free
 * Try to coalesce adjancent blocks if possible */
void mmu_claim_range(unsigned long address, size_t len);

#endif /* __PAGE_H__ */

#ifndef __MMU_H__
#define __MMU_H__

#include <stdint.h>

/* each page dir element points to one page table */
uint32_t page_dir[1024]   __attribute__((aligned(4096)));
uint32_t page_table[1024] __attribute__((aligned(4096)));

void mmu_init(void);
void *get_physaddr(void *virtaddr);

#endif /* end of include guard: __MMU_H__ */

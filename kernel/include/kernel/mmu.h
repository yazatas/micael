#ifndef __MMU_H__
#define __MMU_H__

#include <stdint.h>
#include <stddef.h>

#define PF_SIZE             0x1000
#define PAGE_SIZE           0x1000
#define PE_SIZE             0x80000
#define KALLOC_NO_MEM_ERROR 0xffffffff

enum {
	P_PRESENT    = 1,
	P_READWRITE  = 1 << 1,
	P_PRIV_USER  = 1 << 2,
} PAGING_FLAGS;

typedef uint32_t pageframe_t;

/* general stuff */
void mmu_init(void);
void flush_tlb(void);
void pf_handler(uint32_t error);
void *get_physaddr(void *virtaddr);
void map_page(void *physaddr, void *virtaddr, uint32_t flags);

/* physical page allocation/deallocation  */
uint32_t kalloc_frame(void);
void kfree_frame(uint32_t frame);

#endif /* end of include guard: __MMU_H__ */

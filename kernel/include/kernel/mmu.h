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
} PAGING_FLAGS;

typedef uint32_t pageframe_t;

/* general stuff */
void mmu_init(void);
void pf_handler(uint32_t error);
void *get_physaddr(void *virtaddr);
void map_page(void *physaddr, void *virtaddr, uint32_t flags);

/* allocation/deallocation  */
pageframe_t kalloc_frame(void);
void kfree_frame(pageframe_t frame);

/* void *kmalloc(size_t size); */
/* void *kcalloc(size_t nmemb, size_t size); */
/* void *krealloc(void *ptr, size_t size); */
/* void kfree(void *ptr); */
/* void traverse(); */

#endif /* end of include guard: __MMU_H__ */

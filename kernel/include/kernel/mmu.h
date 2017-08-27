#ifndef __MMU_H__
#define __MMU_H__

#include <stdint.h>
#include <stddef.h>

#define PF_SIZE             0x1000
#define PAGE_SIZE           0x1000
#define PE_SIZE             0x80000
#define KALLOC_NO_MEM_ERROR 0xffffffff

typedef uint32_t pageframe_t;

/* general stuff */
void mmu_init(void);
void pf_handler(uint32_t error);
void *get_physaddr(void *virtaddr);

/* allocation/deallocation  */
pageframe_t kalloc_frame(void);
void kfree_frame(pageframe_t frame);

#endif /* end of include guard: __MMU_H__ */

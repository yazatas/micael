#ifndef __MMU_H__
#define __MMU_H__

#include <stdint.h>

#define PF_SIZE             0x1000
#define KALLOC_NO_MEM_ERROR 0xffffffff

typedef uint32_t pageframe_t;

void mmu_init(void);
void *get_physaddr(void *virtaddr);

void kfree_frame(pageframe_t frame);
pageframe_t kalloc_frame(void);

#endif /* end of include guard: __MMU_H__ */

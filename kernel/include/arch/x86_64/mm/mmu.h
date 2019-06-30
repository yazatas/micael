#ifndef __X86_64_MMU_H__
#define __X86_64_MMU_H__
#ifdef __x86_64__

#include <mm/types.h>

int mmu_native_init(void);

int mmu_native_map_page(unsigned long paddr, unsigned long vaddr, int flags);
int mmu_native_unmap_page(unsigned long vaddr);

unsigned long mmu_native_v_to_p(void *vaddr);
void *mmu_native_p_to_v(unsigned long paddr);

#endif /* __x86_64__ */
#endif /* __X86_64_MMU_H__ */

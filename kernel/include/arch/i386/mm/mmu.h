#ifndef __ARCH_I386_MMU_H__
#define __ARCH_I386_MMU_H__
#ifdef __i386__

#include <fs/multiboot.h>

void mmu_native_init(multiboot_info_t *mbinfo);
cr3_t *mmu_native_alloc_cr3();
cr3_t *mmu_native_duplicate_cr3(cr3_t *cr3);
int mmu_native_dealloc_cr3(cr3_t *cr3);

#endif /* __i386__ */
#endif /* __ARCH_I386_MMU_H__ */


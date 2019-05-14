#ifndef __X86_64_MMU_H__
#define __X86_64_MMU_H__
#ifdef __x86_64__

int mmu_native_init(void);

cr3_t *mmu_native_alloc_cr3();
cr3_t *mmu_native_duplicate_cr3(cr3_t *cr3);
int mmu_native_dealloc_cr3(cr3_t *cr3);
void mmu_native_switch_cr3(cr3_t *cr3);

int mmu_native_map_page(cr3_t *cr3, unsigned long address, page_t *page);
int mmu_native_map_page_self(unsigned long address, page_t *page);

#endif /* __x86_64__ */
#endif /* __X86_64_MMU_H__ */

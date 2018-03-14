#ifndef __MMU_H__
#define __MMU_H__

#include <stdint.h>
#include <stddef.h>

#define PF_SIZE             0x1000
#define PAGE_SIZE           0x1000
#define PE_SIZE             0x80000
#define KALLOC_NO_MEM_ERROR 0xffffffff

enum {
	P_PRESENT   = 1,
	P_READWRITE = 1 << 1,
	P_PRIV_USER = 1 << 2,
} PAGING_FLAGS;

typedef uint32_t pageframe_t;

/* initialization */
void vmm_init(void);
void vmm_heap_init(void);

void *vmm_mkpte(void *pdir, void *ptbl, uint32_t flags);
void *vmm_mkpde(void *pdir, uint32_t flags);
void *vmm_mkpdir(void *virtaddr, uint32_t flags);

void vmm_flush_TLB(void);
void vmm_map_kernel(void *physaddr);
void vmm_map_page(void *physaddr, void *virtaddr, uint32_t flags);

void vmm_pf_handler(uint32_t error);
void *vmm_v_to_p(void *virtaddr);
void vmm_change_pdir(void *cr3);

pageframe_t vmm_kalloc_frame(void);
void        vmm_kfree_frame(pageframe_t frame);

/* kernel heap API */
void *kmalloc(size_t size);
void *kcalloc(size_t nmemb, size_t size);
void *krealloc(void *ptr, size_t size);
void  kfree(void *ptr);

#endif /* end of include guard: __MMU_H__ */

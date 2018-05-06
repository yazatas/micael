#ifndef __VMM_H__
#define __VMM_H__

#include <stdint.h>
#include <stddef.h>

#define PF_SIZE             0x1000
#define PAGE_SIZE           0x1000
#define PE_SIZE             0x80000
#define KALLOC_NO_MEM_ERROR 0xffffffff
#define KSTART              768
#define KSTART_HEAP         832
#define KEND_HEAP           895

/* TODO: add more consistency ie. rewrite */
enum {
    P_PRESENT    = 1,
    P_READWRITE  = 1 << 1,
    P_READONLY   = 0 << 1,
    P_USER       = 1 << 2,
    P_SUPERVISOR = 0 << 2,
    P_WR_THROUGH = 1 << 3,
    P_D_CACHE    = 1 << 4,
    P_ACCESSED   = 1 << 5,
    P_SIZE_4MB   = 1 << 6
} PAGING_FLAGS;

typedef uint32_t pageframe_t;

static inline void *vmm_get_kheap_pdir(void)
{
    return (void*)((uint32_t*)0xfffff000)[KSTART_HEAP];
}

static inline void *vmm_get_kernel_pdir(void)
{
    return (void*)((uint32_t*)0xfffff000)[KSTART];
}

static inline void vmm_flush_TLB(void)
{
    asm volatile ("mov %cr3, %ecx \n \
                   mov %ecx, %cr3");
}

static inline void vmm_change_pdir(void *cr3) 
{
    asm volatile ("mov %0, %%eax \n\
                   mov %%eax, %%cr3" :: "r" ((uint32_t)cr3) 
                                      : "eax", "memory");
}

/* initialization */
void vmm_init(void);
void *vmm_mkpdir(void *virtaddr, uint32_t flags);
void vmm_map_page(void *physaddr, void *virtaddr, uint32_t flags);
void vmm_pf_handler(uint32_t error);
void *vmm_v_to_p(void *virtaddr);
void *vmm_copy_pdir(void *physaddr);

pageframe_t vmm_kalloc_frame(void);
void        vmm_kfree_frame(pageframe_t frame);

void vmm_list_pde(void);
void vmm_list_pte(uint32_t pdi);

#endif /* end of include guard: __VMM_H__ */

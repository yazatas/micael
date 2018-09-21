#ifndef __VMM_H__
#define __VMM_H__

#include <stdint.h>
#include <stddef.h>
#include <fs/multiboot.h>

#define KSTART              768
#define KSTART_HEAP         832
#define PAGE_SIZE           0x1000

#define P_SET_FLAG(value, flag)   (value |= flag)
#define P_UNSET_FLAG(value, flag) (value &= ~flag)
#define P_TEST_FLAG(value, flag)  (value & flag)

/* TODO: add more consistency ie. rewrite */
enum {
    P_PRESENT    = 1,
    P_READWRITE  = 1 << 1,
    P_READONLY   = 0 << 1,
    P_USER       = 1 << 2,
    P_WR_THROUGH = 1 << 3,
    P_D_CACHE    = 1 << 4,
    P_ACCESSED   = 1 << 5,
    P_SIZE_4MB   = 1 << 6,
    P_COW        = 1 << 9,
} PAGING_FLAGS;

typedef uint32_t page_t;
typedef uint32_t ptbl_t;
typedef uint32_t pdir_t;

/* TODO: remove */
static inline void *vmm_get_temporary(void)
{
    return (void*)((uint32_t*)0xfffff000)[960];
}

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

/* TODO: clearn this API
 * 
 * 1) give better names for functions
 * 2) remove too similar functions */

/* miscellaneous */
void  vmm_init(multiboot_info_t *mbinfo);
void  vmm_map_page(void *physaddr, void *virtaddr, uint32_t flags);
void  vmm_pf_handler(uint32_t error);
void  vmm_claim_page(size_t page_idx);
void *vmm_v_to_p(void *virtaddr);

/* allocation */
page_t vmm_kalloc_frame(void);
void   vmm_kfree_frame(page_t frame);
void   vmm_free_tmp_vpage(void *vpage);
void  *vmm_kalloc_mapped_page(uint32_t flags);
void  *vmm_kalloc_tmp_vpage(void);
void  *vmm_duplicate_pdir(void *pdir);
void  *__vmm_map_page(void *physaddr, void *virtaddr);
void *vmm_mmap(void *addr, size_t len);
void vmm_munmap(void *addr);

/* debugging */
void   vmm_list_pde(void);
void   vmm_list_pte(uint32_t pdi);
void   vmm_print_memory_map(void);
size_t vmm_count_free_pages(void);

#endif /* end of include guard: __VMM_H__ */

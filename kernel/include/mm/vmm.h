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

/* miscellaneous */
void  vmm_init(multiboot_info_t *mbinfo);
void  vmm_map_page(void *physaddr, void *virtaddr, uint32_t flags);
void  vmm_pf_handler(uint32_t error);
void  vmm_claim_page(size_t page_idx);
void *vmm_v_to_p(void *virtaddr);

/* allocate one physical page of memory return physical 
 * page address on success and UINT_MAX on error */
page_t vmm_alloc_page(void);

/* allocate virtual range (granularity is 0x1000 bytes)
 * this should be called with care, kernel has only so much
 * free virtual address space so it's easy to run out of it
 * the requests are very large or caller doesn't free the range 
 *
 * F.ex. if you want to allocate 0x2000 bytes of virtual address space:
 * void *ptr = vmm_alloc_addr(2); // to allocate
 * vmm_free_addr(ptr, 2); // to free
 *
 * caller must be sure that the range is sufficient. Kernel doesn't check 
 * in any way if the read/write is beyond allocated range */
void  *vmm_alloc_addr(size_t range);

/* free physical page */
void   vmm_free_page(page_t page);

/* free address range
 * freed range doesn't have to be the same as allocated range
 * if you decided to split the free in n parts, remeber to free all parts */
void   vmm_free_addr(void *addr, size_t range);

/* duplicate page directory pointed to by pdir memory
 * is not allocated for individual pages but instead 
 * they're made to point to original page if write occurs
 * to duplicated page, new page is allocated and contents
 * of the original page is copied to this newly allocated page */
void *vmm_duplicate_pdir(void *pdir);

/* debugging */
void   vmm_list_pde(void);
void   vmm_list_pte(uint32_t pdi);
void   vmm_print_memory_map(void);

#endif /* end of include guard: __VMM_H__ */

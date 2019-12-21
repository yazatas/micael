#ifndef __X86_64_MMU_H__
#define __X86_64_MMU_H__
#ifdef __x86_64__

#include <mm/types.h>

#define KPSTART 0x0000000000100000
#define KVSTART 0xffffffff80100000
#define KPML4I  511

static inline uint64_t native_get_cr3(void)
{
    uint64_t address;

    asm volatile ("mov %%cr3, %%rax \n"
                  "mov %%rax, %0" : "=r" (address));

    return address;
}

static inline void native_set_cr3(uint64_t address)
{
    asm volatile ("mov %0, %%rax \n"
                  "mov %%rax, %%cr3" :: "r" (address));
}

static inline uint64_t *native_p_to_v(uint64_t paddr)
{
    /* TODO: check is this really a physical address
     * ie. that the the addition doesn't overflow */

    return (uint64_t *)(paddr + (KVSTART - KPSTART));
}

static inline uint64_t native_v_to_p(void *vaddr)
{
    /* TODO: check if this highmen ie not idetity mapped */
    return ((uint64_t)vaddr - (KVSTART - KPSTART));
}

static inline void native_flush_tlb(void)
{
    asm volatile ("mov %cr3, %rcx \n \
                   mov %rcx, %cr3");
}

static inline void native_invld_page(uint64_t address)
{
    (void)address;
    /* TODO:  */
}

typedef struct task task_t;

int mmu_native_init(void);

int mmu_native_map_page(unsigned long paddr, unsigned long vaddr, int flags);
int mmu_native_unmap_page(unsigned long vaddr);

unsigned long mmu_native_v_to_p(void *vaddr);
void *mmu_native_p_to_v(unsigned long paddr);

void *mmu_native_build_dir(void);
void *mmu_native_duplicate_dir(void);
void  mmu_native_destroy_dir(void *dir);

void mmu_native_walk_addr(void *addr);

void mmu_native_switch_ctx(task_t *task);

#endif /* __x86_64__ */
#endif /* __X86_64_MMU_H__ */

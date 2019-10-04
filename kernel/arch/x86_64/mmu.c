#include <kernel/kassert.h>
#include <kernel/kprint.h>
#include <kernel/util.h>
#include <mm/mmu.h>
#include <mm/page.h>
#include <sys/types.h>

static uint64_t __pml4[512]   __attribute__((aligned(PAGE_SIZE)));
static uint64_t __pdpt[512]   __attribute__((aligned(PAGE_SIZE)));
static uint64_t __pd[512 * 2] __attribute__((aligned(PAGE_SIZE)));

static uint64_t __pml4_;

#define KPSTART 0x0000000000100000
#define KVSTART 0xffffffff80100000
#define KPML4I  511

#define PML4_ATOEI(addr) (((addr) >> 39) & 0x1FF)
#define PDPT_ATOEI(addr) (((addr) >> 30) & 0x1FF)
#define PD_ATOEI(addr)   (((addr) >> 21) & 0x1FF)
#define PT_ATOEI(addr)   (((addr) >> 12) & 0x1FF)

#define V_TO_P(addr)     ((uint64_t)addr - KVSTART + KPSTART)

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
    kassert(paddr < (uint64_t)(1 << 31));

    return (uint64_t *)(paddr + (KVSTART - KPSTART));
}

static inline uint64_t native_v_to_p(void *vaddr)
{
    /* TODO: check if this highmen ie not idetity mapped */
    return ((uint64_t)vaddr - (KVSTART - KPSTART));
}

int mmu_native_init(void)
{
    kmemset(__pml4, 0, sizeof(__pml4));
    kmemset(__pdpt, 0, sizeof(__pdpt));
    kmemset(__pd,   0, sizeof(__pd));

    /* map also the lower part of the address space so our boot stack still works
     * It only has to work until we switch to init task */
    __pml4[PML4_ATOEI(KVSTART)] = __pml4[0] = V_TO_P(&__pdpt) | MM_PRESENT | MM_READWRITE;

    /* map the first 2GB of address space */
    for (size_t i = 0; i < 512 * 2; ++i) {
        __pd[i] = (i * (1 << 21)) | MM_PRESENT | MM_READWRITE | MM_2MB;
    }

    for (size_t i = 0; i < 2; ++i) {
        __pdpt[PDPT_ATOEI(KVSTART) + i] = __pdpt[i] = V_TO_P(&__pd[i * 512]) | MM_PRESENT | MM_READWRITE;
    }

    native_set_cr3(V_TO_P(__pml4));

    return 0;
}

static uint64_t __alloc_entry(void)
{
    uint64_t addr = mmu_page_alloc(MM_ZONE_DMA | MM_ZONE_NORMAL);
    kmemset((void *)native_p_to_v(addr), 0, PAGE_SIZE);

    return addr | MM_PRESENT | MM_READWRITE;
}

int mmu_native_map_page(unsigned long paddr, unsigned long vaddr, int flags)
{
    unsigned long pml4i = (vaddr >> 39) & 0x1ff;
    unsigned long pdpti = (vaddr >> 30) & 0x1ff;
    unsigned long pdi   = (vaddr >> 21) & 0x1ff;
    unsigned long pti   = (vaddr >> 12) & 0x1ff;

    uint64_t *pml4 = native_p_to_v(native_get_cr3());

    if (!(pml4[pml4i] & MM_PRESENT))
        pml4[pml4i] = __alloc_entry();

    uint64_t *pdpt = native_p_to_v(pml4[pml4i] & ~0xfff);

    if (!(pdpt[pdpti] & MM_PRESENT))
        pdpt[pdpti] = __alloc_entry();

    uint64_t *pd = native_p_to_v(pdpt[pdpti] & ~0xfff);

    if (!(pd[pdi] & MM_PRESENT))
        pd[pdi] = __alloc_entry();

    uint64_t *pt = native_p_to_v(pd[pdi] & ~0xfff);
    pt[pti]      = paddr | flags | MM_PRESENT;

    asm volatile ("mov %cr3, %rcx \n \
                   mov %rcx, %cr3");

    return 0;
}

int mmu_native_unmap_page(unsigned long vaddr)
{
    unsigned long pml4i = (vaddr >> 39) & 0x1ff;
    unsigned long pdpti = (vaddr >> 30) & 0x1ff;
    unsigned long pdi   = (vaddr >> 21) & 0x1ff;
    unsigned long pti   = (vaddr >> 12) & 0x1ff;

    return 0;
}

unsigned long mmu_native_v_to_p(void *vaddr)
{
    return native_v_to_p(vaddr);
}

void *mmu_native_p_to_v(unsigned long paddr)
{
    return native_p_to_v(paddr);
}

void *mmu_native_build_dir(void)
{
    uint64_t pml4_p;
    uint64_t *pml4_v;

    if ((pml4_p = mmu_page_alloc(MM_ZONE_NORMAL)) == INVALID_ADDRESS)
        return NULL;

    pml4_v = native_p_to_v(pml4_p);

    for (size_t i = 0; i < 511; ++i) {
        pml4_v[i] = ~MM_PRESENT;
    }

    pml4_v[0] = pml4_v[PML4_ATOEI(KVSTART)] = __pml4[PML4_ATOEI(KVSTART)] | MM_USER;

    return pml4_v;
}

void mmu_native_walk_addr(void *vaddr)
{
    kprint("----------------------\nWalk address\n");

    if (!vaddr)
        return;

    unsigned long paddr = (unsigned long)vaddr;
    uint64_t *pml4      = native_p_to_v(native_get_cr3());

    uint16_t pml4i = PML4_ATOEI(paddr);
    uint16_t pdpti = PDPT_ATOEI(paddr);
    uint16_t pdi   = PD_ATOEI(paddr);
    uint16_t pti   = PT_ATOEI(paddr);

    kprint("\tPML4: 0x%x 0x%x\n", pml4, paddr);
    kprint("\tPML4[%u] %spresent\n\n", pml4i, (pml4[pml4i] & MM_PRESENT) ? "" : "not ");

    if ((pml4[pml4i] & MM_PRESENT) == 0) {
        kprint("----------------------\n");
        return;
    }

    uint64_t *pdpt = mmu_p_to_v(pml4[pml4i]);
    kprint("\tPDPT: 0x%x 0x%x\n", pdpt, pml4[pml4i]);
    kprint("\tPDPT[%u] %spresent\n\n", pdpti, (pdpt[pdpti] & MM_PRESENT) ? "" : "not ");

    if ((pdpt[pdpti] & MM_PRESENT) == 0) {
        kprint("----------------------\n");
        return;
    }

    uint64_t *pd = mmu_p_to_v(pdpt[pdpti]);
    kprint("\tPD: 0x%x 0x%x\n", pd, pdpt[pdpti]);
    kprint("\tPD[%u] %spresent\n\n", pdi, (pd[pdi] & MM_PRESENT) ? "" : "not ");

    if ((pd[pdi] & MM_PRESENT) == 0) {
        kprint("----------------------\n");
        return;
    }

    uint64_t *pt = mmu_p_to_v(pd[pdi]);
    kprint("\tPT: 0x%x 0x%x\n", pt, pd[pdi]);
    kprint("\tPD[%u] %spresent\n\n", pti, (pt[pti] & MM_PRESENT) ? "" : "not ");

    kprint("----------------------\n");
}

void *mmu_native_duplicate_dir(void)
{
#if 0
    kdebug("starting address space duplication..");

    uint32_t *pdir   = mmu_build_pagedir();
    uint32_t *d_iter = (uint32_t *)0xfffff000;
    uint32_t *t_iter = NULL;
    uint32_t *tbl_v  = NULL;

    for (size_t i = 0; i < KSTART; ++i) {
        if (MM_TEST_FLAG(d_iter[i], MM_PRESENT)) {
            pdir[i] = mmu_alloc_page() | (d_iter[i] & 0xfff);
            t_iter  = ((uint32_t *)0xffc00000) + (0x400 * i);

            for (size_t k = 0; k < 1024; ++k) {
                if (MM_TEST_FLAG(t_iter[k], MM_PRESENT)) {
                    /* TODO: rewrite this!! */
                    void *new_addr = mmu_alloc_addr(1);
                    if (tbl_v != NULL)
                        mmu_free_addr(tbl_v, 1);
                    tbl_v = new_addr;

                    mmu_map_page((void *)pdir[i], tbl_v, MM_PRESENT | MM_READWRITE);

                    t_iter[k] &= ~MM_READWRITE; /* rw must be cleared explicitly */
                    t_iter[k] |= (MM_COW | MM_READONLY);
                    tbl_v[k]   = t_iter[k];
                }
            }
        }
    }
    return pdir;

    uint64_t *pml4_cv = mmu_native_build_dir();          /* copy,     virtual */
    uint64_t *pml4_ov = native_p_to_v(native_get_cr3()); /* original, virtual */
    uint64_t *pdpt_ov = NULL;
    uint64_t *pd_ov   = NULL;
    uint64_t *pt_ov   = NULL;

    for (size_t pml4i = 0; pml4i < KPML4I; ++pml4i) {
        if (pml4_ov[pml4i] & MM_PRESENT) {


            for (size_t pdpti = 0; pdpti < 512; ++pdpti)  {


            }
        }
    }

    return NULL;
#endif

    return NULL;
}

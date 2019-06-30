#include <kernel/util.h>
#include <mm/mmu.h>
#include <mm/page.h>
#include <sys/types.h>

static uint64_t __pml4[512]  __attribute__((aligned(0x1000)));
static uint64_t __pdpt[512]  __attribute__((aligned(0x1000)));
static uint64_t __pd[512]    __attribute__((aligned(0x1000)));

static uint64_t __pml4_;

#define KPSTART 0x0000000000100000
#define KVSTART 0xffffffff80100000

#define PML4_ATOEI(addr) (((addr) >> 39) & 0x1FF)
#define PDPT_ATOEI(addr) (((addr) >> 30) & 0x1FF)
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
    __pdpt[PDPT_ATOEI(KVSTART)] = __pdpt[0] = V_TO_P(&__pd)   | MM_PRESENT | MM_READWRITE;

    /* map the first 1GB of address space */
    for (size_t i = 0; i < 512; ++i) {
        __pd[i] = (i * (1 << 21)) | MM_PRESENT | MM_READWRITE | MM_2MB;
    }

    native_set_cr3(V_TO_P(__pml4));

    return 0;
}

int mmu_native_map_page(unsigned long paddr, unsigned long vaddr, int flags)
{
    unsigned long pml4i = (vaddr >> 39) & 0x1ff;
    unsigned long pdpti = (vaddr >> 30) & 0x1ff;
    unsigned long pdi   = (vaddr >> 21) & 0x1ff;
    unsigned long pti   = (vaddr >> 12) & 0x1ff;

    uint64_t *pml4 = native_p_to_v(native_get_cr3());

    if (!(pml4[pml4i] & MM_PRESENT)) {
        pml4[pml4i]  = mmu_page_alloc(MM_ZONE_DMA | MM_ZONE_NORMAL);
        pml4[pml4i] |= MM_PRESENT | MM_READWRITE;
    }

    uint64_t *pdpt = native_p_to_v(pml4[pml4i]);

    if (!(pdpt[pdpti] & MM_PRESENT)) {
        pdpt[pdpti]  = mmu_page_alloc(MM_ZONE_DMA | MM_ZONE_NORMAL);
        pdpt[pdpti] |= MM_PRESENT | MM_READWRITE;
    }

    uint64_t *pd = native_p_to_v(pdpt[pdpti]);

    if (!(pd[pdi] & MM_PRESENT)) {
        pd[pdi]  = mmu_page_alloc(MM_ZONE_DMA | MM_ZONE_NORMAL);
        pd[pdi] |= MM_PRESENT | MM_READWRITE;
    }

    uint64_t *pt = native_p_to_v(pd[pdi]);

    pt[pti] = paddr | flags;

    return 0;
}

int mmu_native_unmap_page(unsigned long vaddr)
{
    unsigned long pml4i = (vaddr >> 39) & 0x1ff;
    unsigned long pdpti = (vaddr >> 30) & 0x1ff;
    unsigned long pdi   = (vaddr >> 21) & 0x1ff;
    unsigned long pti   = (vaddr >> 12) & 0x1ff;
}

unsigned long mmu_native_v_to_p(void *vaddr)
{
    return native_v_to_p(vaddr);
}

void *mmu_native_p_to_v(unsigned long paddr)
{
    return native_p_to_v(paddr);
}

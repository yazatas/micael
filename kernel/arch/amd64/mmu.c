#include <arch/amd64/mm/mmu.h>
#include <drivers/bus/pci.h>
#include <drivers/gfx/vbe.h>
#include <drivers/ioapic.h>
#include <drivers/lapic.h>
#include <kernel/common.h>
#include <kernel/kassert.h>
#include <kernel/kprint.h>
#include <kernel/util.h>
#include <mm/mmu.h>
#include <mm/page.h>
#include <sched/task.h>
#include <sys/types.h>

static uint64_t __pml4[512]   __attribute__((aligned(PAGE_SIZE)));
static uint64_t __pdpt[512]   __attribute__((aligned(PAGE_SIZE)));
static uint64_t __pd[512 * 2] __attribute__((aligned(PAGE_SIZE)));

static uint64_t __pml4_;
static spinlock_t lock = 0;

#define PML4_ATOEI(addr) (((addr) >> 39) & 0x1FF)
#define PDPT_ATOEI(addr) (((addr) >> 30) & 0x1FF)
#define PD_ATOEI(addr)   (((addr) >> 21) & 0x1FF)
#define PT_ATOEI(addr)   (((addr) >> 12) & 0x1FF)

#define V_TO_P(addr)     ((uint64_t)addr - KVSTART + KPSTART)

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

    amd64_set_cr3(V_TO_P(__pml4));

    return 0;
}

static uint64_t __alloc_entry(void)
{
    uint64_t addr = mmu_page_alloc(MM_ZONE_DMA | MM_ZONE_NORMAL);
    kmemset((void *)amd64_p_to_v(addr), 0, PAGE_SIZE);

    return addr | MM_PRESENT | MM_READWRITE;
}

/* TODO: should this do some error checking */
static void __map_page(uint64_t *pml4, uint64_t paddr, uint64_t vaddr, int flags)
{
    spin_acquire(&lock);

    kassert(PAGE_ALIGNED(paddr));
    kassert(PAGE_ALIGNED(vaddr));

    unsigned long pml4i = (vaddr >> 39) & 0x1ff;
    unsigned long pdpti = (vaddr >> 30) & 0x1ff;
    unsigned long pdi   = (vaddr >> 21) & 0x1ff;
    unsigned long pti   = (vaddr >> 12) & 0x1ff;

    if (!(pml4[pml4i] & MM_PRESENT))
        pml4[pml4i] = __alloc_entry();
    pml4[pml4i] |= flags;

    uint64_t *pdpt = amd64_p_to_v(pml4[pml4i] & ~0xfff);

    if (!(pdpt[pdpti] & MM_PRESENT))
        pdpt[pdpti] = __alloc_entry();
    pdpt[pdpti] |= flags;

    uint64_t *pd = amd64_p_to_v(pdpt[pdpti] & ~0xfff);

    if (!(pd[pdi] & MM_PRESENT))
        pd[pdi] = __alloc_entry();
    pd[pdi] |= flags;

    uint64_t *pt = amd64_p_to_v(pd[pdi] & ~0xfff);
    pt[pti]      = paddr | flags | MM_PRESENT;
    spin_release(&lock);
}

static void __map_range(uint64_t *pml4, unsigned long pstart, unsigned long vstart, size_t n, int flags)
{
    for (size_t i = 0; i < n; ++i) {
        __map_page(pml4, pstart, vstart, flags);

        pstart += PAGE_SIZE;
        vstart += PAGE_SIZE;
    }
}

int mmu_native_map_page(unsigned long paddr, unsigned long vaddr, int flags)
{
    uint64_t *pml4 = amd64_p_to_v(amd64_get_cr3());

    __map_page(pml4, paddr, vaddr, flags);

    /* TODO: use amd64_invld_page() instead! */
    amd64_flush_tlb();

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
    return amd64_v_to_p(vaddr);
}

void *mmu_native_p_to_v(unsigned long paddr)
{
    return amd64_p_to_v(paddr);
}

void *mmu_native_build_dir(void)
{
    uint64_t pml4_p;
    uint64_t *pml4_v;

    if ((pml4_p = mmu_page_alloc(MM_ZONE_NORMAL)) == INVALID_ADDRESS)
        return NULL;

    pml4_v = amd64_p_to_v(pml4_p);

    for (size_t i = 0; i < 511; ++i) {
        pml4_v[i] = 0;
    }

    pml4_v[PML4_ATOEI(KVSTART)] = __pml4[PML4_ATOEI(KVSTART)];

    /* TODO: create some kind of iomem device mapper, this is disgusting! */
    unsigned long lapic  = lapic_get_base();
    unsigned long ioapic = ioapic_get_base();

    kassert(lapic  != INVALID_ADDRESS);
    kassert(ioapic != INVALID_ADDRESS);

    __map_page(pml4_v, lapic,  lapic,  MM_PRESENT | MM_READWRITE);
    __map_page(pml4_v, ioapic, ioapic, MM_PRESENT | MM_READWRITE);

    pci_dev_t *dev = pci_get_dev(VBE_VENDOR_ID, VBE_DEVICE_ID);

    if (dev) {
        void *vga_mem = (uint8_t *)((unsigned long)dev->bar0 - 8);

        __map_range(
            pml4_v,
            (unsigned long)vga_mem,
            (unsigned long)vga_mem,
            4096,
            MM_PRESENT | MM_READWRITE
        );
    }

    return pml4_v;
}

void mmu_native_walk_addr(void *vaddr)
{
    kprint("----------------------\nWalk address\n");

    if (!vaddr)
        return;

    unsigned long paddr = (unsigned long)vaddr;
    uint64_t *pml4      = amd64_p_to_v(amd64_get_cr3());

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

    uint64_t *pdpt = mmu_p_to_v(pml4[pml4i] & ~(PAGE_SIZE - 1));
    kprint("\tPDPT: 0x%x 0x%x\n", pdpt, pml4[pml4i]);
    kprint("\tPDPT[%u] %spresent\n\n", pdpti, (pdpt[pdpti] & MM_PRESENT) ? "" : "not ");

    if ((pdpt[pdpti] & MM_PRESENT) == 0) {
        kprint("----------------------\n");
        return;
    }

    uint64_t *pd = mmu_p_to_v(pdpt[pdpti] & ~(PAGE_SIZE - 1));
    kprint("\tPD: 0x%x 0x%x\n", pd, pdpt[pdpti]);
    kprint("\tPD[%u] %spresent\n\n", pdi, (pd[pdi] & MM_PRESENT) ? "" : "not ");

    if ((pd[pdi] & MM_PRESENT) == 0) {
        kprint("----------------------\n");
        return;
    }

    uint64_t *pt = mmu_p_to_v(pd[pdi] & ~(PAGE_SIZE - 1));
    kprint("\tPT: 0x%x 0x%x\n", pt, pd[pdi]);
    kprint("\tPD[%u] %spresent\n\n", pti, (pt[pti] & MM_PRESENT) ? "" : "not ");

    kprint("----------------------\n");
}

void *mmu_native_duplicate_dir(void)
{
    uint64_t *pml4_cv = mmu_native_build_dir();          /* copy,     virtual */
    uint64_t *pml4_ov = amd64_p_to_v(amd64_get_cr3()); /* original, virtual */
    uint64_t *pdpt_ov = NULL, *pdpt_cv = NULL;
    uint64_t *pd_ov   = NULL, *pd_cv   = NULL;
    uint64_t *pt_ov   = NULL, *pt_cv   = NULL;

    /* TODO: this code looks pretty ugly, use recursion maybe? */

    /* level 3 */
    for (size_t pml4i = 0; pml4i < 511; ++pml4i) {
        if (pml4_ov[pml4i] & MM_PRESENT) {

            /* create new PML4 entry and set up pdpt pointers */
            pml4_cv[pml4i] = __alloc_entry() | MM_PRESENT | MM_USER;
            pdpt_ov        = amd64_p_to_v(pml4_ov[pml4i] & ~(PAGE_SIZE - 1));
            pdpt_cv        = amd64_p_to_v(pml4_cv[pml4i] & ~(PAGE_SIZE - 1));

            /* level 2 */
            for (size_t pdpti = 0; pdpti < 512; ++pdpti)  {
                if (pdpt_ov[pdpti] & MM_PRESENT) {

                    /* create new pdpt entry and set up pd pointers */
                    pdpt_cv[pdpti] = __alloc_entry() | MM_PRESENT | MM_USER;
                    pd_ov          = amd64_p_to_v(pdpt_ov[pdpti] & ~(PAGE_SIZE - 1));
                    pd_cv          = amd64_p_to_v(pdpt_cv[pdpti] & ~(PAGE_SIZE - 1));

                    /* level 1 */
                    for (size_t pdi = 0; pdi < 512; ++pdi)  {
                        if (pd_ov[pdi] & MM_PRESENT) {

                            /* 2MB pages should not be mapped like 4KB pages */
                            if ((pd_ov[pdi] & MM_2MB)) {
                                pd_ov[pdi] &= ~(PAGE_SIZE - 1); /* reset flags */
                                pd_ov[pdi] |= (MM_COW | MM_READONLY | MM_PRESENT | MM_USER | MM_2MB);
                                pd_cv[pdi]  = pd_ov[pdi];
                            }

                            /* create new pd entry and set up pt pointers */
                            pd_cv[pdi] = __alloc_entry() | MM_PRESENT | MM_USER;
                            pt_ov      = amd64_p_to_v(pd_ov[pdi] & ~(PAGE_SIZE - 1));
                            pt_cv      = amd64_p_to_v(pd_cv[pdi] & ~(PAGE_SIZE - 1));

                            for (size_t pti = 0; pti < 512; ++pti) {
                                if (pt_ov[pti] & MM_PRESENT) {
                                    pt_ov[pti] &= ~(PAGE_SIZE - 1); /* reset flags */
                                    pt_ov[pti] |= (MM_COW | MM_READONLY | MM_PRESENT | MM_USER);
                                    pt_cv[pti]  = pt_ov[pti];
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    /* TODO: use amd64_invld_page() instead! */
    amd64_flush_tlb();

    return pml4_cv;
}

void mmu_native_destroy_dir(void *dir)
{
    if (!dir)
        return;

    uint64_t *pml4 = dir;
    uint64_t *pdpt = NULL;
    uint64_t *pd   = NULL;
    uint64_t *pt   = NULL;
    uint64_t page  = 0;
    unsigned flags = 0;

    /* level 3 */
    for (size_t pml4i = 0; pml4i < 511; ++pml4i) {
        if (!(pml4[pml4i] & MM_PRESENT))
            continue;

        pdpt = amd64_p_to_v(pml4[pml4i] & ~(PAGE_SIZE - 1));

        /* level 2 */
        for (size_t pdpti = 0; pdpti < 512; ++pdpti)  {
            if (!(pdpt[pdpti] & MM_PRESENT))
                continue;

            pd = amd64_p_to_v(pdpt[pdpti] & ~(PAGE_SIZE - 1));

            /* level 1 */
            for (size_t pdi = 0; pdi < 512; ++pdi)  {
                if (!(pd[pdi] & MM_PRESENT))
                    continue;

                flags = MM_PRESENT | MM_USER;
                pt    = amd64_p_to_v(pd[pdi] & ~(PAGE_SIZE - 1));

                for (size_t pti = 0; pti < 512; ++pti) {
                    if ((pt[pti] & flags) == flags && !(pt[pti] & MM_COW)) {
                        page = pt[pti] & ~(PAGE_SIZE - 1);
                        /* kdebug("free 4kb page 0x%x", page); */
                        kmemset(amd64_p_to_v(page), 0, PAGE_SIZE);
                        mmu_page_free(page);
                    }
                }

                page = pd[pdi] & ~(PAGE_SIZE - 1);
                kmemset(amd64_p_to_v(page), 0, PAGE_SIZE);
                mmu_page_free(page);
            }

            page = pdpt[pdpti] & ~(PAGE_SIZE - 1);
            kmemset(amd64_p_to_v(page), 0, PAGE_SIZE);
            mmu_page_free(page);
        }

        page = pml4[pml4i] & ~(PAGE_SIZE - 1);
        kmemset(amd64_p_to_v(page), 0, PAGE_SIZE);
        mmu_page_free(page);
    }

    mmu_page_free(amd64_v_to_p(dir));
}

void mmu_native_switch_ctx(task_t *task)
{
    if (!task)
        return;

    amd64_set_cr3(task->cr3);
}

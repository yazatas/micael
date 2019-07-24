#include <kernel/common.h>
#include <kernel/kpanic.h>
#include <kernel/kprint.h>
#include <mm/mmu.h>

static unsigned long __do_cow(unsigned long fault_addr)
{
    (void)fault_addr;
    /* unsigned long p_fault = ROUND_DOWN(fault_addr, PAGE_SIZE); */
    /* page_t p_copy  = mmu_alloc_page(); */
    /* page_t *v_org  = mmu_alloc_addr(1), */
    /*        *v_copy = mmu_alloc_addr(1); */

    /* copy contents from fault page to new page, free temporary addresses and return new page */
    /* mmu_map_page((void *)p_fault, v_org,  MM_PRESENT | MM_READWRITE); */
    /* mmu_map_page((void *)p_copy,  v_copy, MM_PRESENT | MM_READWRITE); */

    /* kmemcpy(v_copy, v_org, PAGE_SIZE); */

    /* mmu_free_addr(v_org,  1); */
    /* mmu_free_addr(v_copy, 1); */

    /* return p_copy | MM_PRESENT | MM_READWRITE | MM_USER; */
    return INVALID_ADDRESS;
}

static void __walk_dir(uint64_t cr3, uint16_t pml4i, uint16_t pdpti, uint16_t pdi, uint16_t pti)
{
    kdebug("");

    uint64_t *pml4 = mmu_p_to_v(cr3);
    kprint("\tPML4: 0x%x 0x%x\n", pml4, cr3);
    kprint("\tPML4[%u] %spresent\n\n", pml4i, (pml4[pml4i] & MM_PRESENT) ? "" : "not ");

    uint64_t *pdpt = mmu_p_to_v(pml4[pml4i]);
    kprint("\tPDPT: 0x%x 0x%x\n", pdpt, pml4[pml4i]);
    kprint("\tPDPT[%u] %spresent\n\n", pdpti, (pdpt[pdpti] & MM_PRESENT) ? "" : "not ");

    uint64_t *pd = mmu_p_to_v(pdpt[pdpti]);
    kprint("\tPD: 0x%x 0x%x\n", pd, pdpt[pdpti]);
    kprint("\tPD[%u] %spresent\n\n", pdi, (pd[pdi] & MM_PRESENT) ? "" : "not ");

    uint64_t *pt = mmu_p_to_v(pd[pdi]);
    kprint("\tPT: 0x%x 0x%x\n", pt, pd[pdi]);
    kprint("\tPD[%u] %spresent\n\n", pti, (pt[pti] & MM_PRESENT) ? "" : "not ");
}

void mmu_pf_handler(isr_regs_t *cpu_state)
{
    uint64_t cr2, cr3;
    uint32_t error = cpu_state->err_num;

    asm volatile ("mov %%cr3, %0" : "=r"(cr3));
    asm volatile ("mov %%cr2, %0" : "=r"(cr2));

    unsigned long pml4i = (cr2 >> 39) & 0x1ff;
    unsigned long pdpti = (cr2 >> 30) & 0x1ff;
    unsigned long pdi   = (cr2 >> 21) & 0x1ff;
    unsigned long pti   = (cr2 >> 12) & 0x1ff;

    const char *strerr = "";
    switch (error) {
        case 0: strerr = "page read, not present (supervisor)";       break;
        case 1: strerr = "page read, protection fault (supervisor)";  break;
        case 2: strerr = "page write, not present (supervisor)";      break;
        case 3: strerr = "page write, protection fault (supervisor)"; break;
        case 4: strerr = "page read, not present (user)";             break;
        case 5: strerr = "page read, protection fault (user)";        break;
        case 6: strerr = "page write, not present (user)";            break;
        case 7: strerr = "page write, protection fault (user)";       break;
        default: kpanic("undocumented error code");
    }

    kprint("\n\n%s\n", strerr);
    kprint("\tfaulting address: 0x%08x\n", cr2);
    kprint("\tPML4 index: 0x%03x %3u\n"
           "\tPDPT index: 0x%03x %3u\n"
           "\tPD index:   0x%03x %3u\n"
           "\tPTindex:    0x%03x %3u\n"
           "\toffset:     0x%03x %3u\n",
           pml4i, pml4i, pdpti, pdpti, pdi, pdi, pti, pti, cr2 & 0xfff, cr2 & 0xfff);

    kprint("physaddr of pdir: 0x%x\n", cr3);

    /* Walk the directory and the virtual address of each directory
     * and whether the entry is present or not */
    __walk_dir(cr3, pml4i, pdpti, pdi, pti);

    for (;;);
}

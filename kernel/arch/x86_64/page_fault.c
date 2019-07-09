#include <kernel/common.h>
#include <kernel/kpanic.h>
#include <kernel/kprint.h>
#include <mm/mmu.h>

static unsigned long mmu_do_cow(unsigned long fault_addr)
{
    unsigned long p_fault = ROUND_DOWN(fault_addr, PAGE_SIZE);
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
}

void mmu_pf_handler(uint32_t error)
{
    uint64_t cr2, cr3;

    asm volatile ("mov %%cr2, %0 \n\
                   mov %%cr3, %0" : "=r" (cr2), "=r" (cr3));

    unsigned long pml4i = (cr2 >> 39) & 0x1ff;
    unsigned long pdpti = (cr2 >> 30) & 0x1ff;
    unsigned long pdi   = (cr2 >> 21) & 0x1ff;
    unsigned long pti   = (cr2 >> 12) & 0x1ff;

    /* uint32_t off = cr2 & 0xfff; */
    /* uint32_t *pd = (uint32_t *)0xfffff000; */
    /* uint32_t *pt = ((uint32_t *)0xffc00000) + (0x400 * pdi); */
    /* if (MM_TEST_FLAG(pt[pti], MM_READWRITE) == 0 && */
    /*     MM_TEST_FLAG(pt[pti], MM_COW)       != 0) */
    /* { */
    /*     pt[pti] = mmu_do_cow(pt[pti]); */
    /*     return; */
    /* } */
    kdebug("%u", error);

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

    /* kprint("\n\n%s\n", strerr); */
    /* kprint("\tfaulting address: 0x%08x\n", cr2); */
    /* kprint("\tpage directory index: %4u 0x%03x\n" */
    /*        "\tpage table index:     %4u 0x%03x\n" */
    /*        "\tpage frame off:       %4u 0x%03x\n", */
    /*        pdi, pdi, pti, pti, off, off); */

    /* kdebug("faulting physaddr: 0x%x", pt[pti]); */
    /* kdebug("physaddr of pdir:  0x%x (0x%x)", cr3, mmu_v_to_p((uint32_t *)0xfffff000)); */

    /* check flags */
    static struct {
        uint32_t flag;
        const char *str;
    } flags[5] = {
        { MM_PRESENT,   "Present" }, { MM_USER,     "User" },
        { MM_READWRITE, "R/W"     }, { MM_READONLY, "Read-Only" },
        { MM_COW,       "CoW"     }
    };

    for (;;);
}

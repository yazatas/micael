#include <kernel/kprint.h>
#include <kernel/kpanic.h>
#include <kernel/compiler.h>
#include <kernel/common.h>
#include <mm/vmm.h>
#include <mm/kheap.h>
#include <fs/multiboot.h>
#include <lib/bitmap.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#define PAGE_FREE                0
#define PAGE_USED                1
#define ENTRY_NOT_PRESENT        0xffff
#define MEM_ADDRESS_SPACE_MAX    0xffffffff
#define MEM_TMP_VPAGES_BASE      0xffe1f000
#define MEM_MAX_TMP_VPAGE_COUNT  (32 * 15) // 480 temporary pages
#define MEM_MAX_PAGE_COUNT       (MEM_ADDRESS_SPACE_MAX / PAGE_SIZE)
#define PHYSADDR_TO_INDEX(addr)  (addr / PAGE_SIZE)
#define INDEX_TO_PHYSADDR(index) (index * PAGE_SIZE)

extern uint32_t boot_page_dir; /* this address is virtual */

static uint32_t PAGE_DIR[MEM_MAX_PAGE_COUNT        / 32] = {PAGE_FREE};
static uint32_t VTMP_PAGES[MEM_MAX_TMP_VPAGE_COUNT / 32] = {PAGE_FREE};

/* bitmap of temporary virtual pages */
static bitmap_t tmp_vpages = {
    MEM_MAX_TMP_VPAGE_COUNT,
    VTMP_PAGES
};

/* bitmap of physical pages */
static bitmap_t phys_pages = {
    MEM_MAX_PAGE_COUNT,
    PAGE_DIR
};

/* kernel page tables */
static uint32_t kpdir[1024]   __align_4k;
static uint32_t kpgtab[1024]  __align_4k;
static uint32_t kheaptb[1024] __align_4k;

void *vmm_kalloc_tmp_vpage(void)
{
    static size_t vpage_ptr   = 0;
    int bit = bm_find_first_unset(&tmp_vpages, vpage_ptr, tmp_vpages.len - 1);

    if (bit == BM_NOT_FOUND_ERROR) {
        bit = bm_find_first_unset(&tmp_vpages, 0, vpage_ptr);

        if (bit == BM_NOT_FOUND_ERROR)
            goto error;
    }

    bm_set_bit(&tmp_vpages, bit);
    vpage_ptr = bit;

    kdebug("allocated tmp vpage 0x%x", MEM_TMP_VPAGES_BASE + bit * PAGE_SIZE);
    return (void *)(MEM_TMP_VPAGES_BASE + bit * PAGE_SIZE);

error:
    kpanic("out of temporary virtual pages!");
    __builtin_unreachable();
}

void vmm_free_tmp_vpage(void *vpage)
{
    kdebug("freeing temporary virtual page at address 0x%x", (uint32_t*)vpage);
    bm_unset_bit(&tmp_vpages, (((uint32_t)vpage) - MEM_TMP_VPAGES_BASE) / PAGE_SIZE);
}

/* try to find free page from range mem_pages_ptr -> phys_pages.len
 * if no free page is found, try to find free page from 
 * 0 -> mem_pages_ptr. If free page is still missing, issue a kernel panic
 *
 * return the address of physical page */
uint32_t vmm_kalloc_frame(void)
{
    static size_t ppage_ptr = 0;
    int bit = bm_find_first_unset(&phys_pages, ppage_ptr, phys_pages.len - 1);

    if (bit == BM_NOT_FOUND_ERROR) {
        bit = bm_find_first_unset(&phys_pages, 0, ppage_ptr);

        if (bit == BM_NOT_FOUND_ERROR)
            goto error;
    }

    bm_set_bit(&phys_pages, bit);
    ppage_ptr = bit;

    return INDEX_TO_PHYSADDR(ppage_ptr);

error:
    kpanic("physical memory exhausted!");
    __builtin_unreachable();
}

/* if the address isn't page-aligned it's round down 
 * and the resulting address pointing to start of the
 * physical page is used to calculate the index 
 * which is in turn used to free the page */
void vmm_kfree_frame(uint32_t physaddr)
{
    uint32_t frame = ROUND_DOWN(physaddr, PAGE_SIZE);
    bm_unset_bit(&phys_pages, PHYSADDR_TO_INDEX(frame));
}

size_t vmm_count_free_pages(void)
{
    size_t free_pages = 0;

    for (uint32_t i = 0; i < phys_pages.len; ++i) {
        if (bm_test_bit(&phys_pages, i) == PAGE_FREE)
            free_pages++;
    }

    return free_pages;
}

void vmm_claim_page(uint32_t physaddr)
{
    uint32_t index = PHYSADDR_TO_INDEX(physaddr);
    if (bm_unset_bit(&phys_pages, index) == BM_OUT_OF_RANGE_ERROR) {
        kpanic("bitmap range error in vmm_claim_page()!");
        __builtin_unreachable();
    }
}

void vmm_pf_handler(uint32_t error)
{
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
        default: kpanic("undocumented error code"); __builtin_unreachable();
    }
    kprint("\n\n");
    kdebug("%s", strerr);

    uint32_t fault_addr = 0;
    asm volatile ("mov %%cr2, %%eax \n \
                   mov %%eax, %0" : "=r" (fault_addr));

    uint32_t pdi    = (fault_addr >> 22) & 0x3ff;
    uint32_t pti    = (fault_addr >> 12) & 0x3ff;
    uint32_t offset = fault_addr & 0xfff;

    kprint("\tfaulting address: 0x%08x\n", fault_addr);
    kprint("\tpage directory index: %4u 0x%03x\n"
           "\tpage table index:     %4u 0x%03x\n"
           "\tpage frame offset:    %4u 0x%03x\n",
           pdi, pdi, pti, pti, offset, offset);

    while (1);

    vmm_map_page((void*)vmm_kalloc_frame(), (void*)fault_addr, P_PRESENT | P_READWRITE);
    vmm_flush_TLB();
}
void vmm_map_page(void *physaddr, void *virtaddr, uint32_t flags)
{
    uint32_t pdi = ((uint32_t)virtaddr) >> 22;
    uint32_t pti = ((uint32_t)virtaddr) >> 12 & 0x3ff; 

    uint32_t *pd = (uint32_t*)0xfffff000;
    if (!(pd[pdi] & P_PRESENT)) {
        kdebug("Page Directory Entry %u is NOT present", pdi);
        pd[pdi] = vmm_kalloc_frame() | flags;
    }

    uint32_t *pt = ((uint32_t*)0xffc00000) + (0x400 * pdi);
    if (!(pt[pti] & P_PRESENT)) {
        kdebug("Page Table Entry %u is NOT present", pti);
        pt[pti] = (uint32_t)physaddr | flags;
    }
}

/* returns physical address of new page directory */
void *vmm_mkpdir(void *virtaddr, uint32_t flags)
{
    uint32_t physaddr = vmm_kalloc_frame();
    vmm_map_page((void*)physaddr, virtaddr, flags);
    memset(virtaddr, 0, 0x1000);

    kdebug("new page directory allocated: 0x%08x", physaddr);

    return (void*)physaddr;
}

void vmm_init(multiboot_info_t *mbinfo)
{
    bm_set_range(&phys_pages, 0, phys_pages.len - 1);
    bm_unset_range(&tmp_vpages, 0, tmp_vpages.len - 1);
    (void)multiboot_map_memory(mbinfo);

    /* enable recursion for temporary page directory 
     * identity mapping enables us this "direct" access to physical memory */
    uint32_t *KPDIR_P = (uint32_t*)((uint32_t)&boot_page_dir - 0xc0000000);
    KPDIR_P[1023]     = ((uint32_t)KPDIR_P) | P_PRESENT | P_READONLY;

    /* initialize kernel page tables */
    for (size_t i = 0; i < 1024; ++i) {
        bm_set_bit(&phys_pages, i);
        kpgtab[i] = (i * PAGE_SIZE) | P_PRESENT | P_READWRITE;
    }

    /* initialize 4MB of initial space for kernel heap */
    for (size_t i = 0; i < 1020; ++i)
        kheaptb[i] = ((uint32_t)vmm_kalloc_frame()) | P_PRESENT | P_READWRITE;

    kpdir[KSTART_HEAP] = ((uint32_t)vmm_v_to_p(&kheaptb)) | P_PRESENT | P_READWRITE;
    kpdir[KSTART]      = ((uint32_t)vmm_v_to_p(&kpgtab))  | P_PRESENT | P_READWRITE;
    kpdir[1023]        = ((uint32_t)vmm_v_to_p(&kpdir))   | P_PRESENT;

    kdebug("changing to pdir located at address: 0x%x...", vmm_v_to_p(&kpdir));
    vmm_change_pdir(vmm_v_to_p(&kpdir));

    /* initialize heap meta data etc. */
    kheap_initialize((uint32_t*)(KSTART_HEAP << 22));
}

/* convert virtual address to physical */
void *vmm_v_to_p(void *virtaddr)
{
    uint32_t pdi = ((uint32_t)virtaddr) >> 22;
    uint32_t pti = ((uint32_t)virtaddr) >> 12 & 0x3ff; 
    uint32_t *pt = ((uint32_t*)0xffc00000) + (0x400 * pdi);

    return (void*)((pt[pti] & ~0xfff) + ((page_t)virtaddr & 0xfff));
}

/* list PDEs (page tables) */
void vmm_list_pde(void)
{
    kdebug("mapped PDEs:");

    uint32_t prev  = ENTRY_NOT_PRESENT,
             first = ENTRY_NOT_PRESENT,
             *v    = (uint32_t*)0xfffff000;

    for (size_t i = 0; i < 1024; ++i) {
        if (v[i] & P_PRESENT) {
            if (first == ENTRY_NOT_PRESENT)
                first = i;
            prev = i;

        /* current entry not present in page directory */
        } else if (prev != ENTRY_NOT_PRESENT) {
            if (first == prev)
                kprint("\tPDE %u present\n", prev);
            else
                kprint("\tPDEs %u - %u present\n", first, prev);
            prev = first = ENTRY_NOT_PRESENT;
        }
    }

    if (prev != ENTRY_NOT_PRESENT) {
        if (first == prev)
            kprint("\tPDE %u present\n", prev);
        else
            kprint("\tPDEs %u - %u present\n", first, prev);
        prev = first = ENTRY_NOT_PRESENT;
    }
}

/* list PTEs (physical pages)
 * print also how many bytes the table maps */
void vmm_list_pte(uint32_t pdi)
{
    if (!(((uint32_t*)0xfffff000)[pdi] & P_PRESENT)) {
        kdebug("Page Directory Entry %u NOT present", pdi);
        return;
    }

    uint32_t nbytes = 0;
    uint32_t *pt = ((uint32_t*)0xffc00000) + (0x400 * pdi);

    for (size_t i = 0; i < 1024; ++i) {
        if (pt[i] & P_PRESENT) {
            nbytes += 0x1000;
        }
    }

    kdebug("Page Table %u:", pdi);
    kprint("\tmapped bytes: %uB %uMB\n",   nbytes, nbytes / 1000);
    kprint("\tmapped table entries: %u\n", nbytes / 0x1000);
}

void vmm_print_memory_map(void)
{
    int bit    = bm_test_bit(&phys_pages, 0);
    int firstb = bit, prevb = bit;
    int first = 0, prev = 0;

    for (size_t i = 1; i < phys_pages.len; ++i) {
        bit = bm_test_bit(&phys_pages, i);

        if (bit != prevb) {
            if (first == prev)
                kdebug("page at address 0x%x is free", INDEX_TO_PHYSADDR(first));
            else if (firstb == BM_BIT_SET)
                kdebug("range 0x%x - 0x%x not free (%u pages)",
                        INDEX_TO_PHYSADDR(first), INDEX_TO_PHYSADDR(prev), prev - first);
            else
                kdebug("range 0x%x - 0x%x free (%u pages)",
                        INDEX_TO_PHYSADDR(first), INDEX_TO_PHYSADDR(prev), prev - first);

            first  = i; firstb = bit;
        }
        prev  = i; prevb = bit;
    }
}

void *vmm_kalloc_mapped_page(uint32_t flags)
{
    uint32_t *TMP_PAGE = vmm_kalloc_tmp_vpage();

    page_t tmp = vmm_kalloc_frame();
    vmm_map_page((void*)tmp, (void*)TMP_PAGE, flags);

    TMP_PAGE += 0x1000;
    return (void*)(TMP_PAGE - 0x1000);
}

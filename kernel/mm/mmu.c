#include <kernel/kprint.h>
#include <kernel/kpanic.h>
#include <kernel/compiler.h>
#include <kernel/common.h>
#include <mm/cache.h>
#include <mm/kheap.h>
#include <mm/mmu.h>
#include <fs/multiboot.h>
#include <lib/bitmap.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#define NUM_FLAGS                5
#define PAGE_FREE                0
#define PAGE_USED                1
#define ENTRY_NOT_PRESENT        0xffff
#define MEM_ADDRESS_SPACE_MAX    0xffffffff
#define MEM_TMP_VPAGES_BASE      0xff800000
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
static size_t   free_page_count = 0;

static page_t vmm_do_cow(page_t fault_addr)
{
#if 0
    page_t new_phys_page = vmm_alloc_page();
    page_t *tmp_page_org = NULL,
           *tmp_page_cpy = NULL;

    vmm_map_page((void *)fault_addr,    tmp_page_org, MM_PRESENT | MM_READWRITE);
    vmm_map_page((void *)new_phys_page, tmp_page_cpy, MM_PRESENT | MM_READWRITE);

    memcpy(tmp_page_cpy, tmp_page_org, 0x1000);

    vmm_free_tmp_vpage(tmp_page_cpy);
    vmm_free_tmp_vpage(tmp_page_org);

    new_phys_page |= MM_PRESENT | MM_USER | MM_READWRITE;

    /* kdebug("new page addr: 0x%x", new_phys_page); */

    return new_phys_page;
#endif
}

void *vmm_alloc_addr(size_t range)
{
    int bit = bm_find_first_unset_range(&tmp_vpages, 0, tmp_vpages.len - 1, range);

    if (bit == BM_NOT_FOUND_ERROR) {
        kpanic("out of temporary virtual addresses");
    }

    bm_set_range(&tmp_vpages, bit, bit + range);

    /* kdebug("allocated address range 0x%x - 0x%x", MEM_TMP_VPAGES_BASE + bit * PAGE_SIZE, */
    /*         MEM_TMP_VPAGES_BASE + (bit + range) * PAGE_SIZE - 1); */
    return (void *)(MEM_TMP_VPAGES_BASE + bit * PAGE_SIZE);
}

void vmm_free_addr(void *addr, size_t range)
{
    const uint32_t start = (((uint32_t)addr) - MEM_TMP_VPAGES_BASE) / PAGE_SIZE;

    bm_unset_range(&tmp_vpages, start, start + range);

    const uint32_t pdi = ((uint32_t)addr) >> 22;
    const uint32_t pti = ((uint32_t)addr) >> 12 & 0x3ff; 
    const uint32_t *pd = (uint32_t *)0xfffff000;
    uint32_t *pt = ((uint32_t *)0xffc00000) + (0x400 * pdi);

    if (pd[pdi] & MM_PRESENT) {
        if (pt[pti] & MM_PRESENT) {
            pt[pti] &= ~MM_PRESENT;
        }
    }
}

void vmm_free_tmp_vpage(void *vpage)
{
    /* kdebug("freeing temporary virtual page at address 0x%x", (uint32_t*)vpage); */
    bm_unset_bit(&tmp_vpages, (((uint32_t)vpage) - MEM_TMP_VPAGES_BASE) / PAGE_SIZE);
    free_page_count++;
}

/* try to find free page from range mem_pages_ptr -> phys_pages.len
 * if no free page is found, try to find free page from 
 * 0 -> mem_pages_ptr. If free page is still missing, issue a kernel panic
 *
 * return the address of physical page */
uint32_t vmm_alloc_page(void)
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
    free_page_count--;

    return INDEX_TO_PHYSADDR(ppage_ptr);

error:
    kpanic("physical memory exhausted!");
    __builtin_unreachable();
}

/* if the address isn't page-aligned it's round down 
 * and the resulting address pointing to start of the
 * physical page is used to calculate the index 
 * which is in turn used to free the page */
void vmm_free_page(uint32_t physaddr)
{
    uint32_t frame = ROUND_DOWN(physaddr, PAGE_SIZE);
    bm_unset_bit(&phys_pages, PHYSADDR_TO_INDEX(frame));
}

void vmm_claim_page(uint32_t physaddr)
{
    uint32_t index = PHYSADDR_TO_INDEX(physaddr);
    if (bm_unset_bit(&phys_pages, index) == BM_OUT_OF_RANGE_ERROR) {
        kpanic("bitmap range error in vmm_claim_page()!");
        __builtin_unreachable();
    }

    free_page_count++;
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

    uint32_t cr3 = 0;
    asm volatile ("mov %%cr3, %%eax \n \
                   mov %%eax, %0" : "=r" (cr3));

    uint32_t pdi    = (fault_addr >> 22) & 0x3ff;
    uint32_t pti    = (fault_addr >> 12) & 0x3ff;
    uint32_t offset = fault_addr & 0xfff;

    kprint("\tfaulting address: 0x%08x\n", fault_addr);
    kprint("\tpage directory index: %4u 0x%03x\n"
           "\tpage table index:     %4u 0x%03x\n"
           "\tpage frame offset:    %4u 0x%03x\n",
           pdi, pdi, pti, pti, offset, offset);

    uint32_t *pd = (uint32_t*)0xfffff000;
    uint32_t *pt = ((uint32_t*)0xffc00000) + (0x400 * pdi);

    kdebug("faulting physaddr: 0x%x", pt[pti]);
    kdebug("physaddr of pdir:  0x%x", cr3);

    /* check flags */
    static struct {
        uint32_t flag;
        const char *str;
    } flags[NUM_FLAGS] = {
        { MM_PRESENT, "present" }, { MM_USER, "user" },
        { MM_READWRITE, "r/w" },   { MM_READONLY, "read-only" },
        { MM_COW, "CoW" }
    };

    for (int i = 0; i < NUM_FLAGS; ++i) {
        if (MM_TEST_FLAG(pt[pti], flags[i].flag)) {
            kdebug("%s flag set", flags[i].str);
        }
    }
    if (MM_TEST_FLAG(pt[pti], MM_READWRITE) == 0 &&
        MM_TEST_FLAG(pt[pti], MM_COW)       != 0)
    {
        kdebug("Copy-on-Write");

        /* pt[pti] = vmm_do_cow(pt[pti]); */
        return;
    }

    for (;;);

    vmm_map_page((void *)vmm_alloc_page(), (void*)fault_addr, MM_PRESENT | MM_READWRITE);
    vmm_flush_TLB();
}

/* map physical address to virtual address
 * this function is capable of mapping only 4kb bytes of memory */
void vmm_map_page(void *physaddr, void *virtaddr, uint32_t flags)
{
    uint32_t pdi = ((uint32_t)virtaddr) >> 22;
    uint32_t pti = ((uint32_t)virtaddr) >> 12 & 0x3ff; 

    uint32_t *pd = (uint32_t*)0xfffff000;
    if (!(pd[pdi] & MM_PRESENT)) {
        /* kdebug("Page Directory Entry %u is NOT present", pdi); */
        pd[pdi] = vmm_alloc_page() | flags;
    } 

    uint32_t *pt = ((uint32_t *)0xffc00000) + (0x400 * pdi);
    if (!(pt[pti] & MM_PRESENT)) {
        /* kdebug("Page Table Entry %u is NOT present", pti); */
        pt[pti] = (uint32_t)physaddr | flags;
    }
}

void vmm_map_range(void *physaddr, void *virtaddr, size_t range, uint32_t flags)
{
    uint32_t paddr = (uint32_t)physaddr;
    uint32_t vaddr = (uint32_t)virtaddr;

    for (size_t i = 0; i < range; ++i) {
        vmm_map_page((void *)paddr, (void *)vaddr, flags);
        paddr += PAGE_SIZE, vaddr += PAGE_SIZE;
    }
}

void vmm_init(multiboot_info_t *mbinfo)
{
    bm_set_range(&phys_pages, 0, phys_pages.len - 1);
    bm_unset_range(&tmp_vpages, 0, tmp_vpages.len - 1);
    (void)multiboot_map_memory(mbinfo);

    /* enable recursion for temporary page directory 
     * identity mapping enables us this "direct" access to physical memory */
    uint32_t *KPDIR_P = (uint32_t*)((uint32_t)&boot_page_dir - 0xc0000000);
    KPDIR_P[1023]     = ((uint32_t)KPDIR_P) | MM_PRESENT | MM_READONLY;

    /* initialize kernel page tables */
    for (size_t i = 0; i < 1024; ++i) {
        bm_set_bit(&phys_pages, i);
        kpgtab[i] = (i * PAGE_SIZE) | MM_PRESENT | MM_READWRITE;
    }

    /* initialize 4MB of initial space for kernel heap */
    for (size_t i = 0; i < 1020; ++i) {
        kheaptb[i] = ((uint32_t)vmm_alloc_page()) | MM_PRESENT | MM_READWRITE;
    }

    kpdir[KSTART_HEAP] = ((uint32_t)vmm_v_to_p(&kheaptb)) | MM_PRESENT | MM_READWRITE;
    kpdir[KSTART]      = ((uint32_t)vmm_v_to_p(&kpgtab))  | MM_PRESENT | MM_READWRITE;
    kpdir[1023]        = ((uint32_t)vmm_v_to_p(&kpdir))   | MM_PRESENT | MM_READWRITE;

    kdebug("changing to pdir located at address: 0x%x...", vmm_v_to_p(&kpdir));
    vmm_change_pdir(vmm_v_to_p(&kpdir));

    /* initialize heap meta data etc. */
    kheap_initialize((uint32_t *)(KSTART_HEAP << 22));

    if (cache_init() < 0)
        kdebug("failed to init page cache");
}

/* convert virtual address to physical */
void *vmm_v_to_p(void *virtaddr)
{
    uint32_t pdi = ((uint32_t)virtaddr) >> 22;
    uint32_t pti = ((uint32_t)virtaddr) >> 12 & 0x3ff; 
    uint32_t *pt = ((uint32_t*)0xffc00000) + (0x400 * pdi);

    return (void *)((pt[pti] & ~0xfff) + ((page_t)virtaddr & 0xfff));
}

/* list PDEs (page tables) */
void vmm_list_pde(void)
{
    kdebug("mapped PDEs:");

    uint32_t prev  = ENTRY_NOT_PRESENT,
             first = ENTRY_NOT_PRESENT,
             *v    = (uint32_t *)0xfffff000;

    for (size_t i = 0; i < 1024; ++i) {
        if (v[i] & MM_PRESENT) {
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
    if (!(((uint32_t*)0xfffff000)[pdi] & MM_PRESENT)) {
        kdebug("Page Directory Entry %u NOT present", pdi);
        return;
    }

    uint32_t nbytes = 0;
    uint32_t *pt = ((uint32_t*)0xffc00000) + (0x400 * pdi);

    for (size_t i = 0; i < 1024; ++i) {
        if (pt[i] & MM_PRESENT) {
            nbytes += 0x1000;
        }
    }

    kprint("Page Table %u:\n", pdi);
    kprint("\tmapped bytes: %uB %uKB\n",   nbytes, nbytes / 1000);
    kprint("\tmapped table entries: %u\n", nbytes / 0x1000);
}

void vmm_print_memory_map(void)
{
    int bit    = bm_test_bit(&phys_pages, 0);
    int firstb = bit, prevb = bit;
    int first = 0, prev = 0;

    kprint("available memory:\n"
           "\tpages:     %u\n"
           "\tkilobytes: %uKB\n"
           "\tmegabytes: %uMB\n"
           "\tgigabytes: %uGB\n",
           free_page_count,
           (free_page_count   * PAGE_SIZE) / 1000,
           ((free_page_count  * PAGE_SIZE) / 1000) / 1000,
           (((free_page_count * PAGE_SIZE) / 1000) / 1000) / 1000);

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

/* temporarily map the page directory to kernel's address space,
 * traverse through it and copy address of each physical page
 * below KSTART to new page directory and mark every page as READ ONLY.
 * Above KSTAR just map the page tables instead if individual pages.
 *
 * Return the virtual address of new page directory
 *
 * @pdir: virtual address of the new page directory */
void *vmm_duplicate_pdir(void *pdir)
{
    kdebug("starting address space duplication..");

    ptbl_t *pt_v_org, *pt_v_copy;
    pdir_t *pdir_v_copy = vmm_alloc_addr(1);
    pdir_t *pdir_v_org  = vmm_alloc_addr(1);
    page_t page         = vmm_alloc_page();

    uint32_t flags = MM_PRESENT | MM_READWRITE;

    vmm_map_page(pdir,         (void *)pdir_v_org,  flags);
    vmm_map_page((void *)page, (void *)pdir_v_copy, flags);

    for (size_t i = 0; i < KSTART; ++i) {
        if (MM_TEST_FLAG(pdir_v_org[i], MM_PRESENT)) {
            pt_v_copy      = NULL;
            pdir_v_copy[i] = ((uint32_t)vmm_v_to_p(pt_v_copy)) | flags; 

            pt_v_org = vmm_alloc_addr(1);
            vmm_map_page((void *)pdir_v_org[i], pt_v_org, flags);

            for (size_t k = 0; k < 1024; ++k) {
                if (MM_TEST_FLAG(pt_v_org[k], MM_PRESENT)) {
                    kdebug("Copy-on-Write");
                    /* pt_v_copy[k] = vmm_do_cow(pt_v_org[k]); */
                }
            }
        }
    }

    /* map the kernel as is, no need to perform any CoW operations */
    for (size_t i = KSTART; i < 1024; ++i) {
        if (pdir_v_org[i] & MM_PRESENT) {
            pdir_v_copy[i] = pdir_v_org[i];
        }
    }

    vmm_flush_TLB();
    return (void *)pdir_v_copy;
}

void *mmu_build_pagedir(void)
{
    /* physical and virtual addresses of the new directory */
    page_t pdir  = vmm_alloc_page();
    pdir_t *vdir = vmm_alloc_addr(1);

    vmm_map_page((void *)pdir, (void *)vdir, MM_PRESENT | MM_READWRITE);

    for (size_t i = 0; i < KSTART; ++i) {
        vdir[i] = ~MM_PRESENT;
    }

    for (size_t i = KSTART; i < 1024; ++i) {
        vdir[i] = kpdir[i];
    }

    return vdir;
}

#if 0
void *mmu_build_pagedir(void)
{
    ptbl_t *pt_v_org, *pt_v_copy;
    pdir_t *pdir_v_copy = vmm_alloc_addr(1);
    pdir_t *pdir_v_org  = vmm_alloc_addr(1);
    page_t page         = vmm_alloc_page();

    uint32_t flags = MM_PRESENT | MM_READWRITE;

    vmm_map_page(pdir,         (void *)pdir_v_org,  flags);
    vmm_map_page((void *)page, (void *)pdir_v_copy, flags);

    for (size_t i = 0; i < KSTART; ++i) {
        if (MM_TEST_FLAG(pdir_v_org[i], MM_PRESENT)) {
            pt_v_copy      = NULL;
            pdir_v_copy[i] = ((uint32_t)vmm_v_to_p(pt_v_copy)) | flags; 

            pt_v_org = vmm_alloc_addr(1);
            vmm_map_page((void *)pdir_v_org[i], pt_v_org, flags);

            for (size_t k = 0; k < 1024; ++k) {
                 /* Add special flag MM_COW which tells us that this is 
                 * really not a read-only page but rather a CoW-mapped page 
                 * which should result in memory copying upon page fault */
                if (MM_TEST_FLAG(pt_v_org[k], MM_PRESENT)) {
                    /* TODO: make sure this works (require user mode support) */
                    /* pt_v_copy[k] = (pt_v_org[k] |= MM_COW | MM_READONLY); */

                }
            }
        }
    }

    /* map the kernel as is, no need to perform any CoW operations */
    for (size_t i = KSTART; i < 1024; ++i) {
        if (pdir_v_org[i] & MM_PRESENT) {
            pdir_v_copy[i] = pdir_v_org[i];
        }
    }

    vmm_flush_TLB();
    return (void *)pdir_v_copy;
}
#endif

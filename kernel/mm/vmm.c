#include <kernel/kprint.h>
#include <kernel/kpanic.h>

#include <mm/vmm.h>
#include <mm/kheap.h>

#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#define PAGE_FREE 0
#define PAGE_USED 1
#define ENTRY_NOT_PRESENT 0xffff

extern uint32_t boot_page_dir; /* this address is virtual */
extern uint32_t __kernel_virtual_end, __kernel_physical_end; 
extern uint32_t __kernel_physical_start;

static uint32_t *KPDIR_P;
static uint32_t *KPDIR_V;
static uint32_t START_FRAME = (uint32_t)&__kernel_physical_end;
static uint32_t FRAME_START = 0xf0000000;

/* TODO: copy bitvector from file system and use it here */
static uint32_t PAGE_DIR[1024] = {PAGE_FREE}; /* FIXME: 1024 is not the correct value */
static uint32_t pg_dir_ptr = 0;

static uint32_t kpdir[1024]  __attribute__((aligned(4096)));
static uint32_t kpgtab[1024] __attribute__((aligned(4096)));

/* notice that this function returns a physical address, not a virtual! */
/* FIXME: this needs a rewrite */
uint32_t vmm_kalloc_frame(void)
{
    uint32_t i, tmp;

    for (i = 0; i < pg_dir_ptr; ++i) {
        if (PAGE_DIR[i] == PAGE_FREE)
            break;
    }

    if (i < pg_dir_ptr) {
        PAGE_DIR[i] = PAGE_USED;
    } else {
        PAGE_DIR[pg_dir_ptr] = PAGE_USED;
        i = pg_dir_ptr++;
    }

    return START_FRAME + (i * PAGE_SIZE);
}

/* address must point to the beginning of the physical address */
void vmm_kfree_frame(uint32_t frame)
{
    uint32_t index = (frame - START_FRAME) / PAGE_SIZE;
    PAGE_DIR[index] = PAGE_FREE;
}

/* TODO: what is and how to handle protection fault (especially supervisor) */
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

    kprint("\tfaulting address: 0x%x\n", fault_addr);
    kprint("\tpage directory index: %4u 0x%03x\n"
           "\tpage table index:     %4u 0x%03x\n"
           "\tpage frame offset:    %4u 0x%03x\n",
           pdi, pdi, pti, pti, offset, offset);

    /* while (1); */

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

void vmm_init(void)
{
    /* 4k align START_FRAME */
    if (START_FRAME % PAGE_SIZE != 0) {
        START_FRAME = (START_FRAME + PAGE_SIZE) & ~(PAGE_SIZE - 1);
    }
    kdebug("START_FRAME aligned to 4k boundary: 0x%x", START_FRAME);

    /* initialize kernel page table */
    KPDIR_P = (uint32_t*)((uint32_t)&boot_page_dir - 0xc0000000);
    KPDIR_V = (uint32_t*)((uint32_t)&boot_page_dir);
    KPDIR_V[1023] = ((uint32_t)KPDIR_P) | P_PRESENT | P_READONLY;

    for (size_t i = 0; i < 1024; ++i)
        kpgtab[i] = (i * PAGE_SIZE) | P_PRESENT | P_READWRITE;

    for (size_t i = KSTART_HEAP; i <= KEND_HEAP; ++i)
        kpdir[i] = vmm_kalloc_frame() | P_PRESENT | P_READWRITE;

    kpdir[KSTART] = ((uint32_t)vmm_v_to_p(&kpgtab)) | P_PRESENT | P_READWRITE;
    kpdir[1023]   = ((uint32_t)vmm_v_to_p(&kpdir)) | P_PRESENT;

    kdebug("changing to pdir located at address: 0x%x...", vmm_v_to_p(&kpdir));
    vmm_change_pdir(vmm_v_to_p(&kpdir));

    /* kheap_initialize((uint32_t*)(KSTART_HEAP << 22)); */
}

/* convert virtual address to physical 
 * return NULL if the address hasn't been mapped */
void *vmm_v_to_p(void *virtaddr)
{
    uint32_t pdi = ((uint32_t)virtaddr) >> 22;
    uint32_t pti = ((uint32_t)virtaddr) >> 12 & 0x3ff; 
    uint32_t *pt = ((uint32_t*)0xffc00000) + (0x400 * pdi);

    return (void*)((pt[pti] & ~0xfff) + ((pageframe_t)virtaddr & 0xfff));
}

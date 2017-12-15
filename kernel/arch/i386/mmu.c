#include <kernel/mmu.h>
#include <kernel/kprint.h>
#include <kernel/kpanic.h>
#include <kernel/kheap.h>

#include <stddef.h>
#include <stdbool.h>

#define PAGE_FREE 0
#define PAGE_USED 1

extern uint32_t boot_page_dir; /* this address is virtual */
extern uint32_t __kernel_virtual_end, __kernel_physical_end; 

static uint32_t *PDIR_PHYS; /* see mmu_init */
static uint32_t START_FRAME = (uint32_t)&__kernel_physical_end;
static uint32_t FRAME_START = 0xf0000000;

/* TODO: copy bitvector from file system and use it here */
static uint32_t PAGE_DIR[1024] = {PAGE_FREE}; /* FIXME: 1024 is not the correct value */
static uint32_t pg_dir_ptr = 0;

/* notice that this function returns a physical address, not a virtual! */
uint32_t kalloc_frame(void)
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
void kfree_frame(uint32_t frame)
{
	uint32_t index = (frame - START_FRAME) / PAGE_SIZE;
	PAGE_DIR[index] = PAGE_FREE;
}

/* TODO: what is and how to handle protection fault (especially supervisor) */
void pf_handler(uint32_t error)
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
		default: strerr = "vou";
	}
	kprint("\n\n%s\n", strerr);

	uint32_t fault_addr = 0;
	asm volatile ("mov %%cr2, %%eax \n \
			       mov %%eax, %0" : "=r" (fault_addr));

	uint32_t pdi    = (fault_addr >> 22) & 0x3ff;
	uint32_t pti    = (fault_addr >> 12) & 0x3ff;
	uint32_t offset = fault_addr & 0xfff;

	kprint("[mmu] faulting address: 0x%x\n", fault_addr);
	kprint("[mmu] page directory index: %4u 0x%03x\n"
		   "page table index:     %4u 0x%03x\n"
		   "page frame offset:    %4u 0x%03x\n",
		   pdi, pdi, pti, pti, offset, offset);

	uint32_t *pd = (uint32_t*)0xffffff000;
	if (!(pd[pdi] & P_PRESENT)) {
		kprint("[mmu] pde %u is NOT present\n", pdi);
		pd[pdi] = kalloc_frame() | P_PRESENT | P_READWRITE;
	}

	uint32_t *pt = ((uint32_t*)0xffc00000) + (0x400 * pdi);
	if (!(pt[pti] & P_PRESENT)) {
		kprint("[mmu] pte %u is NOT present\n", pti);
		pt[pti] = kalloc_frame() | P_PRESENT | P_READWRITE;
	}

	flush_tlb();
}

/* return 0 on success and -1 on error */
void map_page(void *physaddr, void *virtaddr, uint32_t flags)
{
	uint32_t pdi = ((uint32_t)virtaddr) >> 22;
	uint32_t pti = ((uint32_t)virtaddr) >> 12 & 0x3ff; 

	uint32_t *pd = (uint32_t*)0xfffff000;
	if (!(pd[pdi] & P_PRESENT)) {
		pd[pdi] = kalloc_frame() | flags;
	}

	uint32_t *pt = ((uint32_t*)0xffc00000) + (0x400 * pdi);
	if (!(pt[pti] & P_PRESENT)) {
		pt[pti] = (uint32_t)physaddr | flags;
	}
}

void mmu_init(void)
{
	/* 4k align START_FRAME */
	if (START_FRAME % PAGE_SIZE != 0) {
		START_FRAME = (START_FRAME + PAGE_SIZE) & ~(PAGE_SIZE - 1);
	}
	kprint("START_FRAME aligned to 4k boundary: 0x%08x\n", START_FRAME);

	/* initialize recursive page directory */
	PDIR_PHYS = (uint32_t*)((uint32_t)&boot_page_dir - 0xc0000000);
	PDIR_PHYS[1023] = (uint32_t)PDIR_PHYS | P_PRESENT | P_READWRITE;
	PDIR_PHYS[0] &= ~P_PRESENT;
	kprint("[mmu] recursive page directory enabled!\n");

	/* init kernel heap */
	kheap_init();
	flush_tlb();
}

inline void flush_tlb(void)
{
	asm volatile ("mov %cr3, %ecx \n \
			       mov %ecx, %cr3");
}


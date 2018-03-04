#include <kernel/mmu.h>
#include <kernel/kprint.h>
#include <kernel/kpanic.h>

#include <stddef.h>
#include <stdbool.h>
#include <string.h>

typedef struct meta {
	size_t size;
	uint8_t flags;
	struct meta *next;
	struct meta *prev;
} __attribute__((packed)) meta_t;

#define META_SIZE      sizeof(meta_t)
#define IS_FREE(x)     (x->flags & 0x1)
#define MARK_FREE(x)   (x->flags |= 0x1)
#define UNMARK_FREE(x) (x->flags &= ~0x1)
#define GET_BASE(x)    ((meta_t*)x - 1)

#define PAGE_FREE 0
#define PAGE_USED 1

extern uint32_t boot_page_dir; /* this address is virtual */
extern uint32_t __kernel_virtual_end, __kernel_physical_end; 

static uint32_t *PDIR_PHYS; /* see mmu_init */
static uint32_t START_FRAME = (uint32_t)&__kernel_physical_end;
static uint32_t FRAME_START = 0xf0000000;

static meta_t   *kernel_base = NULL;
static uint32_t *HEAP_START  = (uint32_t*)0xd0000000;
static uint32_t *HEAP_BREAK  = (uint32_t*)0xd0000000;

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
	kprint("\n\n");
	kdebug("%s", strerr);

	uint32_t fault_addr = 0;
	asm volatile ("mov %%cr2, %%eax \n \
			       mov %%eax, %0" : "=r" (fault_addr));

	uint32_t pdi    = (fault_addr >> 22) & 0x3ff;
	uint32_t pti    = (fault_addr >> 12) & 0x3ff;
	uint32_t offset = fault_addr & 0xfff;

	kprint("faulting address: 0x%x\n", fault_addr);
	kprint("page directory index: %4u 0x%03x\n"
		   "page table index:     %4u 0x%03x\n"
		   "page frame offset:    %4u 0x%03x\n",
		   pdi, pdi, pti, pti, offset, offset);

	uint32_t *pd = (uint32_t*)0xffffff000;
	if (!(pd[pdi] & P_PRESENT)) {
		kdebug("[mmu] pde %u is NOT present", pdi);
		pd[pdi] = kalloc_frame() | P_PRESENT | P_READWRITE;
	}

	uint32_t *pt = ((uint32_t*)0xffc00000) + (0x400 * pdi);
	if (!(pt[pti] & P_PRESENT)) {
		kdebug("[mmu] pte %u is NOT present", pti);
		pt[pti] = kalloc_frame() | P_PRESENT | P_READWRITE;
	}

	flush_TLB();
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
	kdebug("START_FRAME aligned to 4k boundary: 0x%08x", START_FRAME);

	/* initialize recursive page directory */
	PDIR_PHYS = (uint32_t*)((uint32_t)&boot_page_dir - 0xc0000000);
	PDIR_PHYS[1023] = (uint32_t)PDIR_PHYS | P_PRESENT | P_READWRITE;
	PDIR_PHYS[0] &= ~P_PRESENT;
	kdebug("recursive page directory enabled!");

	/* init kernel heap */
	kheap_init();
	flush_TLB();
}

inline void flush_TLB(void)
{
	asm volatile ("mov %cr3, %ecx \n \
			       mov %ecx, %cr3");
}

/**************************** kernel heap ****************************/

/* TODO: this never returns NULL, map_page status code when??? */
static meta_t *morecore(size_t size)
{
	size_t pgcount = size / 0x1000 + 1;
	meta_t *tmp = (meta_t*)HEAP_BREAK;

	for (size_t i = 0; i < pgcount; ++i) {
		map_page((void*)kalloc_frame(), HEAP_BREAK, P_PRESENT | P_READWRITE);
		HEAP_BREAK = (uint32_t*)((uint8_t*)HEAP_BREAK + 0x1000);
	}
	flush_TLB();

	/* place the new block at the beginning of block list */
	tmp->size = pgcount * 0x1000 - META_SIZE;
	kernel_base->prev = tmp;
	tmp->next = kernel_base;
	tmp->prev = NULL;
	kernel_base = tmp;
	MARK_FREE(tmp);

	return tmp;
}

/* append b2 to b1 (b2 becomes invalid) */
static inline void merge_blocks(meta_t *b1, meta_t *b2)
{
	b1->size += b2->size + META_SIZE;
	b1->next = b2->next;
	if (b2->next)
		b2->next->prev = b1;
}

static void split_block(meta_t *b, size_t split)
{
	if (b->size <= split + META_SIZE)
		return;

	meta_t *tmp = (meta_t*)((uint8_t*)b + split + META_SIZE);
	tmp->size = b->size - split - META_SIZE;
	b->size = split;
	MARK_FREE(tmp);

	tmp->next = b->next;
	if (b->next)
		b->next->prev = tmp;
	tmp->prev = b;
	b->next = tmp;
}

static meta_t *find_free_block(size_t size)
{
	meta_t *b = kernel_base, *tmp = NULL;

	while (b && !(IS_FREE(b) && b->size >= size)) {
		b = b->next;
	}

	if (b != NULL) {
		UNMARK_FREE(b);
		split_block(b, size);
	}
	return b;
}

void *kmalloc(size_t size)
{
	meta_t *b;

	if ((b = find_free_block(size)) == NULL) {
		if ((b = morecore(size + META_SIZE)) == NULL)
			kpanic("kernel heap exhausted"); /* kpanic doesn't return  */

		split_block(b, size);
		UNMARK_FREE(b);
	}

	return b + 1;
}

void *kcalloc(size_t nmemb, size_t size)
{
	void *b = kmalloc(nmemb * size);

	memset(b, 0, nmemb * size);
	return b;
}

void *krealloc(void *ptr, size_t size)
{
	/* check if merging adjacent blocks achieves the goal size */
	meta_t *b = GET_BASE(ptr), *tmp;
	size_t new_size = b->size;

	if (b->prev && IS_FREE(b->prev)) new_size += b->prev->size + META_SIZE;
	if (b->next && IS_FREE(b->next)) new_size += b->next->size + META_SIZE;

	if (new_size >= size) {
		if (b->prev != NULL && IS_FREE(b->prev)) {
			merge_blocks(b->prev, b);
			memmove(b->prev + 1, b + 1, b->size);
			b = b->prev;
		}

		if (b->next != NULL && IS_FREE(b->next)) {
			merge_blocks(b, b->next);
		}
		split_block(b, size);

	} else {
		tmp = find_free_block(size);
		if (!tmp) {
			tmp = morecore(size);
			split_block(tmp, size);
		}

		UNMARK_FREE(tmp);
		memcpy(tmp + 1, b + 1, b->size);
		kfree(ptr);
		b = tmp;
	}

	return b + 1;
}

/* mark the current block as free and 
 * merge adjacent free blocks if possible 
 *
 * hell break loose if ptr doesn't point to the beginning of memory block */
void kfree(void *ptr)
{
	meta_t *b = GET_BASE(ptr), *tmp;
	MARK_FREE(b);

	if (b->prev != NULL && IS_FREE(b->prev)) {
		merge_blocks(b->prev, b);
		b = b->prev;
	}

	if (b->next != NULL && IS_FREE(b->next))
		merge_blocks(b, b->next);

	/* TODO: call kfree_frame */
}

/* allocate one page (4KiB) of initial memory for kheap */
void kheap_init()
{
	map_page((void*)kalloc_frame(), HEAP_START, P_PRESENT | P_READWRITE);
	kdebug("kernel heap initialized! Start addres: 0x%08x", HEAP_START);
	memset(HEAP_START, 0, 0x1000);

	kernel_base = (meta_t*)HEAP_START;
	kernel_base->size = 0x1000 - META_SIZE;
	kernel_base->next = kernel_base->prev = NULL;
	MARK_FREE(kernel_base);
	HEAP_BREAK = (uint32_t*)0xd0001000;
}

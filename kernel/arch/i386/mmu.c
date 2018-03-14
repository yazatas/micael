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

static uint32_t *PDIR_PHYS; /* see vmm_init */
static uint32_t START_FRAME = (uint32_t)&__kernel_physical_end;
static uint32_t FRAME_START = 0xf0000000;

static meta_t   *kernel_base = NULL;
static uint32_t *HEAP_START  = (uint32_t*)0xd0000000;
static uint32_t *HEAP_BREAK  = (uint32_t*)0xd0000000;

/* TODO: copy bitvector from file system and use it here */
static uint32_t PAGE_DIR[1024] = {PAGE_FREE}; /* FIXME: 1024 is not the correct value */
static uint32_t pg_dir_ptr = 0;


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

void *vmm_mkpdir(void *virtaddr, uint32_t flags)
{
	kdebug("allocating new page directory...");

	pageframe_t physaddr = vmm_kalloc_frame();
	vmm_map_page((void*)physaddr, virtaddr, flags);

	return (void*)physaddr;
}


void vmm_init(void)
{
	/* TODO: read multiboot info and 
	 * calculate the amount of available pages */

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
	vmm_heap_init();
	vmm_flush_TLB();
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

inline void vmm_map_kernel(void *pdir)
{
	((uint32_t*)pdir)[768] = (pageframe_t)PDIR_PHYS;
}


inline void vmm_flush_TLB(void)
{
	asm volatile ("mov %cr3, %ecx \n \
			       mov %ecx, %cr3");
}

inline void vmm_change_pdir(void *cr3)
{
	asm volatile ("push %%eax \n\
				   mov %0, %%eax \n\
				   mov %%eax, %%cr3 \n\
				   pop %%eax" 
				   :: "r" ((uint32_t)cr3));
}

/**************************** kernel heap ****************************/

static meta_t *morecore(size_t size)
{
	size_t pgcount = size / 0x1000 + 1;
	meta_t *tmp = (meta_t*)HEAP_BREAK;

	for (size_t i = 0; i < pgcount; ++i) {
		vmm_map_page((void*)vmm_kalloc_frame(), HEAP_BREAK, P_PRESENT | P_READWRITE);
		HEAP_BREAK = (uint32_t*)((uint8_t*)HEAP_BREAK + 0x1000);
	}
	vmm_flush_TLB();

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
		if ((b = morecore(size + META_SIZE)) == NULL) {
			kpanic("kernel heap exhausted");
			__builtin_unreachable();
		}

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
 * hell breaks loose if ptr doesn't point to the beginning of memory block */
void kfree(void *ptr)
{
	meta_t *b = GET_BASE(ptr), *tmp;
	MARK_FREE(b);

	kdebug("freeing memory block of size %u", b->size);

	if (b->prev != NULL && IS_FREE(b->prev)) {
		merge_blocks(b->prev, b);
		b = b->prev;
	}

	if (b->next != NULL && IS_FREE(b->next))
		merge_blocks(b, b->next);

	/* TODO: call vmm_kfree_frame */
}

/* allocate one page (4KiB) of initial memory for kheap */
void vmm_heap_init()
{
	vmm_map_page((void*)vmm_kalloc_frame(), HEAP_START, P_PRESENT | P_READWRITE);
	memset(HEAP_START, 0, 0x1000);

	kernel_base = (meta_t*)HEAP_START;
	kernel_base->size = 0x1000 - META_SIZE;
	kernel_base->next = kernel_base->prev = NULL;
	MARK_FREE(kernel_base);
	HEAP_BREAK = (uint32_t*)0xd0001000;

	kdebug("kernel heap initialized! Start addres: 0x%08x", HEAP_START);
}

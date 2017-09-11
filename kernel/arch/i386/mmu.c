#include <kernel/mmu.h>
#include <kernel/kprint.h>
#include <kernel/kpanic.h>

#include <stddef.h>
#include <stdbool.h>

#define FREE 0
#define USED 1

enum {
	P_PRESENT    = 1,
	P_READWRITE  = 1 << 1,
	P_SUPERVISOR = 0 << 2, /* TODO: what? */
} PAGING_FLAGS;

extern uint32_t boot_page_dir; /* this address is virtual */
extern uint32_t __kernel_virtual_end, __kernel_physical_end; 

static pageframe_t frame_map[PE_SIZE];
static pageframe_t pre_frames[20];

static uint32_t *PDIR_PHYS; /* see mmu_init */
static uint32_t START_FRAME = (uint32_t)&__kernel_physical_end;
static uint32_t FRAME_START = 0xf0000000;
static uint32_t HEAP_START  = 0xd0000000;

typedef struct meta {
	size_t size;
	unsigned flags;
	struct meta *next;
	struct meta *prev;
} __attribute__((packed)) meta_t;

/* TODO: switch from linked list to avl tree */
static meta_t *base = NULL;;

#define META_SIZE sizeof(meta_t)
#define IS_FREE(x)     (x->flags & 0x1)
#define MARK_FREE(x)   (x->flags |= 0x1)
#define UNMARK_FREE(x) (x->flags &= ~0x1)
#define GET_BASE(x)    ((meta_t*)x - 1)

static pageframe_t kalloc_frame_int(void)
{
	uint32_t i = 0;
	while (frame_map[i] != FREE) {

		i++;
		if (i == PE_SIZE) {
			return KALLOC_NO_MEM_ERROR;
		}
	}

	frame_map[i] = USED;
	return (START_FRAME + (i * PF_SIZE));
}

/* TODO: this function should update kernel page directory */
/* TODO: or should it? */
/* pageframe_t kalloc_frame(void) */
/* { */
/* 	static bool allocate = true; /1* allocate new set of preframes yay/nay *1/ */
/* 	static uint8_t nframes = 0;  /1* number of free frames *1/ */
/* 	pageframe_t ret; */
/* 	/1* we're out of free frames, allocate more *1/ */
/* 	if (nframes == 20) { */
/* 		allocate = true; */
/* 	} */
/* 	if (allocate == true) { */
/* 		for (size_t i = 0; i < 20; ++i) { */
/* 			/1* TODO: this should trigger a page fault *1/ */
/* 			if ((pre_frames[i] = kalloc_frame_int()) == KALLOC_NO_MEM_ERROR) { */
/* 				kpanic("out of free pages!"); */
/* 			} */
/* 		} */
/* 		nframes = 0; */
/* 		allocate = false; */
/* 	} */
/* 	ret = pre_frames[nframes]; */
/* 	nframes++; */
/* 	return ret; */
/* } */

void kfree_frame(pageframe_t frame)
{
	pageframe_t tmp = frame - START_FRAME;
	uint32_t index;

	if (frame == 0) {
		index = tmp;
		frame_map[index] = FREE;
	} else {
		index = tmp / PF_SIZE;
		frame_map[index] = FREE;
	}
}

/* TODO: wew */
/* return a physical address to page frame */
uint32_t kalloc_frame(void)
{
	static size_t i = 0;
	uint32_t tmp = START_FRAME + (i * 0x1000);
	i++;
	return tmp;
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

	kprint("faulting address: 0x%x\n", fault_addr);
	kprint("page directory index: %4u 0x%03x\n"
		   "page table index:     %4u 0x%03x\n"
		   "page frame offset:    %4u 0x%03x\n",
		   pdi, pdi, pti, pti, offset, offset);

	uint32_t *pd = (uint32_t*)0xffffff000;
	if (!(pd[pdi] & P_PRESENT)) {
		kprint("pde %u is NOT present\n", pdi);
		pd[pdi] = kalloc_frame() | P_PRESENT | P_READWRITE;
	}

	uint32_t *pt = ((uint32_t*)0xffc00000) + (0x400 * pdi);
	if (!(pt[pti] & P_PRESENT)) {
		kprint("pte %u is NOT present\n", pti);
		pt[pti] = kalloc_frame() | P_PRESENT | P_READWRITE;
	}

	/* flush TLB */
	asm volatile ("mov %cr3, %ecx \n \
				  mov %ecx, %cr3");
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

	/* flush TLB */
	asm volatile ("mov %cr3, %ecx \n \
			       mov %ecx, %cr3");
}


void mmu_init(void)
{
	/* 4k align START_FRAME */
	if (START_FRAME % 0x1000 != 0) {
		START_FRAME = (START_FRAME + 0x1000) & ~(0x1000 - 1);
	}
	kprint("START_FRAME aligned to 4k boundary: 0x%08x\n", START_FRAME);

	/* initialize recursive page directory and invalidate the first PTE */
	PDIR_PHYS = (uint32_t*)((uint32_t)&boot_page_dir - 0xc0000000);
	PDIR_PHYS[1023] = (uint32_t)PDIR_PHYS | P_PRESENT | P_READWRITE;
	kprint("recursive page directory enabled!\n");

	/* init kernel heap */
	map_page((void*)kalloc_frame(), (void*)HEAP_START, P_PRESENT | P_READWRITE);
	kprint("kernel heap initialized! Start addres: 0x%08x\n", HEAP_START);
	memset((void*)HEAP_START, 0, 0x1000);

	base = (meta_t*)HEAP_START;
	base->size = 0x1000 - META_SIZE;
	base->next = base->prev = NULL;
	MARK_FREE(base);
}

static meta_t *morecore(size_t size)
{

}

static meta_t *split_block(meta_t *b, size_t split)
{
	if (b->size <= split || b->size - split - META_SIZE <= 0)
		return NULL;

	meta_t *tmp = (meta_t*)((uint8_t*)b + split + META_SIZE);
	tmp->size = b->size - split - META_SIZE;
	b->size = split;
	MARK_FREE(tmp);

	return tmp;
}

static meta_t *find_free_block(size_t size)
{
	meta_t *b = base, *tmp = NULL;

	while (b && !(IS_FREE(b) && b->size >= size)) {
		b = b->next;
	}

	if (b != NULL) {
		UNMARK_FREE(b);

		if ((tmp = split_block(b, size)) != NULL) {
			tmp->next = b->next;
			if (b->next)
				b->next->prev = tmp;
			tmp->prev = b;
			b->next = tmp;
		}
	}
	return b;
}

void *kmalloc(size_t size)
{
	meta_t *b;

	if ((b = find_free_block(size)) == NULL) {
		kprint("SOS! find_free_block returned NULL!\n\n\n");
		return NULL;
	}
	/* TODO: else allocate more pages */

	return b + 1;
}

void *kcalloc(size_t nmemb, size_t size)
{
	meta_t *b;

	if ((b = kmalloc(nmemb * size)) == NULL)
		return NULL;

	memset(b + 1, 0, nmemb * size);
	return b + 1;
}

/* TODO:  */
void *krealloc(void *ptr, size_t size)
{
	return NULL;
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
		tmp = b->prev;
		tmp->size += b->size;
		tmp->next = b->next;
		if (b->next)
			b->next->prev = tmp;
		b = tmp;
	}

	if (b->next != NULL && IS_FREE(b->next)) {
		tmp = b->next;
		b->size += tmp->size;
		b->next = tmp->next;
		if (tmp->next)
			tmp->next->prev = b;
	}
}

void traverse()
{
	meta_t *b = base;

	kprint("-------------------------------------\n");
	while (b) {
		kprint("b->size %u is free %u b->next 0x%08x b->prev 0x%08x\n",
				b->size, IS_FREE(b), b->next, b->prev);
		b = b->next;
	}
	kprint("-------------------------------------\n");
}

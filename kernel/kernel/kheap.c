#include <kernel/mmu.h>
#include <kernel/kprint.h>
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

static meta_t *kernel_base   = NULL;
static uint32_t *HEAP_START  = (uint32_t*)0xd0000000;

static meta_t *morecore(size_t size)
{
	/* TODO: how to know where the current heap break is?? */
	/* TODO: create separate physical address container for kheap?? */

	/* TODO: i don't need to keep track of physical pages (that's done by kalloc_frame)
	 * but instead i need to know where the current break is (what info do i need to 
	 * acquire that knowled??) */
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
	meta_t *b = kernel_base, *tmp = NULL;

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

void *krealloc(void *ptr, size_t size)
{
	/* check if merging adjacent blocks achieves the goal size */
	meta_t *b = GET_BASE(ptr), *tmp;
	size_t new_size = b->size;

	if (b->prev && IS_FREE(b->prev)) new_size += b->prev->size;
	if (b->next && IS_FREE(b->next)) new_size += b->next->size;

	if (new_size >= size) {
		if (b->prev != NULL && IS_FREE(b->prev)) {
			tmp = b->prev;
			tmp->size += b->size;
			tmp->next = b->next;
			if (b->next)
				b->next->prev = tmp;

			/* move b contents to the beginning of new block */
			memmove(tmp, b, b->size);
			b = tmp;
		}

		if (b->next != NULL && IS_FREE(b->next)) {
			tmp = b->next;
			b->size += tmp->size;
			b->next = tmp->next;
			if (tmp->next)
				tmp->next->prev = b;
		}

		/* split block if possible to save space */
		meta_t *split;
		if ((split = split_block(b, size)) != NULL) {
			tmp->next = b->next;
			if (b->next)
				b->next->prev = tmp;
			tmp->prev = b;
			b->next = tmp;
		}

	} else {
		tmp = find_free_block(size);
		if (!tmp) {
			/* TODO: allocate more memory */
		}

		memcpy(tmp, b, b->size);
		MARK_FREE(b); /* TODO: kfree?? */
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
		tmp = b->prev;
		tmp->size += b->size + META_SIZE; /* header is part of the block size too */
		tmp->next = b->next;
		if (b->next)
			b->next->prev = tmp;
		b = tmp;
	}

	if (b->next != NULL && IS_FREE(b->next)) {
		tmp = b->next;
		b->size += tmp->size + META_SIZE; /* header is part of the block size too */
		b->next = tmp->next;
		if (tmp->next)
			tmp->next->prev = b;
	}
}

void traverse()
{
	meta_t *b = kernel_base;

	kprint("-------------------------------------\n");
	while (b) {
		kprint("b 0x%08x b->size %4u free %u b->prev 0x%08x b->next 0x%08x\n",
				b, b->size, IS_FREE(b), b->prev, b->next);
		b = b->next;
	}
	kprint("-------------------------------------\n");
}

void kheap_init()
{
	map_page((void*)kalloc_frame(), HEAP_START, P_PRESENT | P_READWRITE);
	kprint("kernel heap initialized! Start addres: 0x%08x\n", HEAP_START);
	memset(HEAP_START, 0, 0x1000);

	kernel_base = (meta_t*)HEAP_START;
	kernel_base->size = 0x1000 - META_SIZE;
	kernel_base->next = kernel_base->prev = NULL;
	MARK_FREE(kernel_base);
}

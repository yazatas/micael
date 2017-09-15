#include <kernel/mmu.h>
#include <kernel/kprint.h>
#include <kernel/kheap.h>
#include <kernel/kpanic.h>
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

static meta_t   *kernel_base = NULL;
static uint32_t *HEAP_START  = (uint32_t*)0xd0000000;
static uint32_t *HEAP_BREAK  = (uint32_t*)0xd0000000;

/* TODO: this never returns NULL, map_page status code when??? */
static meta_t *morecore(size_t size)
{
	size_t pgcount = size / 0x1000 + 1;
	meta_t *tmp = (meta_t*)HEAP_BREAK;

	for (size_t i = 0; i < pgcount; ++i) {
		map_page((void*)kalloc_frame(), HEAP_BREAK, P_PRESENT | P_READWRITE);
		HEAP_BREAK = (uint32_t*)((uint8_t*)HEAP_BREAK + 0x1000);
	}
	flush_tlb();

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

void traverse()
{
	meta_t *b = kernel_base;

	kprint("-------------------------------------\n");
	while (b) {
		kprint("b 0x%08x b->size %4d free %u b->prev 0x%08x b->next 0x%08x\n",
				b, b->size, IS_FREE(b), b->prev, b->next);
		b = b->next;
	}
	kprint("-------------------------------------\n");
}

/* allocate one page (4KiB) of initial memory for kheap */
void kheap_init()
{
	map_page((void*)kalloc_frame(), HEAP_START, P_PRESENT | P_READWRITE);
	kprint("kernel heap initialized! Start addres: 0x%08x\n", HEAP_START);
	memset(HEAP_START, 0, 0x1000);

	kernel_base = (meta_t*)HEAP_START;
	kernel_base->size = 0x1000 - META_SIZE;
	kernel_base->next = kernel_base->prev = NULL;
	MARK_FREE(kernel_base);
	HEAP_BREAK = (uint32_t*)0xd0001000;
}

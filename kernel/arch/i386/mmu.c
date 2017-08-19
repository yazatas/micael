#include <kernel/mmu.h>
#include <kernel/kprint.h>

#include <stddef.h>
#include <stdbool.h>

#define FREE 0
#define USED 1

extern uint32_t __kernel_end;
static uint32_t START_FRAME = (uint32_t)&__kernel_end;
static size_t npages = 256;

static pageframe_t frame_map[256];
static pageframe_t pre_frames[20];

static pageframe_t kalloc_frame_int(void)
{
	uint32_t i = 0;
	while (frame_map[i] != FREE) {


		i++;
		if (i == npages) {
			return KALLOC_NO_MEM_ERROR;
		}
	}

	frame_map[i] = USED;
	return (START_FRAME + (i * PF_SIZE));
}

pageframe_t kalloc_frame(void)
{
	static bool allocate = true; /* allocate new set of preframes yay/nay */
	static uint8_t nframes = 0;  /* number of free frames */
	pageframe_t ret;

	/* we're out of free frames, allocate more */
	if (nframes == 20) {
		allocate = true;
	}

	if (allocate == true) {
		for (size_t i = 0; i < 20; ++i) {
			if ((pre_frames[i] = kalloc_frame_int()) == KALLOC_NO_MEM_ERROR) {
				kpanic("out of free pages");
			}
		}
		nframes = 0;
		allocate = false;
	}

	ret = pre_frames[nframes];
	nframes++;

	return ret;
}

void kfree_frame(pageframe_t frame)
{
	pageframe_t tmp = frame - START_FRAME;
	if (frame == 0) {
		uint32_t index = tmp;
		frame_map[index] = FREE;
	} else {
		tmp = frame;
		uint32_t index = tmp / PF_SIZE;
		frame_map[index] = FREE;
	}
}

void mmu_init(void)
{
	/* size_t i; */
	/* uint32_t address = 0; */
	/* /1* set PDEs to not present *1/ */
	/* for (i = 1; i < 1024; ++i) { */
	/* 	page_dir[i] = 0x00000002; */
	/* } */
	/* for (i = 0; i < 10000; ++i) { */
	/* 	page_table[i] = address | 3; */
	/* 	address += 0x1000; */
	/* } */
	/* /1* set the first page dir present *1/ */
	/* page_dir[0] = page_table; */
	/* page_dir[0] |= 3; */
	/* asm volatile ("mov %0, %%eax         \n \ */
	/* 		       mov %%eax, %%cr3      \n \ */
	/* 			   mov %%cr0, %%eax      \n \ */
	/* 			   or $0x80000000, %%eax \n \ */
	/* 			   mov %%eax, %%cr0" :: "r" (page_dir)); */
	/* kprint("Paging enabled!\n"); */
}

#include <kernel/mmu.h>
#include <kernel/kprint.h>

#include <stddef.h>

void mmu_init(void)
{
	size_t i;
	uint32_t address = 0;

	/* set PDEs to not present */
	for (i = 1; i < 1024; ++i) {
		page_dir[i] = 0x00000002;
	}

	for (i = 0; i < 10000; ++i) {
		page_table[i] = address | 3;
		address += 0x1000;
	}

	/* set the first page dir present */
	page_dir[0] = page_table;
	page_dir[0] |= 3;

	asm volatile ("mov %0, %%eax         \n \
			       mov %%eax, %%cr3      \n \
				   mov %%cr0, %%eax      \n \
				   or $0x80000000, %%eax \n \
				   mov %%eax, %%cr0" :: "r" (page_dir));

	kprint("Paging enabled!\n");
}

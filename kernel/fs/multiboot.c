#include <fs/multiboot.h>

/* TODO: should this return a some kind of memory map? */

void vfs_multiboot_read(multiboot_info_t *mbinfo)
{
	multiboot_memory_map_t *mmap;
     
   kprint ("mmap_addr = 0x%x, mmap_length = 0x%x\n", (unsigned) mbinfo->mmap_addr, (unsigned) mbinfo->mmap_length);

   for (mmap = (multiboot_memory_map_t *) mbinfo->mmap_addr;
		mmap < mbinfo->mmap_addr + mbinfo->mmap_length;
		mmap = (multiboot_memory_map_t *) (mmap + mmap->size + sizeof (mmap->size)))
   {
		 kprint (" size = 0x%x, base_addr = 0x%x%08x,"
				 " length = 0x%x%08x, type = 0x%x\n",
				 (unsigned) mmap->size,
				 (unsigned) (mmap->addr >> 32),
				 (unsigned) (mmap->addr & 0xffffffff),
				 (unsigned) (mmap->len >> 32),
				 (unsigned) (mmap->len & 0xffffffff),
				 (unsigned) mmap->type);
	}
}

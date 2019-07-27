#include <arch/x86_64/mm/mmu.h>
#include <arch/i386/mm/mmu.h>
#include <kernel/kprint.h>
#include <kernel/kpanic.h>
#include <mm/bootmem.h>
#include <mm/heap.h>
#include <mm/mmu.h>
#include <mm/page.h>
#include <mm/slab.h>

int mmu_init(void *arg)
{
#ifdef __i386__
    return mmu_native_init(arg);
#endif

    /* initialize all components of the memory management unit
     * these functions don't return any status codes because it is assumed that
     * they succeed and if not, the condition is fatal enough to cause kernel panic */

    /* switch from boot page directory to properly initialized directory 
     * and map all of the address space to the new page directory */
    mmu_native_init();

    /* Page frame allocator needs access slab cache to work and slab in turn 
     * requires kernel heap to work. Preinitialize heap and slab using 
     * the boot memory allocator which holds a simple bitmap of free pages 
     *
     * When the page frame allocator has been initialized, bootmem is no longer used
     * and heap and slab will be reinitialized, this time using page frame allocator */
    (void)mmu_bootmem_init(arg);
    (void)mmu_heap_preinit();
    (void)mmu_slab_preinit();

    /* initialize page frame allocator and build the memory map */
    mmu_zones_init(arg);

    /* Reinitialize kernel heap and slab allocator using PFA */
    mmu_heap_init();
    mmu_slab_init();

    kdebug("MMU initialized!");

    return 0;
}

static int __map_range(unsigned long pstart, unsigned long vstart, size_t n, int flags)
{
    int ret = 0;

    for (size_t i = 0; i < n; ++i) {
        if ((ret = mmu_native_map_page(pstart, vstart, flags)) != 0) {
            kdebug("failed to map physical address 0x%x to 0x%x", pstart, vstart);
            return ret;
        }

        pstart += PAGE_SIZE;
        vstart += PAGE_SIZE;
    }

    return ret;
}

int mmu_map_page(unsigned long paddr, unsigned long vaddr, int flags)
{
    return mmu_native_map_page(paddr, vaddr, flags);
}

int mmu_unmap_page(unsigned long vaddr)
{
    return mmu_native_unmap_page(vaddr);
}

int mmu_map_range(unsigned long pstart, unsigned long vstart, size_t n, int flags)
{
    return __map_range(pstart, vstart, n, flags);
}

int mmu_unmap_range(unsigned long start, size_t n)
{
    int ret = 0;

    for (size_t i = 0; i < n; ++i) {
        if ((ret = mmu_native_unmap_page(start + i * PAGE_SIZE)) != 0) {
            kdebug("failed to unmap page at address 0x%x", start + i * PAGE_SIZE);
            return ret;
        }
    }

    return ret;
}

unsigned long mmu_v_to_p(void *vaddr)
{
    return mmu_native_v_to_p(vaddr);
}

void *mmu_p_to_v(unsigned long paddr)
{
    return mmu_native_p_to_v(paddr);
}

void *mmu_build_dir(void)
{
    return mmu_native_build_dir();
}

void *mmu_duplicate_dir(void)
{
    return mmu_native_duplicate_dir();
}

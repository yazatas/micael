#include <mm/mmu.h>
#include <arch/x86_64/mm/mmu.h>
#include <arch/i386/mm/mmu.h>

int mmu_init(void *arg)
{
#ifdef __x86_64__
    return mmu_native_init();
#elif defined(__i386__)
    return mmu_native_init(arg);
#endif
}

cr3_t *mmu_alloc_cr3()
{
    return mmu_native_alloc_cr3();
}

cr3_t *mmu_duplicate_cr3(cr3_t *cr3)
{
    return mmu_native_duplicate_cr3(cr3);
}

int mmu_dealloc_cr3(cr3_t *cr3)
{
    return mmu_native_dealloc_cr3(cr3);
}

void mmu_switch_cr3(cr3_t *cr3)
{
    mmu_native_switch_cr3(cr3);
}

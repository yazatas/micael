#include <arch/amd64/mm/mmu.h>
#include <kernel/common.h>
#include <kernel/irq.h>
#include <kernel/kpanic.h>
#include <kernel/kprint.h>
#include <kernel/percpu.h>
#include <kernel/util.h>
#include <mm/mmu.h>
#include <mm/page.h>
#include <sched/sched.h>

static void __walk_dir(uint64_t cr3, uint16_t pml4i, uint16_t pdpti, uint16_t pdi, uint16_t pti)
{
    kprint("\n");

    uint64_t *pml4 = mmu_p_to_v(cr3);
    kprint("\tPML4: 0x%x 0x%x\n", pml4, cr3);
    kprint("\tPML4[%u] %spresent\n\n", pml4i, (pml4[pml4i] & MM_PRESENT) ? "" : "not ");

    uint64_t *pdpt = mmu_p_to_v(pml4[pml4i] & ~(PAGE_SIZE - 1));
    kprint("\tPDPT: 0x%x 0x%x\n", pdpt, pml4[pml4i]);
    kprint("\tPDPT[%u] %spresent\n\n", pdpti, (pdpt[pdpti] & MM_PRESENT) ? "" : "not ");

    uint64_t *pd = mmu_p_to_v(pdpt[pdpti] & ~(PAGE_SIZE - 1));
    kprint("\tPD: 0x%x 0x%x\n", pd, pdpt[pdpti]);
    kprint("\tPD[%u] %spresent\n\n", pdi, (pd[pdi] & MM_PRESENT) ? "" : "not ");

    uint64_t *pt = mmu_p_to_v(pd[pdi] & ~(PAGE_SIZE - 1));
    kprint("\tPT: 0x%x 0x%x\n", pt, pd[pdi]);
    kprint("\tPD[%u] %spresent\n\n", pti, (pt[pti] & MM_PRESENT) ? "" : "not ");
}

static void __print_error(uint32_t error)
{
    const char *s[3] = {
        ((error >> 1) & 0x1) ? "Page write"       : "Page read",
        ((error >> 0) & 0x1) ? "protection fault" : "not present",
        ((error >> 2) & 0x1) ? "user"             : "supervisor"
    };

    kprint("\n\n----------------------------------\n");
    kprint("%s, %s (%s)\n", s[0], s[1], s[2]);

    if ((error >> 3) & 0x1)
        kprint("CPU read 1 from reserved field!\n");

    if ((error >> 4) & 0x1)
        kprint("Faulted due to instruction fetch (NXE set?)\n");
}

uint32_t mmu_pf_handler(void *ctx)
{
    isr_regs_t *cpu_state = (isr_regs_t *)ctx;

    uint64_t cr2   = 0;
    uint64_t cr3   = 0;
    uint32_t error = cpu_state->err_num;
    task_t *task   = sched_get_active();

    asm volatile ("mov %%cr3, %0" : "=r"(cr3));
    asm volatile ("mov %%cr2, %0" : "=r"(cr2));

    unsigned long pml4i = (cr2 >> 39) & 0x1ff;
    unsigned long pdpti = (cr2 >> 30) & 0x1ff;
    unsigned long pdi   = (cr2 >> 21) & 0x1ff;
    unsigned long pti   = (cr2 >> 12) & 0x1ff;

    uint64_t *pml4 = mmu_p_to_v(cr3);

    if (!(pml4[pml4i] & MM_PRESENT))
        goto error;

    uint64_t *pdpt = mmu_p_to_v(pml4[pml4i] & ~(PAGE_SIZE - 1));

    if (!(pdpt[pdpti] & MM_PRESENT))
        goto error;

    uint64_t *pd = mmu_p_to_v(pdpt[pdpti] & ~(PAGE_SIZE - 1));

    if (!(pd[pdi] & MM_PRESENT))
        goto error;

    uint64_t *pt = mmu_p_to_v(pd[pdi] & ~(PAGE_SIZE - 1));

    if (!(pt[pti] & MM_PRESENT))
        goto error;

    if ((pt[pti] & MM_COW) && !(pt[pti] & MM_READWRITE)) {
        static spinlock_t lock = 0;
        spin_acquire(&lock);

        unsigned long copy = mmu_page_alloc(MM_ZONE_NORMAL, 0);
        uint8_t *copy_v    = mmu_native_p_to_v(copy);
        int flags          = MM_PRESENT | MM_USER | MM_READWRITE; /* TODO: preserve flags */

        kmemcpy(copy_v, (void *)ROUND_DOWN(cr2, PAGE_SIZE), PAGE_SIZE);

        pt[pti] = (unsigned long)copy | flags;
        spin_release(&lock);
        return IRQ_HANDLED;
    }

error:
    __print_error(error);
    kprint("\nFaulting address: 0x%08x %10u\n", cr2, cr2);
    kprint("PML4 index:       0x%08x %10u\n"
           "PDPT index:       0x%08x %10u\n"
           "PD index:         0x%08x %10u\n"
           "PT index:         0x%08x %10u\n"
           "Offset:           0x%08x %10u\n\n",
           pml4i, pml4i, pdpti, pdpti, pdi, pdi, pti, pti, cr2 & 0xfff, cr2 & 0xfff);

    kprint("CPUID:            0x%08x %10u\n", get_thiscpu_id(), get_thiscpu_id());
    kprint("CR3:              0x%08x %10u\n", cr3, cr3);

    if (task)
        kprint("task name:        %s\n", sched_get_active()->name);

    native_dump_registers(cpu_state);

    /* Walk the directory and the virtual address of each directory
     * and whether the entry is present or not */
    /* __walk_dir(cr3, pml4i, pdpti, pdi, pti); */

    for (;;);
}

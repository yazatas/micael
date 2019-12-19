#include <kernel/cpu.h>
#include <kernel/compiler.h>
#include <kernel/kpanic.h>
#include <kernel/kprint.h>
#include <kernel/percpu.h>
#include <mm/mmu.h>
#include <errno.h>
#include <stdint.h>

void __noreturn kpanic(const char *error)
{
    disable_irq();

    kprint("\n");
    kdebug("'%s'", error);
    kdebug("errno: %s", kstrerror(errno));
    kdebug("CPUID: %u", get_thiscpu_id());

    arch_dump_registers();

    /* mmu_print_memory_map(); */
    /* mmu_list_pde(); */

    while (1) { }
    __builtin_unreachable();
}

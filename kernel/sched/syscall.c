#include <sched/syscall.h>
#include <sched/proc.h>
#include <sched/sched.h>
#include <mm/vmm.h>
#include <mm/kheap.h>
#include <kernel/kprint.h>
#include <kernel/kpanic.h>

#define MAX_SYSCALLS 3
#define SYSCALL_NAME(name) sys_##name
#define DEFINE_SYSCALL(name) uint32_t SYSCALL_NAME(name)(isr_regs_t *cpu)

typedef uint32_t (*syscall_t)(isr_regs_t *cpu);

DEFINE_SYSCALL(read)
{
    int fd = cpu->ecx;
}

DEFINE_SYSCALL(write)
{
    char *ptr = (char*)cpu->ebx;
    for (size_t i = 0; i < cpu->ecx; ++i)
        kprint("%c", ptr[i]);
    return 0;
}

DEFINE_SYSCALL(fork)
{
    (void)cpu;
    pcb_t *p = kmalloc(sizeof(pcb_t));
    p->pid   = get_next_pid();

    uint32_t cr3;
    asm volatile ("mov %%cr3, %0" : "=r"(cr3));

    void *page_dir = vmm_duplicate_pdir((void*)cr3);
    vmm_change_pdir(vmm_v_to_p(page_dir));

    return 0;
}

static syscall_t syscalls[MAX_SYSCALLS] = {
    [0] = SYSCALL_NAME(read),
    [1] = SYSCALL_NAME(write),
    [2] = SYSCALL_NAME(fork),
};

void syscall_handler(isr_regs_t *cpu)
{
    if (cpu->eax >= MAX_SYSCALLS) {
        kpanic("unsupported system call");
    } else {
        uint32_t ret = syscalls[cpu->eax](cpu);
    }
}

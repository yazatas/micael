#include <string.h>
#include <kernel/kprint.h>
#include <kernel/kpanic.h>
#include <mm/mmu.h>
#include <mm/heap.h>
#include <sched/sched.h>
#include <sched/syscall.h>

#define MAX_SYSCALLS 4
#define SYSCALL_NAME(name) sys_##name
#define DEFINE_SYSCALL(name) uint32_t SYSCALL_NAME(name)(isr_regs_t *cpu)

typedef uint32_t (*syscall_t)(isr_regs_t *cpu);

DEFINE_SYSCALL(fork)
{
    (void)cpu;

    task_t *cur = sched_get_current();
    task_t *t   = sched_task_fork(cur);

    /* forking failed, errno has been set
     * return -1 to parent */
    if (!t)
        return -1;

    sched_task_schedule(t);

    /* return pid to child, 0 to parent */
    t->threads->exec_state->eax = t->pid;

    return 0;
}

DEFINE_SYSCALL(execv)
{
    (void)cpu;

    return 0;
}

static syscall_t syscalls[MAX_SYSCALLS] = {
    [2] = SYSCALL_NAME(fork),
    [3] = SYSCALL_NAME(execv),
};

void syscall_handler(isr_regs_t *cpu)
{
    if (cpu->eax >= MAX_SYSCALLS) {
        kpanic("unsupported system call");
    } else {
        uint32_t ret = syscalls[cpu->eax](cpu);

        /* return value is transferred in eax */
        task_t *current = sched_get_current();
        current->threads->exec_state->eax = ret;
    }
}

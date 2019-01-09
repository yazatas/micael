#include <errno.h>
#include <string.h>

#include <fs/binfmt.h>
#include <fs/fs.h>
#include <kernel/kprint.h>
#include <kernel/kpanic.h>
#include <mm/mmu.h>
#include <mm/heap.h>
#include <sched/sched.h>
#include <sched/syscall.h>

#define MAX_SYSCALLS 7

typedef int32_t (*syscall_t)(isr_regs_t *cpu);

int32_t sys_fork(isr_regs_t *cpu)
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

int32_t sys_execv(isr_regs_t *cpu)
{
    const char *path = (const char *)cpu->ebx;

    file_t *file   = NULL;
    dentry_t *dntr = NULL;

    if ((dntr = vfs_lookup(path)) == NULL)
        return -1;

    if ((file = vfs_open_file(dntr)) == NULL)
        return -1;

    /* clear page tables of current process:
     * mark all user page tables as not present and try to free
     * as many page frames as possible (all pages not marked as CoW) */
    mmu_unmap_pages(0, KSTART - 1);

    /* binfmt_load either jumps to user land and starts to execute
     * the new process or it fails (and returns) and we must return -1 to caller */
    binfmt_load(file, 0, NULL);

    return -1;
}

int32_t sys_exit(isr_regs_t *cpu)
{
    int status      = cpu->eax;
    task_t *current = sched_get_current();
    task_t *parent  = current->parent;

    kdebug("exiting from %s ( pid %d): status %d", current->name, current->pid, status);
    sched_print_tasks();

    /* reassign new parent for current's children */
    if (LIST_EMPTY(current->children) == false) {
        if (parent == NULL)
            parent = sched_get_init();

        /* TODO: assign new parent for children */
    }

    /* TODO: signal parent (SIGCHLD) */

    /* free all used memory (all memory that can be freed) */
    vfs_free_fs_context(current->fs_ctx);
    sched_free_threads(current);
    mmu_unmap_pages(0, KSTART - 1);

    current->threads->state = T_ZOMBIE;

    sched_switch();
}

static syscall_t syscalls[MAX_SYSCALLS] = {
    [2] = sys_fork,
    [3] = sys_execv,
    [6] = sys_exit,
};

void syscall_handler(isr_regs_t *cpu)
{
    if (cpu->eax >= MAX_SYSCALLS) {
        kpanic("unsupported system call");
    } else {
        int32_t ret = syscalls[cpu->eax](cpu);

        /* return value is transferred in eax */
        task_t *current = sched_get_current();
        current->threads->exec_state->eax = ret;
    }
}

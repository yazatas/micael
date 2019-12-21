#include <errno.h>
#include <fs/binfmt.h>
#include <fs/fs.h>
#include <fs/file.h>
#include <kernel/kassert.h>
#include <kernel/kprint.h>
#include <kernel/kpanic.h>
#include <kernel/util.h>
#include <mm/mmu.h>
#include <mm/heap.h>
#include <sched/sched.h>
#include <sched/syscall.h>

#define MAX_SYSCALLS 8

typedef int32_t (*syscall_t)(isr_regs_t *cpu);

int32_t sys_read(isr_regs_t *cpu)
{
    int fd          = (int)cpu->edx;
    void *buf       = (void *)cpu->ebx;
    size_t len      = (size_t)cpu->ecx;
    task_t *current = sched_get_current();

    if ((buf == NULL) ||
        (!current->file_ctx || !current->file_ctx->fd) ||
        (fd < 0 || fd >= current->file_ctx->numfd))
    {
        errno = EINVAL;
        return -1;
    }

    if (current->file_ctx->fd[fd] == NULL) {
        errno = ENOENT; /* TODO: ??? */
        return -1;
    }

    int nread = file_read(current->file_ctx->fd[fd], 0, len, buf);
    current->threads->exec_state = (exec_state_t *)cpu;

    return nread;
}

int32_t sys_write(isr_regs_t *cpu)
{
    int fd          = (int)cpu->edx;
    void *buf       = (void *)cpu->ebx;
    size_t len      = (size_t)cpu->ecx;
    task_t *current = sched_get_current();

    if ((buf == NULL) ||
        (!current->file_ctx || !current->file_ctx->fd) ||
        (fd < 0 || fd >= current->file_ctx->numfd))
    {
        errno = EINVAL;
        return -1;
    }

    if (current->file_ctx->fd[fd] == NULL) {
        errno = ENOENT; /* TODO: ??? */
        return -1;
    }

    return file_write(current->file_ctx->fd[fd], 0, len, buf);
}

int32_t sys_fork(isr_regs_t *cpu)
{
    (void)cpu;

    task_t *cur = sched_get_current();
    task_t *t   = sched_task_fork(cur);

    /* forking failed, errno has been set
     * return -1 to parent */
    if (!t)
        return -1;

    /* set the task's state T_READY and schedule it */
    sched_task_set_state(t, T_READY);

    /* return 0 to child, pid to parent */
    t->threads->exec_state->eax = 0;

    return t->pid;
}

int32_t sys_execv(isr_regs_t *cpu)
{
    char *p      = (char *)cpu->ebx;
    file_t *file = NULL;
    path_t *path = NULL;

    if ((path = vfs_path_lookup(p, LOOKUP_OPEN))->p_status != LOOKUP_STAT_SUCCESS) {
        kdebug("Path lookup failed for %s, status %d", p, path->p_status);
        goto error;
    }

    if ((file = file_open(path->p_dentry, O_RDONLY)) == NULL) {
        kdebug("file_open(%s): %s", p, kstrerror(errno));
        goto error;
    }

    vfs_path_release(path);

    /* clear page tables of current process:
     * mark all user page tables as not present and try to free
     * as many page frames as possible (all pages not marked as CoW) */
    /* TODO:  */
    /* mmu_unmap_pages(0, KSTART - 1); */

    /* binfmt_load either jumps to user land and starts to execute
     * the new process or it fails (and returns) and we must return -1 to caller */
    binfmt_load(file, 0, NULL);

error:
    vfs_path_release(path);
    return -1;
}

int32_t sys_exit(isr_regs_t *cpu)
{
    int status      = cpu->eax;
    task_t *current = sched_get_current();
    task_t *parent  = current->parent;

    /* kdebug("exiting from %s (pid %d): status %d", current->name, current->pid, status); */

    /* reassign new parent for current task's children */
    if (LIST_EMPTY(current->children) == false) {
        if (parent == NULL)
            parent = sched_get_init();
        /* TODO: assign new parent for children */
    }

    /* free all used memory (all memory that can be freed) */
    vfs_free_fs_ctx(current->fs_ctx);
    vfs_free_file_ctx(current->file_ctx);
    sched_free_threads(current);

    /* Set task's state to T_ZOMBIE and append it to parent's
     * zombie list from where it will be reaped when parent is rescheduled
     * and send a wakeup signal to parent's wait queue */
    sched_task_set_state(current, T_ZOMBIE);
    wq_wakeup(&parent->wqh_child);

    sched_switch();
}

int32_t sys_wait(isr_regs_t *cpu)
{
    task_t *current = sched_get_current();
    task_t *child   = NULL;

    /* wait for one of the children to wake us up so we can reap it */
    wq_wait_event(&current->wqh_child, current, NULL);

    /* One of current's children called exit(),
     * reap all zombies and return back to user land */
    FOREACH(current->children, iter) {
        child = container_of(iter, task_t, p_children);

        kassert(child != NULL);

        if (child->threads->state != T_ZOMBIE)
            continue;

        /* sched_task_destroy() releases all memory the child still has,
         * including kernel stack and page directory, and removes it
         * from all possible lists and finally deallocates the task object */
        sched_task_destroy(child);
    }

    return 0;
}

static syscall_t syscalls[MAX_SYSCALLS] = {
    [0] = sys_read,
    [1] = sys_write,
    [2] = sys_fork,
    [3] = sys_execv,
    [6] = sys_exit,
    [7] = sys_wait,
};

void syscall_handler(isr_regs_t *cpu)
{
    if (cpu->eax >= MAX_SYSCALLS) {
        kpanic("unsupported system call");
    } else {
        task_t *current = sched_get_current();
        int32_t ret     = syscalls[cpu->eax](cpu);

        /* return value is transferred in eax */
        current->threads->exec_state->eax = ret;
    }
}

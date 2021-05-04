#include <errno.h>
#include <fs/binfmt.h>
#include <fs/fs.h>
#include <fs/file.h>
#include <kernel/irq.h>
#include <kernel/kassert.h>
#include <kernel/kprint.h>
#include <kernel/kpanic.h>
#include <kernel/util.h>
#include <mm/mmu.h>
#include <mm/heap.h>
#include <net/socket.h>
#include <sched/sched.h>
#include <sched/syscall.h>
#include <sys/socket.h>

#define MAX_SYSCALLS 9

typedef int32_t (*syscall_t)(isr_regs_t *cpu);

int32_t sys_read(isr_regs_t *cpu)
{
    int fd          = (int)cpu->rdx;
    void *buf       = (void *)cpu->rbx;
    size_t len      = (size_t)cpu->rcx;
    task_t *current = sched_get_active();

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
    int fd          = (int)cpu->rdx;
    void *buf       = (void *)cpu->rbx;
    size_t len      = (size_t)cpu->rcx;
    task_t *current = sched_get_active();

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

    task_t *cur = sched_get_active();
    task_t *t   = sched_task_fork(cur);

    /* forking failed, errno has been set
     * return -1 to parent */
    if (!t)
        return -1;

    /* set the task's state T_READY and schedule it */
    sched_task_set_state(t, T_READY);

    /* return 0 to child, pid to parent */
    t->threads->exec_state->rax = 0;

    return t->pid;
}

int32_t sys_execv(isr_regs_t *cpu)
{
    char *p      = (char *)cpu->rbx;
    file_t *file = NULL;
    path_t *path = NULL;

    if ((path = vfs_path_lookup(p, LOOKUP_OPEN))->p_status != LOOKUP_STAT_SUCCESS) {
        kdebug("Path lookup failed for %s, status %d", p, path->p_status);
        goto error;
    }

    /* kdebug("file open"); */
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
    int status      = cpu->rax;
    task_t *current = sched_get_active();
    task_t *parent  = current->parent;

    kassert(parent != NULL);
    /* kdebug("exiting from %s (pid %d): status %d", current->name, current->pid, status); */

    /* reassign new parent for current task's children */
    if (LIST_EMPTY(current->children) == false) {
        /* kdebug("assign new parent for %s's children", current->name); */
        current->parent = NULL;
    }

    /* free all used memory (all memory that can be freed) */
    vfs_free_fs_ctx(current->fs_ctx);
    vfs_free_file_ctx(current->file_ctx);
    sched_free_threads(current);

    /* Set task's state to T_ZOMBIE and append it to parent's
     * zombie list from where it will be reaped when parent is rescheduled
     * and send a wakeup signal to parent's wait queue */
    sched_task_set_state(current, T_ZOMBIE);
    list_remove(&current->list);
    list_append(&parent->zombies, &current->list);
    wq_wakeup(&parent->wqh_child);

    sched_switch();
    __builtin_unreachable();
}

int32_t sys_wait(isr_regs_t *cpu)
{
    (void)cpu;

    task_t *current = sched_get_active();
    task_t *child   = NULL;

    /* If the task has children, we need to wait until one of the calls sys_exit().
     * Otherwise, we can directly jump to reaping the children because it is possible
     * that the scheduler scheduled child right after fork and execv failed which caused
     * the child to call sys_exit even before we got chance to call sys_wait. */
    if (LIST_EMPTY(current->children) == false) {
        /* wait for one of the children to wake us up so we can reap it */
        wq_wait_event(&current->wqh_child, current, NULL);
    }

    /* One of current's children called exit(),
     * reap all zombies and return back to user land */
    FOREACH(current->zombies, iter) {
        child = container_of(iter, task_t, list);

        kassert(child != NULL);
        kassert(child->threads != NULL);
        kassert(child->threads->state == T_ZOMBIE);

        /* sched_task_destroy() releases all memory the child still has,
         * including kernel stack and page directory, and removes it
         * from all possible lists and finally deallocates the task object */
        sched_task_destroy(child);
    }

    /* TODO: race condition here? */
    list_init(&current->zombies);

    return 0;
}

int32_t sys_socket(isr_regs_t *cpu)
{
    int domain      = (int)cpu->rdx;
    int type        = (int)cpu->rbx;
    int proto       = (size_t)cpu->rcx;
    task_t *current = sched_get_active();

    return socket_alloc(current->file_ctx, domain, type, proto);
}

static syscall_t syscalls[MAX_SYSCALLS] = {
    [0] = sys_read,
    [1] = sys_write,
    [2] = sys_fork,
    [3] = sys_execv,
    [6] = sys_exit,
    [7] = sys_wait,
    [8] = sys_socket,
};

uint32_t syscall_handler(void *ctx)
{
    isr_regs_t *cpu = (isr_regs_t *)ctx;

    if (cpu->rax >= MAX_SYSCALLS) {
        kpanic("unsupported system call");
    } else {
        task_t *current = sched_get_active();
        int32_t ret     = syscalls[cpu->rax](cpu);

        /* return value is transferred in rax */
        current->threads->exec_state->rax = ret;
    }

    return IRQ_HANDLED;
}

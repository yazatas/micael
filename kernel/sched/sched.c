#include <drivers/lapic.h>
#include <drivers/pit.h>
#include <fs/binfmt.h>
#include <fs/file.h>
#include <kernel/common.h>
#include <kernel/gdt.h>
#include <kernel/kassert.h>
#include <kernel/kpanic.h>
#include <kernel/kprint.h>
#include <kernel/percpu.h>
#include <kernel/pic.h>
#include <kernel/tick.h>
#include <kernel/util.h>
#include <mm/heap.h>
#include <sched/mts.h>
#include <sched/sched.h>
#include <errno.h>
#include <stdbool.h>

static unsigned ap_initialized    = 0;
static bool     sched_initialized = false;

/* --------------- idle and init tasks --------------- */
static void *idle_task_func(void *arg)
{
    (void)arg;

    /* kdebug("Starting idle task for CPUID %u...", get_thiscpu_id()); */

    ap_initialized++;

    for (;;) {
        cpu_relax();
    }

    return NULL;
}

/* Init task is only run by the BSP and it does two things:
 *  - wake up Application Processors
 *  - start the /sbin/init task
 *
 * If an AP doesn't start for whatever reason, it is not fatal
 * and the init task will just continue to the next.
 * On the other hand, if /sbin/init cannot be started, that will
 * cause a kernel panic which cannot be recovered from (though
 * it's very unlikely) */
static void *init_task_func(void *arg)
{
    (void)arg;

#if 1
    for (size_t i = 1; i < lapic_get_cpu_count(); ++i) {
        lapic_send_init(i);
        tick_wait(tick_ms_to_ticks(10));
        lapic_send_sipi(i, 0x55);

        kdebug("Waiting for CPU %u to register itself...", i);

        while (READ_ONCE(ap_initialized) != i)
            cpu_relax();
    }
#endif
    sched_initialized = true;

    kdebug("All CPUs initialized");

    file_t *file = NULL;
    path_t *path = NULL;

    if ((path = vfs_path_lookup("/sbin/init", 0))->p_dentry == NULL)
        kpanic("failed to find init script from file system");

    if ((file = file_open(path->p_dentry, O_RDONLY)) == NULL)
        kpanic("failed to open file /sbin/init");

    vfs_path_release(path);

    binfmt_load(file, 0, NULL);

    kpanic("binfmt_load() returned, failed to start init task!");

    return NULL;
}

/* This function gets called when timer interrupt occurs.
 *
 * It saves the state of currently running process and then
 * calls sched_switch() to perform the actual context switch */
static void __prepare_switch(struct isr_regs *cpu_state)
{
    task_t *cur = mts_get_active();

    kassert(cpu_state != NULL);
    kassert(cur       != NULL);

    if (cur) {
        if (get_sp() < (unsigned long)cur->threads->kstack_top)
            kpanic("kernel stack overflow!");

        /* cpu_state now points to the beginning of trap frame, 
         * update exec_state to point to it so next context switch succeeds */
        cur->threads->exec_state = (exec_state_t *)cpu_state;
        cur->threads->exec_state->eflags |= (1 << 9);

        /* all threads get the same amount of execution time (for now) */
        if (cur->nthreads > 1)
            cur->threads = container_of(cur->threads->list.next, thread_t, list);
    }

    sched_switch();
}

void sched_enter_userland(void *eip, void *esp)
{
    task_t *cur = mts_get_active();

    kassert(cur != NULL);

    /* prepare the context for this architecture */
    arch_context_prepare(cur, eip, esp);

    /* update TSS and load the context from 
     * threads->exec_state essentially switching the task */
    tss_update_rsp((unsigned long)cur->threads->kstack_top + KSTACK_SIZE);
    arch_context_load(cur->cr3, &cur->threads->bootstrap);

    kpanic("arch_context_load() returned!");
}

void sched_task_set_state(task_t *task, int state)
{
    if (!task)
        return;

    if ((int)task->threads->state == state)
        return;

    int ret = ST_OK;

    /* moving active or waiting-to-become-active task to a wait queue */
    if (state & (T_BLOCKED | T_ZOMBIE)) {
        if (state == T_BLOCKED) {
            if ((ret = mts_block(task)) < 0)
                kdebug("mts_block() failed, error: %d", ret);
        } else if (state == T_ZOMBIE) {
            if ((ret = mts_unschedule(task)) < 0)
                kdebug("mts_unschedule() failed, error: %d", ret);
        }

        kassert(ret >= 0);

        task->threads->state = state;
    }

    if (state == T_READY) {
        if (task->threads->state == T_BLOCKED) {
            if ((ret = mts_unblock(task)) < 0)
                kdebug("mts_unblock() failed, error: %d", ret);
        } else if (task->threads->state == T_UNSTARTED) {
            if ((ret = mts_schedule(task, 0)) < 0)
                kdebug("mts_schedule() failed, error: %d", ret);
        }

        kassert(ret >= 0);

        if (task->threads->state & (T_BLOCKED | T_RUNNING))
            task->threads->state = T_READY;
    }
}

void sched_switch(void)
{
    task_t *cur  = mts_get_active();
    task_t *next = mts_get_next();

    kassert(cur  != NULL);
    kassert(next != NULL);

    /* Task switch was initiated but it may have just been a resched.
     *
     * Do not load context if task was not changed
     * but return from where we came from [sched_tick()] */
    if (cur == next)
        return;

    /* Switch page directory and update TSS's RSP */
    tss_update_rsp((unsigned long)next->threads->kstack_top + KSTACK_SIZE);
    mmu_switch_ctx(next);

    if (next->threads->state == T_UNSTARTED) {
        next->threads->state = T_RUNNING;
        arch_context_switch_user(&cur->threads->kstack_bottom, next->threads->exec_state);
    } else {
        next->threads->state = T_RUNNING;
        arch_context_switch(&cur->threads->kstack_bottom, next->threads->kstack_bottom);
    }
}

void sched_init_cpu(void)
{
    task_t   *task   = NULL;
    thread_t *thread = NULL;
    
    if ((task = sched_task_create("idle_task")) == NULL)
        kpanic("Failed to create idle task");

    if ((thread = sched_thread_create(idle_task_func, NULL)) == NULL)
        kpanic("Failed to create thread for idle task");

    sched_task_add_thread(task, thread);
    mts_init_cpu(task);
}

void sched_init(void)
{
    task_t   *task   = NULL;
    thread_t *thread = NULL;
    
    if (sched_task_init() < 0)
        kpanic("Failed to inititialize tasks");

    if (mts_init() < 0)
        kpanic("Failed to initialize MTS!");

    /* initialize MTS's per-CPU areas and create idle task */
    sched_init_cpu();

    if ((task = sched_task_create("init_task")) == NULL)
        kpanic("Failed to create init task");

    if ((thread = sched_thread_create(init_task_func, NULL)) == NULL)
        kpanic("failed to create thread for init task");

    sched_task_add_thread(task, thread);

    if (mts_schedule(task, 0) < 0)
        kpanic("Failed to schedule init task!");
}

__noreturn void sched_start()
{
    disable_irq();

    task_t *next = mts_get_next();
    next->threads->state = T_RUNNING;

    /* arch_context_load() loads a new context from cr3/exec_state discarding
     * the current context entirely. Used only for task bootstrapping */
    arch_context_load(next->cr3, next->threads->exec_state);

    kpanic("arch_context_load() returned!");
}

task_t *sched_get_active(void)
{
    if (READ_ONCE(sched_initialized))
        return mts_get_active();
    return NULL;
}

task_t *sched_get_init(void)
{
    return NULL;
    /* return init_task; */
}

void sched_tick(isr_regs_t *cpu)
{
    if (!READ_ONCE(sched_initialized))
        return;

    if (mts_tick() != ST_OK)
        __prepare_switch(cpu);
}

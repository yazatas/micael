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
#include <sched/sched.h>
#include <errno.h>
#include <stdbool.h>

#define TIMESLICE 100

__percpu static list_head_t run_queue;
__percpu static list_head_t wait_queue;
__percpu static task_t *current;
__percpu static task_t *idle_task;

/* only used by the BSP */
static task_t *init_task;

/* defined by the linker */
extern uint8_t _trampoline_start;
extern uint8_t _trampoline_end;

static unsigned ap_initialized    = 0;
static bool     sched_initialized = false;

/* --------------- idle and init tasks --------------- */
static void *idle_task_func(void *arg)
{
    (void)arg;

    kdebug("Starting idle task for CPUID %u...", get_thiscpu_id());

    ap_initialized++;

    for (;;) {
        asm volatile ("pause");
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

    kdebug("in init task!");

    /* Initialize SMP trampoline and wake up all APs one by one
     * The SMP trampoline is located at 0x55000 and the trampoline switches
     * the AP from real mode to protected mode and calls _start (in boot.S)
     *
     * 0x55000 is below 0x100000 so the memory is indetity mapped */
    size_t trmp_size = (size_t)&_trampoline_end - (size_t)&_trampoline_start;
    kmemcpy((uint8_t *)0x55000, &_trampoline_start, trmp_size);

    for (size_t i = 1; i < lapic_get_cpu_count(); ++i) {
        lapic_send_init(i);
        tick_wait(tick_ms_to_ticks(10));
        lapic_send_sipi(i, 0x55);

        kdebug("Waiting for CPU %u to register itself...", i);

        /* bool initialized = READ_ONCE(ap_initialized); */
        while (READ_ONCE(ap_initialized) != i)
            asm volatile ("pause");
    }
    sched_initialized = true;

    kdebug("All CPUs initialized");

    for (;;);

#if 0
    file_t *file   = NULL;
    path_t *path   = NULL;

    if ((path = vfs_path_lookup("/sbin/init", 0))->p_dentry == NULL)
        kpanic("failed to find init script from file system");

    if ((file = file_open(path->p_dentry, O_RDONLY)) == NULL)
        kpanic("failed to open file /sbin/init");

    vfs_path_release(path);

    binfmt_load(file, 0, NULL);

    kpanic("binfmt_load() returned, failed to start init task!");

    return NULL;
#endif
}

/* --------------- /idle and init tasks --------------- */

/* TODO: add comment */
static task_t *__dequeue_task(list_head_t *queue)
{
    if (!queue)
        return NULL;

    task_t *ret = NULL;

    /* queue is empty if it's next elements points to itself */
    if (queue->next != queue) {
        ret = container_of(queue->next, task_t, list);
        queue->next = ret->list.next;

        if (queue->next == queue)
            list_init(queue);
    }

    return ret;
}

/* run_queue has a list of tasks in order of execution ie queue->next
 * points to task that is going to get execution time next.
 *
 * queue->prev points to last element of the list so that new task can be
 * inserted in constant time. Last_task->next points to queue in order to
 * make the list iterable (see sched_print_tasks) */
static void __enqueue_task(list_head_t *queue, task_t *task)
{
    if (!queue || !task)
        return;

    if (queue->next == queue->prev) {
        if (queue->next != queue) {
            queue->prev->next = &task->list;
            task->list.next = queue;
        } else {
            task->list.next = queue;
            queue->next = &task->list;
        }
    } else {
        task_t *last = container_of(queue->prev, task_t, list);
        last->list.next = &task->list;
        task->list.next = queue;
    }

    /* make queue->prev point to last element on the list so
     * future insertions can be done in constant time */
    queue->prev = &task->list;
}

/* this function gets called when timer interrupt occurs. It saves
 * the state of currently running process and then calls sched_switch()
 * to perform the actual context switch */
static void __noreturn __switch(struct isr_regs *cpu_state)
{
    task_t *cur = get_thiscpu_var(current);

    if (cpu_state != NULL) {
        if (get_sp() < (unsigned long)cur->threads->kstack_top) {
            kpanic("kernel stack overflow!");
        }

        /* cpu_state now points to the beginning of trap frame, 
         * update exec_state to point to it so next context switch succeeds */
        cur->threads->exec_state = (exec_state_t *)cpu_state;
        cur->threads->exec_state->eflags |= (1 << 9);

        /* Reschedule this thread by settings its */
        cur->threads->flags         &= ~TIF_NEED_RESCHED;
        cur->threads->total_runtime += cur->threads->exec_runtime;
        cur->threads->exec_runtime   = 0;
    }

    /* all threads get the same amount of execution time (for now) */
    if (cur->nthreads > 1)
        cur->threads = container_of(cur->threads->list.next, thread_t, list);

    put_thiscpu_var(current);
    sched_switch();
}

void __noreturn sched_switch(void)
{
    /* first remove next task from run_queue and then and current queue to run_queue.
     * There's a chance that system has only one task running (very unlikely in the future).
     * If we enqueded this task before dequeuing, the run_queue would fill up with the same task 
     * which is obviously something we don't want */
    task_t *next = __dequeue_task(get_thiscpu_ptr(run_queue));
    task_t *cur  = get_thiscpu_var(current);

    /* current may be zombie if user called sys_exit. Sys_exit releases
     * all memory that can be released, sets the state of its only thread to
     * T_ZOMBIE and then calls sched_switch. These zombie tasks should be
     * scheduled (obviously) */
    if (cur != get_thiscpu_var(idle_task) &&
        current->threads->state  != T_ZOMBIE)
        __enqueue_task(get_thiscpu_ptr(run_queue), cur);

    if ((cur = next) == NULL) {
        /* there was no task in the run_queue, select current task again */
        cur = __dequeue_task(get_thiscpu_ptr(run_queue));

        if (cur == NULL) {
            cur = get_thiscpu_var(idle_task);
            put_thiscpu_var(idle_task);
        }
    }

    /* update tss for this CPU, important for user mode tasks */
    tss_update_rsp((unsigned long)current->threads->kstack_bottom);
    put_thiscpu_var(current);

    context_switch(
        cur->cr3,
        cur->threads->exec_state
    );
    kpanic("context_switch() returned!");
}

void sched_task_schedule(task_t *task)
{
    __enqueue_task(get_thiscpu_ptr(run_queue), task);
}

void sched_init_cpu(void)
{
    thread_t *idle_thread = NULL;

    list_init(get_thiscpu_ptr(run_queue));
    list_init(get_thiscpu_ptr(wait_queue));

    get_thiscpu_var(idle_task) = sched_task_create("idle_task");
    if ((get_thiscpu_var(idle_task)) == NULL)
        kpanic("failed to create idle task");

    if ((idle_thread = sched_thread_create(idle_task_func, NULL)) == NULL)
        kpanic("failed to create thread for idle task");

    sched_task_add_thread(get_thiscpu_var(idle_task), idle_thread);
    get_thiscpu_var(current) = get_thiscpu_var(idle_task);
}

/* initialize idle task and run/wait queues */
void sched_init(void)
{
    thread_t *init_thread = NULL;

    if (sched_task_init() < 0)
        kpanic("failed to inititialize tasks");

    if ((init_task = sched_task_create("init_task")) == NULL)
        kpanic("failed to create init task");

    if ((init_thread = sched_thread_create(init_task_func, NULL)) == NULL)
        kpanic("failed to create thread for init task");

    sched_init_cpu();

    sched_task_add_thread(init_task, init_thread);
    __enqueue_task(get_thiscpu_ptr(run_queue), init_task);
}

void sched_start(void)
{
    disable_irq();
    sched_switch();

    kpanic("sched_switch() returned!");
}

void __noreturn sched_enter_userland(void *eip, void *esp)
{
    disable_irq();

    current->threads->exec_state->eip     = (unsigned long)eip;
    current->threads->exec_state->esp     = (unsigned long)esp;
    current->threads->exec_state->esp     = (unsigned long)esp;
    current->threads->exec_state->eflags |= (1 << 9); /* enable interrupts */

#ifdef __i386__
    current->threads->exec_state->gs = SEG_USER_DATA;
    current->threads->exec_state->fs = SEG_USER_DATA;
    current->threads->exec_state->es = SEG_USER_DATA;
    current->threads->exec_state->ds = SEG_USER_DATA;
#endif

    current->threads->exec_state->cs = SEG_USER_CODE;
    current->threads->exec_state->ss = SEG_USER_DATA;

    context_switch(current->cr3, current->threads->exec_state);
}

task_t *sched_get_current(void)
{
    return current;
}

task_t *sched_get_init(void)
{
    return init_task;
}

void sched_tick(isr_regs_t *cpu)
{
    if (!READ_ONCE(sched_initialized))
        return;

    task_t *t = get_thiscpu_var(current);

    if (++t->threads->exec_runtime > TIMESLICE)
        t->threads->flags |= TIF_NEED_RESCHED;

    put_thiscpu_var(current);

    /* If currently running process has used its timeslice, 
     * TODO */
    if (t->threads->flags & TIF_NEED_RESCHED)
        __switch(cpu);
}

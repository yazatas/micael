#include <drivers/timer.h>
#include <kernel/common.h>
#include <kernel/gdt.h>
#include <kernel/irq.h>
#include <kernel/kpanic.h>
#include <kernel/kprint.h>
#include <mm/heap.h>
#include <sched/sched.h>
#include <errno.h>

static list_head_t run_queue;
static list_head_t wait_queue;

static task_t *idle_task = NULL;
static task_t *init_task = NULL;
static task_t *current   = NULL;

/* defined in arch/i386/switch.s */
extern void __noreturn context_switch(uint32_t, void *);

/* --------------- idle and init tasks --------------- */
static void *idle_task_func(void *arg)
{
    (void)arg;

    kdebug("starting idle task...");

    for (;;) {
        kdebug("in idle task!");

        for (volatile int i = 0; i < 50000000; ++i)
            ;
    }

    return NULL;
}

/* --------------- /idle and init tasks --------------- */

/* TODO: add comment */
static task_t *sched_dequeue_task(list_head_t *queue)
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
static void sched_enqueue_task(list_head_t *queue, task_t *task)
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
static void __noreturn do_context_switch(struct isr_regs *cpu_state)
{
    disable_irq();

    if (cpu_state != NULL) {
        if (get_sp() < (uint32_t)current->threads->kstack_top)
            kpanic("kernel stack overflow!");

        /* because we've installed do_context_switch as timer interrupt routine
         * and this function doesn't return, we must acknowledge the interrupt here */
        irq_ack_interrupt(cpu_state->isr_num);

        /* cpu_state now points to the beginning of trap frame, 
         * update exec_state to point to it so next context switch succeeds */
        current->threads->exec_state = (exec_state_t *)cpu_state;
        current->threads->exec_state->eflags |= (1 << 9);
    }

    /* all threads get the same amount of execution time (for now) */
    if (current->nthreads > 1) {
        current->threads =
            container_of(current->threads->list.next, thread_t, list);
    }

    sched_switch();
}

void __noreturn sched_switch(void)
{
    /* first remove next task from run_queue and then and current queue to run_queue.
     * There's a chance that system has only one task running (very unlikely in the future).
     * If we enqueded this task before dequeuing, the run_queue would fill up with the same task 
     * which is obviously something we don't want */
    task_t *next = sched_dequeue_task(&run_queue);

    /* current may be zombie if user called sys_exit. Sys_exit releases
     * all memory that can be released, sets the state of its only thread to
     * T_ZOMBIE and then calls sched_switch. These zombie tasks should be
     * scheduled (obviously) */
    if (current != idle_task && current->threads->state != T_ZOMBIE)
        sched_enqueue_task(&run_queue, current);

    if ((current = next) == NULL) {
        /* there was no task in the run_queue, select current task again */
        current = sched_dequeue_task(&run_queue);

        if (current == NULL) {
            kdebug("no task to run, choosing idle task");
            current = idle_task;
        }
    }

    /* update tss, important for user mode tasks */
    tss_ptr.esp0 = (uint32_t)current->threads->kstack_bottom;

    context_switch(current->cr3, current->threads->exec_state);
    kpanic("context_switch() returned!");
}

void sched_task_schedule(task_t *task)
{
    sched_enqueue_task(&run_queue, task);
}

/* initialize idle task (and init in the future) and run/wait queues */
void sched_init(void)
{
    if (sched_task_init() < 0)
        kpanic("failed to inititialize tasks");

    if ((idle_task = sched_task_create("idle_task")) == NULL)
        kpanic("failed to create idle task");

    if ((idle_task->threads = sched_thread_create(idle_task_func, NULL)) == NULL)
        kpanic("failed to create thread for idle task");

    list_init(&run_queue);
    list_init(&wait_queue);

    current = idle_task;
}

void sched_start(void)
{
    disable_irq();

    timer_phase(100);
    irq_install_handler(do_context_switch, 0);

    sched_switch();

    kpanic("do_context_switch() returned!");
}

void __noreturn sched_enter_userland(void *eip, void *esp)
{
    disable_irq();

    /* TODO: remove */
    static int index = 0;
    static const char *names[5] = {
        "user_mode1", "user_mode2", "user_mode3",
        "user_mode4", "user_mode5"
    };

    current->threads->exec_state->eip      = (uint32_t)eip;
    current->threads->exec_state->esp      = (uint32_t)esp;
    current->threads->exec_state->useresp  = (uint32_t)esp;
    current->threads->exec_state->eflags  |= (1 << 9); /* enable interrupts */

    current->threads->exec_state->gs = SEG_USER_DATA;
    current->threads->exec_state->fs = SEG_USER_DATA;
    current->threads->exec_state->es = SEG_USER_DATA;
    current->threads->exec_state->ds = SEG_USER_DATA;

    current->threads->exec_state->cs = SEG_USER_CODE;
    current->threads->exec_state->ss = SEG_USER_DATA;

    tss_ptr.esp0 = (uint32_t)current->threads->kstack_bottom;

    /* TODO: remove */
    current->name = names[index++];

    context_switch(current->cr3, current->threads->exec_state);
}

/* TODO: remove this! */
void sched_print_tasks(void)
{
    kprint("-------\nlist of items:\n");
    task_t *task  = NULL;

    for (list_head_t *t = run_queue.next; t != &run_queue; t = t->next) {
        task = container_of(t, task_t, list);
        kprint("\t%s\n", task->name);
    }
    kprint("-------\n");
}

task_t *sched_get_current(void)
{
    return current;
}

task_t *sched_get_init(void)
{
    return init_task;
}

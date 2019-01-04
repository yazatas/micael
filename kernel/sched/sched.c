#include <drivers/timer.h>
#include <kernel/common.h>
#include <kernel/gdt.h>
#include <kernel/irq.h>
#include <kernel/kpanic.h>
#include <kernel/kprint.h>
#include <mm/heap.h>
#include <sched/sched.h>
#include <errno.h>

static task_t *current   = NULL;
static task_t *idle_task = NULL;
static list_head_t queued_tasks;

/* defined in arch/i386/switch.s */
extern void __noreturn context_switch(uint32_t, void *);

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

    if (current->nthreads > 1) {
        current->threads =
            container_of(current->threads->list.next, thread_t, list);
    }

    task_t *running = current,
           *next    = container_of(queued_tasks.next, task_t, list);

    list_append(&queued_tasks, &running->list);
    list_remove(&next->list);
    current = next;

    running->threads->state = T_READY;
    next->threads->state    = T_RUNNING;

    tss_ptr.esp0 = (uint32_t)current->threads->kstack_bottom;

    context_switch(next->cr3, next->threads->exec_state);
    kpanic("context_switch() returned!");
}

void sched_task_schedule(task_t *t)
{
    list_append(&queued_tasks, &t->list);
}

void sched_init(void)
{
    if (sched_task_init() < 0)
        kpanic("failed to inititialize tasks");

    list_init_null(&queued_tasks);

    if ((idle_task = sched_task_create("idle_task")) == NULL)
        kpanic("failed to create idle task");

    if ((idle_task->threads = sched_thread_create(idle_task_func, NULL)) == NULL)
        kpanic("failed to create thread for idle task");

    sched_task_schedule(idle_task);
    current = idle_task;
}

void sched_start(void)
{
    disable_irq();

    timer_phase(100);
    irq_install_handler(do_context_switch, 0);

    do_context_switch(NULL);

    kpanic("do_context_switch() returned!");
}

task_t *sched_get_current(void)
{
    return current;
}

void sched_enter_userland(void *eip, void *esp)
{
    disable_irq();

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

    context_switch(current->cr3, current->threads->exec_state);
}

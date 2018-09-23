#include <errno.h>
#include <stdint.h>

#include <drivers/timer.h>
#include <kernel/irq.h>
#include <kernel/kprint.h>
#include <kernel/kpanic.h>
#include <mm/vmm.h>
#include <sched/proc.h>
#include <sched/sched.h>
#include <sync/mutex.h>


#define SCHED_MAX_TASKS   5
#define SCHED_TIME_SLICE  100 /* 100 milliseconds */

static mutex_t pid_mutex;
static pid_t pid;

static size_t task_ptr = 0;
static size_t task_count = 0;
static pcb_t *tasks[5];
static int timer___ = 0;
static pcb_t *old = NULL;
static pcb_t *new = NULL;

extern void enter_usermode(void);
extern void save_registers(void *);
extern void context_switch(void *, void *);

void test_func(void)
{
    static int i = 0;
    while (1) {
        kprint("hello %d", i++);
        for (volatile int i = 0; i < 10000000; ++i);
    }
}

static void do_context_switch(void)
{
    static int i = 0; 
    kdebug("doing %uth contex switch!", i++);

    while (task_count > 0) {
        old = new;

        if (task_ptr == task_count)
            task_ptr = 0;

        new = tasks[1];

        if (new->state == P_SOMETHING) {
            new->state = P_RUNNING;
            old->state = P_PAUSED;

            kdebug("staring proc %u", new->pid);
            /* save_registers(&old->regs); */

            kdebug("switching to pdir 0x%x", new->page_dir);

            vmm_change_pdir(new->page_dir);

            enter_usermode();
        } else {
            new->state = P_RUNNING;
            old->state = P_PAUSED;
            kdebug("switching from %u to %u", old->pid, new->pid);
            while (1);
            context_switch(&old->regs, &new->regs);
        }
    }

    /* the execution should never get here, it would mean that all
     * tasks (including init) were killed which should happen unless
     * user explicitly wanted that to happen
     *
     * kernel panic occurs in this case (for now at least)
     * TODO: develop some kind of self-healing mechanism */
}

void test__(void)
{
    kdebug("in test");
    new = tasks[task_count-2];
    vmm_change_pdir(new->page_dir);
    enter_usermode();
}

__attribute__((noreturn)) void schedule(void)
{
    pid = 1;
    /* timer_phase(SCHED_TIME_SLICE * 1); */

    old = NULL;
    new = tasks[task_count - 1];
    task_ptr = task_count;

    /* vmm_change_pdir(new->page_dir); */
    /* enter_usermode(); */

	/* irq_install_handler(do_context_switch, 0); */
    kdebug("here");
    test__();
    do_context_switch();

    /* the execution should *never* get here, if it does
     * there's bug somewhere in the code */
    kpanic("do_context_switch() returned!");
    __builtin_unreachable();
}

/* TODO: protect with mutex */
pid_t sched_get_next_pid(void)
{
    return pid++;
}

/* TODO: return error code */
int sched_add_task(pcb_t *task)
{
    if (task_count >= SCHED_MAX_TASKS) {
        kdebug("unable to schedule any more tasks");
        return -ENOSPC;
    }

    tasks[task_count] = task;
    task_ptr++, task_count++;

    return 0;
}

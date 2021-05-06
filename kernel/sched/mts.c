#include <kernel/compiler.h>
#include <kernel/cpu.h>
#include <kernel/kassert.h>
#include <kernel/kpanic.h>
#include <kernel/percpu.h>
#include <kernel/util.h>
#include <lib/bheap.h>
#include <lib/list.h>
#include <mm/heap.h>
#include <mm/slab.h>
#include <sched/mts.h>
#include <sync/spinlock.h>
#include <limits.h>
#include <errno.h>

#define ST_ERROR(err) ((errno = -(err)) ? NULL : NULL);

typedef struct sched_task    sched_task_t;
typedef struct run_queue     run_queue_t;
typedef struct mts_scheduler mts_scheduler_t;
typedef unsigned long        tick_t;

enum SCHED_STATES {
    ST_ACTIVE       = 1 << 1,
    ST_READY        = 1 << 2,
    ST_BLOCKED      = 1 << 3,
    ST_NEED_RESCHED = 1 << 4,
    ST_PREEMPTED    = 1 << 5,
    ST_UNSCHEDULED  = 1 << 6,
};

enum SCHED_PRIOS {
    SP_IDLE        = 0,
    SP_BATCH       = 5,
    SP_BASE        = 7,
    SP_INTERACTIVE = 10,
};

enum SCHED_PRIO_BONUS {
    SPB_BIRTH    =  2,
    SPB_SLEEP    =  3,
    SPB_INACTIVE =  2,
    SPB_ACTIVE   = -2,
};

enum SCHED_TIMESLICES {
    STS_PENALTY = 2,
    STS_BASE    = 10,
    STS_BATCH   = 15,
    STS_NORMAL  = 5,
};

enum SCHED_TASK_TYPES {
    SCHED_NORMAL = 0,  /* foreground/interactive process */
    SCHED_BATCH  = 1,  /* background/batch process */
};

/* runtime heuristics of task */
struct heur {
    size_t blocked;        /* how many times the task has been blocked */
    size_t total_rt;       /* total runtime of process in ticks */
    size_t unused_rt;      /* how much of the timeslice goes unused */

    tick_t total_blocked;  /* how many ticks in total has the task been blocked */
    tick_t total_running;  /* how many ticks in total has the task been running */
    tick_t total_alloc;    /* total amount of ticks allocated for this task */
    tick_t tick;           /* tick counter used to measure time (active/blocked state) */
    tick_t birth;          /* tick count value when this task was created */

    size_t unfair;         /* how many reschedules has gone by with unfair execution times */
    size_t nexec;          /* how many times this task has been selected for execution */
};

/* The entity being scheduled */
struct sched_task {
    int type;            /* type of the task (see SCHED_TYPES) */
    int state;           /* state of the task (see SCHED_STATES) */
    task_t *task;        /* pointer to the actual task struct */
    int prio;            /* priority of the task */
    int sprio;           /* scheduling priority */
    int nice;            /* nice value given by user to mts_schedule() */

    size_t timeslice;    /* time allocated for the task */
    size_t exec_rt;      /* how many ticks have been consumed out of "timeslice" ticks */

    struct heur rt_heur; /* various run time heuristics for dynamic priority adjustment */
    list_head_t list;    /* list used for the wait queue */

    int pid;             /* pid of the task */
};

struct run_queue {
    bheap_t *pqueue;       /* priority queue for tasks */
    list_head_t wait_list; /* list of blocked tasks */
    sched_task_t *active;  /* currently running task */
    task_t *idle;          /* idle task for this run queue, chosen if pqueue is empty */

    tick_t tick;           /* run queue tick counter, used for heuristics */

    size_t ntasks;         /* how many tasks this run queue has */
    size_t nready;         /* how many tasks are waiting in the priority queueu */
    size_t nblocked;       /* how many tasks are waiting in the wait queue */
    size_t nexec;          /* how many task switches in total has happened */
    ssize_t rprio;         /* priority of all ready tasks combined */
    int lowest;            /* current lowest value of scheduling priority */
    bool iactive;          /* set to true when idle task is running */

    spinlock_t lock;       /* lock for this run queueu */
};

struct mts_scheduler {
    size_t ncpu;                /* how many CPUs are active */
    spinlock_t lock;            /* lock for the scheduler */
    run_queue_t *rq[MAX_CPU];   /* pointers to other CPUs' run queues */
    mm_cache_t *st_cache;       /* SLAB cache for sched_task_t allocations */
};

static __percpu run_queue_t rq;
static mts_scheduler_t mts;

static bool __cmp_pld(void *a1, void *a2)
{
    if (!a1 || !a2)
        return false;

    task_t *t1 = (task_t *)a1;
    task_t *t2 = (task_t *)((sched_task_t *)a2)->task;

    return (t1->pid == t2->pid);
}

static inline run_queue_t *__get_rq(unsigned cpu)
{
    run_queue_t *q = get_percpu_ptr(rq, cpu);
    kassert(q != NULL);
    spin_acquire(&q->lock);

    return q;
}

static inline void __put_rq(run_queue_t *q)
{
    kassert(q != NULL);
    spin_release(&q->lock);
}

static int __update_blocked(sched_task_t *t, bool start)
{
    kassert(t != NULL);

    run_queue_t *q = get_thiscpu_ptr(rq);

    if (start) {
        t->rt_heur.blocked++;
        t->rt_heur.tick = q->tick;
    } else {
        t->rt_heur.total_blocked += (q->tick - t->rt_heur.tick);
    }

    return ST_OK;
}

/* NOTE: This is very crude way of recalculating a new priority.
 *
 * There are a few factors considered when calculating the priority:
 *   - Has the task just woken up from sleep?
 *   - What is the utilization rate of task's execution time?
 *   - How much of the total processing time has the task used?
 *   - How long time ago did the task last execute?
 *
 * Tasks that have run a long time are penalized and prevented from
 * running until other tasks have made progress. Both scheduling priority
 * and timeslice are modified based on need. */
static int __adjust_priority(sched_task_t *t)
{
    kassert(t != NULL);
    kassert(t->rt_heur.total_alloc != 0);

    run_queue_t *q = get_thiscpu_ptr(rq);

    if (q->tick - t->rt_heur.birth < 20) {
        t->prio  = SP_BASE   + t->nice;
        t->sprio = SP_BASE   + t->nice;
        q->rprio = q->rprio  + t->prio;

        return ST_OK;
    }

    /* runtime utilization aka how much of the allocated time is spent running */
    int rtu   = ((t->rt_heur.total_running / (float)t->rt_heur.total_alloc) * 100);
    int bonus = t->nice;

    /* Task has just woken up, reward it with a small bump in priority */
    if (t->state == ST_BLOCKED && t->type == SCHED_BATCH)
        bonus += SPB_SLEEP;

    /* If runtime utilization is low and the task has been running
     * for more than three full timeslices, it might be interactive
     * task and a switch to SCHED_NORMAL is made */
    if (rtu <= 40 && t->rt_heur.total_running > 1 * STS_BASE) {
        t->type      = SCHED_NORMAL;
        t->timeslice = STS_NORMAL;
        t->prio      = SP_INTERACTIVE + bonus;
        t->sprio     = SP_INTERACTIVE + bonus;
        q->rprio     = q->rprio       + t->prio;

        return ST_OK;
    }

    /* This is with high confidence a interactive process,
     * check the task runtime heuristics and possibly adjust task's priority
     *
     * For batch processes the priority is calculated as the combination of
     * the actual priority + used execution. The more execution time the task
     * has been given, the lower its priority to prevent starvation of lower-
     * priority processes.
     *
     * There are two priorites associated with each task:
     *   - priority used to calculate tick amount for task
     *   - priority used to balance tasks
     *
     * The first priority is used for calculating the fair share of ticks for each task.
     * It is combination of SP_BATCH and user-specified nice value (which is often 0).
     *
     * The second priority, the dynamic priority, is the one used to balance tasks and
     * share execution time. This priority can take negative values and is used for the
     * priority queue from which tasks are selected.
     *
     * Having two priority levels makes it possible to prevent starvation but penalizing
     * tasks that use too much CPU time and gradually "aging" the lower-priority tasks by
     * lowering the scheduling priority of higher-priority tasks in O(1) time. */
    if (rtu >= 90) {
        float tprio = (float)q->rprio + t->prio;                     /* total priority */
        float p2t   = ((float)(q->tick - t->rt_heur.birth) / tprio); /* priority to ticks */
        float tu    = t->rt_heur.total_running / (p2t * t->prio);    /* tick usage */
        int ret     = ST_OK;

        /* Execution time overuse is more than 3%,
         * penalize task by moving it at the bottom of queue */
        if (tu > 1.03f) {
            t->timeslice = STS_BATCH;
            t->sprio     = q->lowest - 1;
            ret          = ST_SWITCH;
        }

        /* Execution time underuse is more than 3%,
         * boost process priority and adjust timeslice */
        else if (tu < 0.97f) {
            /* This task has not gotten its fair share of execution time (< 97% of it to be
             * exact) and it will be boosted. Also, a new timeslice is calculated for it also
             * but timeslice is used only if it more than STS_BATCH to prevent trashing */
            t->sprio     = (SP_BATCH + t->nice) * 1.10;
            t->timeslice = MAX((int)((p2t * t->prio) - t->rt_heur.total_running), STS_BATCH);
        }

        t->type  = SCHED_BATCH;
        t->prio  = SP_BATCH + t->nice;
        q->rprio = q->rprio + t->prio;

        if (t->sprio < q->lowest)
            q->lowest = t->sprio;

        return ret;
    }

    /* runtime utilization is somewhere between 40% - 90% so it's
     * unclear whether this is a batch or interactive process,
     * just reset the priority with additional bonus (if any) */
    t->timeslice = STS_BASE;
    t->prio      = SP_BASE + bonus;
    t->sprio     = SP_BASE + bonus;
    q->rprio     = q->rprio + t->prio;

    return ST_OK;
}

static int __schedule_task(sched_task_t *t, bool switch_task)
{
    kassert(t != NULL);

    run_queue_t *q = get_percpu_ptr(rq, t->task->cpu);

    /* Do not schedule tasks that have been scheduled already */
    if (t->state == ST_READY)
        return ST_OK;

    /* Before the priority is re-evaluated,
     * we must update relevant runtime heuristics */
    if (t->state == ST_BLOCKED) {
        t->rt_heur.unused_rt += (t->timeslice - t->exec_rt);
        q->nblocked          -= 1;
    }

    /* Runtime statistic are updated only for tasks
     * that are sleeping or have used their timeslice.
     *
     * Preempted tasks shall just continue where they left off */
    if (t->state & (ST_NEED_RESCHED | ST_BLOCKED))
        t->rt_heur.total_running += t->exec_rt;

    if (__adjust_priority(t) == ST_SWITCH)
        switch_task = true;

    if (t->state & (ST_NEED_RESCHED | ST_BLOCKED)) {
        t->rt_heur.total_alloc += t->timeslice;
        t->exec_rt              = 0;
    }

    /* finally set the task's state to read. ST_READY means that the
     * task been scheduled and is waiting to be picked up for execution */
    t->state = ST_READY;

    if (switch_task) {
        q->nready++;
        (void)bh_insert(q->pqueue, t->sprio, t);
        return ST_SWITCH;
    }

    q->rprio -= t->prio;
    return ST_OK;
}

int mts_init(void)
{
    mts.lock = 0;
    mts.ncpu = 0;

    if ((mts.st_cache = mmu_cache_create(sizeof(sched_task_t), MM_NO_FLAGS)) == NULL)
        return ST_ENOMEM;

    return ST_OK;
}

int mts_init_cpu(task_t *idle)
{
    spin_acquire(&mts.lock);

    run_queue_t *q = get_thiscpu_ptr(rq);

    if ((q->pqueue = bh_init(256)) == NULL) {
        spin_release(&mts.lock);
        return ST_ENOMEM;
    }

    list_init(&q->wait_list);

    q->active   = NULL;
    q->idle     = NULL;
    q->tick     = 0;
    q->ntasks   = 0;
    q->nready   = 0;
    q->nblocked = 0;
    q->nexec    = 0;
    q->lowest   = INT_MAX;
    q->idle     = idle;
    q->lock     = 0;

    mts.rq[mts.ncpu++] = get_thiscpu_ptr(rq);

    spin_release(&mts.lock);
    return ST_OK;
}

int mts_schedule(task_t *task, int nice)
{
    if (mts.ncpu == 0 || !task || ABS(nice) > 4)
        return ST_EINVAL;

    spin_acquire(&mts.lock);

    /* By default assign, new processes to the first CPU and
     * spread out the load so that the CPU that has the fewest
     * processes running gets this process */
    run_queue_t *q = mts.rq[0];
    task->cpu      = 0;

    for (size_t i = 1; i < mts.ncpu; ++i) {
        if (mts.rq[i]->ntasks < q->ntasks) {
            /* kprint("\t[%u] select CPU %u instead of %u (%u vs %u)\n", */
            /*         get_thiscpu_id(), i, task->cpu, q->ntasks, mts.rq[i]->ntasks); */
            q = mts.rq[i];
            task->cpu = i;
        }
    }

    sched_task_t *st = mmu_cache_alloc_entry(mts.st_cache, MM_ZERO);
    int ret          = ST_OK;

    if (!st) {
        spin_release(&mts.lock);
        return ST_ENOMEM;
    }

    list_init(&st->list);
    kmemset(&st->rt_heur, 0, sizeof(struct heur));

    st->task      = task;
    st->nice      = nice;
    st->prio      = SP_BASE + SPB_BIRTH + nice;
    st->sprio     = SP_BASE + SPB_BIRTH + nice;
    st->type      = SCHED_BATCH;
    st->state     = ST_ACTIVE;
    st->timeslice = STS_BASE;
    st->exec_rt   = 0;
    st->pid       = task->pid;

    spin_acquire(&q->lock);

    if (q->lowest > st->sprio)
        q->lowest = st->sprio;

    st->rt_heur.birth       = q->tick;
    st->rt_heur.total_alloc = STS_BASE;

    q->ntasks += 1;
    q->nready += 1;
    q->rprio  += st->prio;

    if ((ret = bh_insert(q->pqueue, st->sprio, st)) < 0)
        goto end;

    if (q->active && q->active->sprio < st->sprio && task->cpu != get_thiscpu_id())
        ret = ST_SWITCH;

end:
    spin_release(&mts.lock);
    spin_release(&q->lock);

    return ret;
}

task_t *mts_get_next(void)
{
    run_queue_t *q = get_thiscpu_ptr(rq);

    if (!q)
        return ST_ERROR(ST_EINVAL);

    spin_acquire(&q->lock);

    sched_task_t *st = NULL;
    int prio         = bh_peek_max(q->pqueue);

    /* MTS has not been started or idle task is being
     * run, pop the first task from queue
     *
     * If there are no tasks, return idle task and keep q->active as NULL */
    if (!q->active)
        goto setup_active;

    /* Removed task can be deallocated only here */
    if (q->active->state == ST_UNSCHEDULED) {
        /* TODO: move spinlock to SLAB! */
        spin_acquire(&mts.lock);
        list_remove(&q->active->list);
        mmu_cache_free_entry(mts.st_cache, q->active, 0);
        spin_release(&mts.lock);
        q->active = NULL;
        goto setup_active;
    }
    /* There are two different possible ways this can proceed:
     *
     * 1) There is a higher-priority task waiting
     *     -> reschedule active task and switch to this new task
     *
     * 2) There is a task of same priority waiting
     *     -> check active task's timeslice
     *       if task has time left
     *         -> return active task
     *       else
     *         -> reschedule current task and switch to new task
     *
     * Get the task with highest priority from queue, reschedule active and return */
    if (prio > (int)q->active->sprio) {
        q->active->state |= ST_PREEMPTED;
        goto setup_active;
    }

    /* active task was blocked, we need to switch tasks right away
     * but **not** add active task to priority queue (it's in the wait queue already) */
    if (q->active->state == ST_BLOCKED) {
        q->active = NULL;
        goto setup_active;
    }

    /* There are only tasks with equal or lower priority waiting,
     * check the run time of current task and if it has time left, do nothing */
    if (q->active->exec_rt < q->active->timeslice)
        return q->active->task;

    /* active task needs rescheduling, reschedule it and if there are no higher-priority tasks
     * re-execute, otherwise switch task */
    if (q->active->state & ST_NEED_RESCHED) {
        /* q->active has the highest priority (__schedule_task() might adjust it)
         * so reschedule it and keep executing it (ie. do not move it priority queue
         *
         * mts_get_next() is not, however, the one making the decision about task switch.
         * If __schedule_task() returns ST_OK, we can keep executing this task. If, however,
         * ST_SWITCH is returned, we must switch the active task even if the current task
         * has the highest priority [it might have changed in __adjust_priority()]*/
        if (prio < (int)q->active->sprio) {
            if (__schedule_task(q->active, false) == ST_OK) {
                spin_release(&q->lock);
                return q->active->task;
            }
        }

        goto setup_active;
    }

setup_active:
    /* If the priority queue is empty, we need to select idle task */
    if (prio == INT_MIN)
        goto setup_idle;

    /* Here the actual task switch is done, first we pop the highest
     * priority task from queue, schedule active task (if we have one),
     * set the popped task as active task and updated its runtime heuristics */
    st = bh_remove_max(q->pqueue);
    kassert(st != NULL);

    if (q->active)
        __schedule_task(q->active, true);

    q->active  = st;
    q->iactive = false;
    st->state  = ST_ACTIVE;

    kassert(q->active != NULL);

    /* update relevant runtime heuristics */
    st->rt_heur.tick   = q->tick;
    st->rt_heur.nexec += 1;

    q->nready -= 1;
    q->nexec  += 1;
    q->rprio  -= st->prio;

    spin_release(&q->lock);
    return st->task;

setup_idle:
    /* kdebug("setup idle task"); */
    q->active  = NULL;
    q->iactive = true;

    spin_release(&q->lock);
    return q->idle;
}

task_t *mts_get_active(void)
{
    run_queue_t *q = get_thiscpu_ptr(rq);

    if (!q)
        return ST_ERROR(ST_EINVAL);

    if (!q->active) {
        if (q->iactive)
            return q->idle;
        else {
            kpanic("why is active NULL and iactive false???");
        }
    }

    return q->active->task;
}

int mts_tick(void)
{
    run_queue_t *q = get_thiscpu_ptr(rq);

    if (!q)
        return ST_EINVAL;

    /* Active is NULL, switch tasks immediately */
    if (!q->active) {
        if (!q->iactive)
            return ST_SWITCH;

        /* If the priority queue is not empty, switch task.
         * Otherwise keep executing idle task */
        return (bh_peek_max(q->pqueue) > INT_MIN) ? ST_SWITCH : ST_OK;
    }

    /* each run queue has a tick counter which is updated
     * on every call to mts_tick() and which is used to keep
     * track of various heuristics regarding task execution */
    q->tick++;
    q->active->exec_rt++;

    /* There are three different scenarios:
     *
     * 1) there is a higher-priority task waiting
     *      -> set active state to ST_PREEMPTED and return ST_PREEMPT
     *         to caller to indicate that tasks should be switched
     *
     * 2) active task has used its timeslice
     *      -> set active state to ST_NEED_RESCHED and return ST_RESCHED
     *         to caller to indicate that caller should reschedule active task
     *
     * 3) active task has time left and there are no higher priority tasks waiting
     *      -> continue execution normally
     */
    int prio = bh_peek_max(q->pqueue);

    if (prio > (int)q->active->sprio) {
        /* kdebug("preempt %s", q->active->task->name); */
        q->active->state |= ST_PREEMPTED;
        return ST_SWITCH;
    }

    if (q->active->state == ST_BLOCKED)
        return ST_SWITCH;

    if (q->active->exec_rt >= q->active->timeslice) {
        /* kdebug("%s needs resched", q->active->task->name); */
        q->active->state |= ST_NEED_RESCHED;
        return ST_SWITCH;
    }

    return ST_OK;
}

int mts_unschedule(task_t *task)
{
    run_queue_t *q = get_percpu_ptr(rq, task->cpu);

    if (!task)
        return ST_EINVAL;

    /* MTS has not been started */
    if (!q->active && !q->iactive)
        return ST_OK;

    /* Task to be deleted is active task, release memory and
     * return ST_SWITCH to caller indicating that task must be switched */
    if (q->active->task == task) {
        q->active->state = ST_UNSCHEDULED;
        q->ntasks--;
        return ST_SWITCH;
    }

    spin_acquire(&q->lock);

    /* the task that needs to be blocked is not active task
     * and we must thus find it from the queue */
    sched_task_t *t = bh_remove_pld(q->pqueue, task, __cmp_pld);

    q->ntasks--;
    q->nready--;

    spin_acquire(&mts.lock);
    list_remove(&t->list);
    mmu_cache_free_entry(mts.st_cache, t, 0);
    spin_release(&mts.lock);
    spin_release(&q->lock);

    return ST_OK;
}

int mts_block(task_t *task)
{
    if (!task)
        return ST_EINVAL;

    int ret         = ST_OK;
    run_queue_t *bq = __get_rq(0);
    run_queue_t  *q = task->cpu ? __get_rq(task->cpu) : bq;
    sched_task_t *t = NULL;

    if (q->active && (t = q->active)->task == task) {
        ret = ST_SWITCH;
        goto end;
    }

    /* the task that needs to be blocked is not active task
     * and we must thus find it from the queue */
    t = bh_remove_pld(q->pqueue, task, __cmp_pld);

    if (!t) {
        ret = ST_ENOENT;
        goto end;
    }

end:
    /* Move task from run queue to wait queue
     *
     * The default waiting queue is on CPU 0 because
     * interrupt forwarding does not work at the moment */
    bq->nblocked += 1;
    q->nready    -= 1;
    q->rprio     -= t->prio;
    t->state      = ST_BLOCKED;
    task->cpu     = 0;

    list_init(&t->list);
    list_append(&bq->wait_list, &t->list);
    (void)__update_blocked(t, true);

    kassert(t->task != NULL);

    if (q != bq)
        __put_rq(bq);
    __put_rq(q);

    return ret;
}

int mts_unblock(task_t *task)
{
    if (!task)
        return ST_EINVAL;

    run_queue_t *q = __get_rq(task->cpu);
    int ret        = ST_ENOENT;

    /* MTS has not been started */
    if (!q->active && !q->iactive) {
        ret = ST_OK;
        goto end;
    }

    FOREACH(q->wait_list, iter) {
        sched_task_t *t = container_of(iter, sched_task_t, list);

        if (t->task->pid == task->pid) {
            list_remove(&t->list);
            (void)__update_blocked(t, false);
            (void)__schedule_task(t, true);

            ret = (q->active) ? ((t->sprio > q->active->sprio) ? ST_SWITCH : ST_OK) : ST_OK;
            goto end;
        }
    }

end:
    __put_rq(q);
    return ret;
}

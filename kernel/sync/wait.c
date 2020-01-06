#include <sync/wait.h>
#include <sched/sched.h>
#include <sched/task.h>
#include <errno.h>

int wq_init_head(wait_queue_head_t *head)
{
    if (!head)
        return -EINVAL;

    list_init(&head->list);
    head->lock = 0;

    return 0;
}

int wq_init(wait_queue_t *wq, task_t *task)
{
    if (!wq || !task)
        return -EINVAL;

    list_init(&wq->list);
    wq->task = task;

    return 0;
}

int wq_wakeup(wait_queue_head_t *head)
{
    if (!head)
        return -EINVAL;

    spin_acquire(&head->lock);

    FOREACH(head->list, iter) {
        wait_queue_t *wq = container_of(iter, wait_queue_t, list);
        task_t *task     = container_of(wq, task_t, wq);

        sched_task_set_state(task, T_READY);
        list_remove(&wq->list);
    }

    spin_release(&head->lock);
    return 0;
}

int wq_wait_event(wait_queue_head_t *head, task_t *task, spinlock_t *lock)
{
    if (!head || !task)
        return -EINVAL;

    /* How this works:
     *
     * When we call this function, we may be holding some spinlock
     * that we must release before we call sched_switch() but before that,
     * we must add ourselves to the wait queue head and and put current task
     * to blocked state.
     *
     * When we've initialized ourselves to a blocking state, we release the spinlock
     * (if we're holding one) and call sched_switch() to put current task to sleep
     *
     * Some other task will call wq_wakeup() at some point waking us up from the sleep */
    spin_acquire(&head->lock);
    list_init(&task->wq.list);
    list_init(&head->list);
    list_append(&head->list, &task->wq.list);

    /* move task to a wait queue */
    sched_task_set_state(task, T_BLOCKED);

    /* release a lock if we're holding before we release
     * the wait queue lock and just before sched_switch(),
     * release the wait queue lock so other tasks can wake us up */
    if (lock)
        spin_release(lock);

    spin_release(&head->lock);

    sched_switch();

    /* Before we can return where we came from,
     * we need to reacquire the lock if we were holding it before we got here.
     *
     * Otherwise the state of the caller might get corrupted */
    if (lock)
        spin_acquire(lock);

    return 0;
}

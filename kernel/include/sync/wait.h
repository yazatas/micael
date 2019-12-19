#ifndef __WAIT_H__
#define __WAIT_H__

#include <lib/list.h>
#include <sync/spinlock.h>

typedef struct task task_t;

typedef struct wait_queue {
    task_t *task;     /* pionter task waiting on the queue */
    list_head_t list; /* list of tasks */
} wait_queue_t;

typedef struct wait_queue_head {
    spinlock_t lock;  /* spinlock protecting "wq_list" */
    list_head_t list; /* list of tasks waiting on this queue */
} wait_queue_head_t;

/* Initialize the wait_queue_head_t structure "head"
 *
 * Return 0 on success
 * Return -EINVAL if "head" is NULL */
int wq_init_head(wait_queue_head_t *head);

/* Initialize the wait queue "wq"
 *
 * Return 0 on success
 * Return -EINVAL if "wq" or "task" is NULL */
int wq_init(wait_queue_t *wq, task_t *task);

/* Wake up all tasks waiting on a wait queue "wq"
 * The tasks are removed from the wait queue as they're woken up
 *
 * Return 0 on success
 * Return -EINVAL if "wq" is NULL */
int wq_wakeup(wait_queue_head_t *wq);

/* Add new task "t" to wait queue "head"
 *
 * If "lock" is not NULL, it means that the calling task is holding a lock
 * that must be released before the task is put to sleep
 * In that case, the calling application was doing something that required
 * exclusive access so before wq_wait_event() returns, the lock must be acquired again
 *
 * Return 0 on success
 * Return -EINVAL if "head" or "wq" is NULL */
int wq_wait_event(wait_queue_head_t *head, task_t *t, spinlock_t *lock);

#endif /* __WAIT_H__ */

#ifndef __MTS_H__
#define __MTS_H__

#include <sched/task.h>
#include <errno.h>

enum SCHED_STATUS {
    ST_OK      = 0,
    ST_SWITCH  = 1,
    ST_ENOMEM  = -ENOMEM,
    ST_EINVAL  = -EINVAL,
    ST_ENOSPC  = -ENOSPC,
    ST_ENOENT  = -ENOENT
};

/* Initialize MTS and allocate space for run queueus
 * mts_init() shoul be called only once!
 *
 * Return 0 on success
 * Return -ENOMEM if allocation failed */
int mts_init(void);

/* Each CPU used to execute tasks should call this function
 * as an initialization method before it starts the scheduler
 * to initialize the per-cpu related structures
 *
 * Return ST_OK on success
 * Return ST_ENOMEM if allocating the priority queue failed */
int mts_init_cpu(task_t *idle);

/* mts_tick() is the driving force of MTS. It should be called periodically
 * to update various heuristics of MTS and active task state. This should be
 * called, for example, every time a timer interrupt occurs
 *
 * mts_tick() updates the runtime statistics of active task and checks if there
 * are higher-priority tasks waiting. If active has used its timeslice or there
 * is a higher-priority task waiting, mts_tick() signals to the calling function
 * to call mts_get_next() to do a task switch
 *
 * Return ST_OK if nothing has to be done
 * Return ST_SWITCH if mts_get_next() should be called */
int mts_tick(void);

/* mts_schedule() function is used to add new tasks to scheduler's
 * run queue. This function should **not** be used to manually
 * reschedule tasks, only to add new tasks to scheduler's queue
 *
 * Calling mts_get_next() is not strictly necessary after calling
 * mts_schedule() even if the priority of "task" is higher than
 * active task's priority but is advised. If "task" does not have
 * a higher priority than active task, mts_get_next() just returns
 * active task so calling does not do any harm. If "task" does have
 * higher priority than active task, a task switch is "forced" on
 * next call to mts_tick()
 *
 * Allowed values for "nice" range from -4 to 4. Higher or lower nice value
 * is considered invalid and ST_ENOENT will be returned. The higher the nice
 * value, the higher the priority of the task and the more execution it will
 * be given (but not at the expense of starving other processes).
 *
 * NOTE: Scheduling the same task multiple times is **undefined behaviour**.
 * Only schedule the task once, for each call to mts_schedule() there should
 * be a call to mts_unschedule().
 *
 * Return ST_OK if scheduling succeeded and nothing need to be acted on
 * Return ST_SWITCH if mts_get_next() should be called
 * Return ST_EINVAL if "task" or "nice" is invalid in some way
 * Return ST_ENOMEM if allocation failed */
int mts_schedule(task_t *task, int nice);

/* Unschedule does the opposite of mts_schedule(): it removes a task from
 * MTS's run queue. The run queue of the CPU pointed to by "task" is searched
 * and if it is found, the task is removed from the queue and all underlying
 * memory is released.
 *
 * NOTE: Calling mts_unschedule() on a task also removes all collected
 * heuristics so if the plan is to reschedule this task soon, it is more
 * advisable to call mts_block() instead to preserve collected heuristics
 *
 * NOTE: Even if ST_OK is returned, it is perfectly valid calling
 * mts_get_next() if it makes the calling implementation simpler
 *
 * Return ST_OK on success
 * Return ST_SWITCH if mts_get_next() must be called
 * Return ST_ENOENT if "task" does not exist in the queue */
int mts_unschedule(task_t *task);

/* mts_block() is used to block the execution of a task.
 * In other words, it puts "task" to sleep (ie. moved from
 * run queue to wait queue). Because the active task can be blocked
 * too, it might be necessary to call mts_get_next() after calling
 * mts_block() to select a new active task for execution.
 * This need is signalled via return codes documented below
 *
 * NOTE: Even if ST_OK is returned, it is perfectly valid calling
 * mts_get_next() if it makes the calling implementation simpler
 *
 * Return ST_OK on success
 * Return ST_SWITCH if mts_get_next() must be called
 * Return ST_ENOENT if "task" was not found */
int mts_block(task_t *task);

/* mts_unblock() wakes up a task from sleep and moves it
 * to run queue of the CPU
 *
 * Return ST_OK on success
 * Return ST_ENOENT if "task" was not found */
int mts_unblock(task_t *task);

/* This is the function used to select a task for execution
 * Calling this multiple times in a row does not corrup the
 * internal state. It will always return the task with highest
 * priority or if there are tasks of same priority waiting,
 * the active tasks as long as it has time left
 *
 * NOTE: mts_get_next() returns always something, even if there
 * are no entries in the queue, it will return pointer to idle task.
 * The only time it can return NULL is if mts_init_cpu() has no been
 * called (the state has not been initialized)
 *
 * Return pointer to task structure that should execute next
 * Return NULL if MTS has not been started */
task_t *mts_get_next(void);

/* Get the task struct of currently running proces
 *
 * Return pointer to task on succes
 * Return NULL if MTS has not been started */
task_t *mts_get_active(void);

#endif /* __MTS_H__ */

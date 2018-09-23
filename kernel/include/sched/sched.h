#ifndef __SCHED_H__
#define __SCHED_H__

/* TODO: how to cooperative with task.h */

void schedule(void) __attribute__((noreturn));

/* voluntarily yield execution
 *
 * useful if you know you'll be waiting for I/O or
 * mutex/semaphore you're trying to take is locked */
/* TODO: remove, hopefully not useful */
void yield_tmp(void);

int sched_add_task(pcb_t *task);

#endif /* end of include guard: __SCHED_H__ */

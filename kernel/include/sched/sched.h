#ifndef __SCHED_H__
#define __SCHED_H__

/* TODO: how to cooperative with task.h */

void schedule(void) __attribute__((noreturn));

/* voluntarily yield execution
 *
 * useful if you know you'll be waiting for I/O or
 * mutex/semaphore you're trying to take is locked */
void yield_tmp(void);

#endif /* end of include guard: __SCHED_H__ */

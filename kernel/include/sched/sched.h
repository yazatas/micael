#ifndef __SCHED_H__
#define __SCHED_H__

#include <sched/task.h>

void sched_init(void);
void sched_start(void); // __attribute__((noreturn));
void sched_task_schedule(task_t *t);

#endif /* end of include guard: __SCHED_H__ */

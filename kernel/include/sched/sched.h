#ifndef __SCHED_H__
#define __SCHED_H__

#include <sched/task.h>
#include <kernel/compiler.h>

void sched_init(void);
void sched_start(void) __noreturn;
void sched_task_schedule(task_t *t);

/* TODO: comment */
task_t *sched_get_current(void);
void sched_enter_userland(void *eip, void *esp);

#endif /* end of include guard: __SCHED_H__ */

#ifndef __SCHED_H__
#define __SCHED_H__

#include <sched/task.h>
#include <kernel/compiler.h>

void sched_init(void);
void sched_init_cpu(void);
void sched_suspend(void);
void sched_resume(void);
void sched_start(void) __noreturn;
void sched_switch(void);
void sched_task_schedule(task_t *t);

/* Update the tick count of currently running task */
void sched_tick(isr_regs_t *cpu);

/* Because we're using percpu variables, each sched_get_current()
 * internally calls get_thiscpu_var() which must be coupled with
 * put_thiscpu_var() [ie. sched_put_current()] */
task_t *sched_get_current(void);
void    sched_put_current(void);

/* TODO: comment */
task_t *sched_get_init(void);
void sched_enter_userland(void *eip, void *esp) __noreturn;

/* TODO:  */
void sched_task_set_state(task_t *task, int state);

#endif /* end of include guard: __SCHED_H__ */

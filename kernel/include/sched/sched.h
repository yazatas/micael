#ifndef __SCHED_H__
#define __SCHED_H__

#include <sched/task.h>
#include <kernel/compiler.h>

void sched_init(void);
void sched_init_cpu(void);
void sched_suspend(void);
void sched_resume(void);
void sched_start(void) __noreturn;
void sched_switch(void) __noreturn;
void sched_task_schedule(task_t *t);

/* Update the tick count of currently running task */
void sched_tick(isr_regs_t *cpu);

/* TODO: comment */
task_t *sched_get_current(void);
task_t *sched_get_init(void);
void sched_enter_userland(void *eip, void *esp) __noreturn;

#endif /* end of include guard: __SCHED_H__ */

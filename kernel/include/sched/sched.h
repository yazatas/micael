#ifndef __SCHED_H__
#define __SCHED_H__

#include <kernel/compiler.h>
#include <sched/task.h>

/* Initialize the whole scheduling subsystem
 *
 * This function does not return anything because
 * if it fails, it issues a kernel panic because
 * the kernel cannot operate without the scheduling
 * subsystem */
void sched_init(void);

/* Initialize the per-CPU areas of scheduling subsystem.
 * This mainly includes whatever the actual scheduler needs
 * to initialize in order to operate.
 *
 * This function also doesn't return anything because if the per-CPU
 * areas cannot be initialized, the subsystem cannot function */
void sched_init_cpu(void);

/* Start the scheduler on the calling CPU
 *
 * Once again, this function can only fail fatally
 * or succeed and start the init task for the CPU */
void sched_start(void) __noreturn;

/* Start task pointed to by "eip" and "esp"
 * by switching to user mode.
 *
 * This function is called by the binfmt loaders
 * that are in turn called by sys_execv().
 *
 * This function cannot fail because arch_context_load()
 * does not return but if by some miracle it does return,
 * kernel panic is issued and execution is halted. */
void sched_enter_userland(void *eip, void *esp) __noreturn;

/* TODO: */
void sched_switch(void);

/* Update the runtime heuristics of active process.
 * Internally this calls MTS's tick function and that
 * determines whether the active task should continue executing
 * or a task switch should be made.
 *
 * This function should be called periodically
 * f.ex. from timer interrupt function */
void sched_tick(isr_regs_t *cpu);

/* Change the state of the task
 *
 * This is used to f.ex. to block the task, to zombify it
 * or to wake it up form sleep. */
void sched_task_set_state(task_t *task, int state);

/* Get pointer to currently running task
 *
 * Return pointer to task on success
 * Return NULL if MTS has not been started */
task_t *sched_get_active(void);

/* TODO: remove this? */
task_t *sched_get_init(void);

#endif /* end of include guard: __SCHED_H__ */

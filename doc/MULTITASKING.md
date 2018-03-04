# Multitasking in micael

## Cooperative multitasking

This model shall be legacy in the near future but at the moment it's all micael has in terms of multitasking.

There are few guidelines one must follow when writing tasks:

* If task has an exit point somewhere, call delete_task before exiting the function
   * for every exit points there must be a call to delete_task
* Task must yield the execution periodically
* All tasks must be created in kmain

By adhering to these rules, one can make sure task execution is smooth and micael doesn't crash. As this is kernel space multitasking, there's no supervisor. Please make sure your task is bug free to prevent kernel panics.

### API

`void yield(void)` - stop current task and yield the execution to next task. This function calls `switch_task()`

`void start_tasking(void)` - setup running task and call the first task on the list

`void delete_task(void))` - remove task from the list and call next task

`void create_task(void(*func)(), uint32_t stack_size, const char *name)` - Create new task. Parameters are function pointer to task function, task's stack size and task name.

`extern void switch_task(registers_t *reg_old, registers_t *reg_new)` - save current task's register info to reg_old and load info from reg_new to registers to start execution of next task


## Future
I'm currently in the process of improving multitasking significantly and in the near future micael should have a priority based scheduling and support for processes.

#include <kernel/gdt.h>
#include <sched/task.h>

void arch_context_prepare(task_t *task, void *esp, void *eip)
{
    task->threads->bootstrap.eip     = (unsigned long)eip;
    task->threads->bootstrap.ebp     = (unsigned long)esp;
    task->threads->bootstrap.esp     = (unsigned long)esp;
    task->threads->bootstrap.eflags  = (1 << 9); /* enable interrupts */

    task->threads->bootstrap.gs = SEG_USER_DATA;
    task->threads->bootstrap.fs = SEG_USER_DATA;
    task->threads->bootstrap.es = SEG_USER_DATA;
    task->threads->bootstrap.ds = SEG_USER_DATA;
    task->threads->bootstrap.cs = SEG_USER_CODE;
    task->threads->bootstrap.ss = SEG_USER_DATA;

    task->threads->state        = T_RUNNING;
}

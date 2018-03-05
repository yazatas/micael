#include <kernel/task.h>
#include <kernel/kprint.h>
#include <kernel/mmu.h>
#include <kernel/kpanic.h>

static tcb_t *head = NULL;
static tcb_t *running = NULL;

void start_tasking(void)
{
	running = head;

	tcb_t *tmp = head;
	while (tmp->next)
		tmp = tmp->next;
	tmp->next = head;

	((void (*)())head->regs.eip)();
}

void create_task(void(*func)(), uint32_t stack_size, const char *name)
{
	kdebug("initializing task %s", name);

	tcb_t *tmp = kcalloc(1, sizeof(tcb_t));

	tmp->name        = name;
	tmp->stack_start = kmalloc(stack_size);
	tmp->regs.eip    = (uint32_t)func;
	tmp->regs.esp    = (uint32_t)tmp->stack_start + stack_size - 1;

	/* FIXME: all kernel tasks have the same page directory, is this necessary */
	/* FIXME: it is not */
	asm volatile("movl %%cr3, %%eax \n \
                  movl %%eax, %0 " : "=r" (tmp->regs.cr3));

    asm volatile("pushfl \n \
                  movl (%%esp), %%eax \n \
                  movl %%eax, %0 \n \
                  popfl" : "=r" (tmp->regs.eflags));

	tmp->next = head;
	head = tmp;
}

/* running task has reached its ended (ie. returned),
 * remove the task from task list  */
void delete_task(void)
{
	kdebug("deleting task %s", running->name);

	tcb_t *prev, *tmp;
	prev = tmp = running;
	while (prev->next != running)
		prev = prev->next;

	prev->next = running->next;
	running = prev->next;

	kfree(tmp->stack_start);
	kfree(tmp);

	yield();
}

void yield(void)
{
	tcb_t *prev = running;
	running = running->next;

	kdebug("switching from %s to %s", prev->name, running->name);
	switch_task(&prev->regs, &running->regs);
}

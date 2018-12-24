#include <kernel/kprint.h>
#include <kernel/kpanic.h>

#include <sched/kthread.h>

#include <mm/mmu.h>
#include <mm/heap.h>

/* defined in boot.s */
extern uint32_t boot_page_dir;

static tcb_t *head = NULL;
static tcb_t *running = NULL;

#define REG(reg) reg, reg

static void kthread_dump_info(tcb_t *t)
{
	kdebug("%s (0x%x) %s:", t->name, (uint32_t)t->stack_start,  t->next->name);
	kprint("\teax: 0x%08x %8u\n\tebx: 0x%08x %8u\n\tecx: 0x%08x %8u\n"
           "\tedx: 0x%08x %8u\n\tedi: 0x%08x %8u\n"
           "\tesp: 0x%08x %8u\n\tebp: 0x%08x %8u\n"
           "\teip: 0x%08x %8u\n\tflg: 0x%08x %8u\n",
		   REG(t->regs.eax), REG(t->regs.ebx), REG(t->regs.ecx), 
		   REG(t->regs.edx), REG(t->regs.edi), REG(t->regs.esp),
		   REG(t->regs.ebp), REG(t->regs.eip), REG(t->regs.eflags));
}

void kthread_start(void)
{
	running = head;

	tcb_t *tmp = head;
	while (tmp->next)
		tmp = tmp->next;
	tmp->next = head;

	((void (*)())head->regs.eip)();
}

void kthread_create(void(*func)(), uint32_t stack_size, const char *name)
{
	kdebug("initializing task %s", name);

	tcb_t *tmp = kcalloc(1, sizeof(tcb_t));

	tmp->name        = name;
	tmp->stack_start = kmalloc(stack_size);
	tmp->regs.eip    = (uint32_t)func;
	tmp->regs.esp    = (uint32_t)tmp->stack_start + stack_size - 1;
	tmp->regs.cr3    = ((uint32_t)&boot_page_dir - 0xc0000000);

    asm volatile("pushfl \n \
                  movl (%%esp), %%eax \n \
                  movl %%eax, %0 \n \
                  popfl" : "=r" (tmp->regs.eflags));

	tmp->next = head;
	head = tmp;
}

/* running task has reached its ended (ie. returned),
 * remove the task from task list  */
/* FIXME: this is probably a wrong solution to the problem */
void kthread_delete(void)
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

	kthread_yield();
}

void kthread_yield(void)
{
	tcb_t *prev = running;
	running = running->next;

	kdebug("switching from %s to %s", prev->name, running->name);
	context_switch(&prev->regs, &running->regs);
}

#include <kernel/task.h>
#include <kernel/kprint.h>
#include <kernel/mmu.h>

#include <string.h>

static task_t *running_task;
static task_t main_task;
static task_t other_task;

static void other_main(void)
{
    kdebug("hello multitasking world!\n");
	kdebug("calculating first 10 fibonacci numbers...\n");

	int n = 0, k = 1, i = 0;

	for (; i < 10; ++i) {
		kdebug("%d\n", n + k);
		int tmp = n;
		n = n + k;
		k = tmp;
	}

    yield();
}

void init_tasking()
{
    asm volatile("movl %%cr3, %%eax \n \
                  movl %%eax, %0 " : "=r" (main_task.regs.cr3));

    asm volatile("pushfl \n \
                  movl (%%esp), %%eax \n \
                  movl %%eax, %0 \n \
                  popfl" : "=r" (main_task.regs.eflags));

    create_task(&other_task, other_main, main_task.regs.eflags, main_task.regs.cr3);
    main_task.next  = (task_t*)&other_task;
    other_task.next = &main_task;

    running_task = &main_task;
}


void create_task(task_t *task, void(*func)(), uint32_t flags, uint32_t page_dir)
{
	memset(task, 0, sizeof(task_t));

    task->regs.eflags = flags;
    task->regs.eip    = (uint32_t)func;
    task->regs.cr3    = (uint32_t)page_dir;
    task->regs.esp    = (uint32_t)kalloc_frame() + 0x1000;

	kdebug("creating task... eip (0x%x) cr3 (0x%x) esp (0x%x)\n",
			task->regs.eip, task->regs.cr3, task->regs.esp);
}

void yield(void)
{
    task_t *last = running_task;
    running_task = running_task->next;

	while (1) {}

	kdebug("switching tasks...\n");
    switch_task(&last->regs, &running_task->regs);

	/* kdebug("eax 0x%x ebx 0x%x ecx 0x%x edx 0x%x\n" */
	/*  "        esi 0x%x edi 0x%x eflags 0x%x\n" */
	/*  "        eip 0x%x cr3 0x%x esp 0x%x\n", */
	/* last->regs.eax, last->regs.ebx, last->regs.ecx, last->regs.edx, last->regs.esi, last->regs.edi, last->regs.eflags, last->regs.eip, last->regs.cr3, last->regs.esp); */

	/* kdebug("eax 0x%x ebx 0x%x ecx 0x%x edx 0x%x\n" */
	/*  "        esi 0x%x edi 0x%x eflags 0x%x\n" */
	/*  "        eip 0x%x cr3 0x%x esp 0x%x\n", */
	/* running_task->regs.eax, running_task->regs.ebx, running_task->regs.ecx, running_task->regs.edx, running_task->regs.esi, running_task->regs.edi, running_task->regs.eflags, running_task->regs.eip, running_task->regs.cr3, running_task->regs.esp); */

}

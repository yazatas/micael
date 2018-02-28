#include <kernel/task.h>
#include <kernel/kprint.h>
#include <kernel/mmu.h>

#include <string.h>

static task_t *running_task;
static task_t main_task;
static task_t other_task;

static void other_main(void)
{
    kdebug("hello multitasking world!");
	kdebug("calculating first 10 fibonacci numbers...");

	int n = 0, k = 1, i = 0;

	for (; i < 10; ++i) {
		kprint("%d ", n + k);
		int tmp = n;
		n = n + k;
		k = tmp;
	}
	kprint("\n");

	kdebug("yielding cpu...");
    yield();

	kdebug("im back! calculating the factorial of 5");

	n = 1;
	for (int i = 1; i <= 5; ++i) {
		n *= i;
	}

	kdebug("factorial of 5 is %d", n);
	kdebug("this function has reached its end");
	yield();
}

void init_tasking(void)
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
    task->regs.esp    = (uint32_t)0xe0000000 + 0x900; /* TODO: use kheap */

	kdebug("creating task... eip (0x%x) cr3 (0x%x) esp (0x%x)",
			task->regs.eip, task->regs.cr3, task->regs.esp);

	map_page((void*)kalloc_frame(), (void*)0xe0000000, P_PRESENT | P_READWRITE);
}

void yield(void)
{
    task_t *last = running_task;
    running_task = running_task->next;

	kdebug("switching tasks...");
	switch_task(&last->regs, &running_task->regs);
}

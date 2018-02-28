#ifndef __TASK_H__
#define __TASK_H__

#include <stdint.h>

typedef struct {
	uint32_t eax, ebx, ecx, edx, esi, edi;
	uint32_t esp, ebp, eip, eflags, cr3;
} registers_t;

typedef struct task {
	registers_t regs;
	struct task *next;
} task_t;

void yield(void);
void init_tasking(void);
void create_task(task_t *task, void(*func)(), uint32_t eflags, uint32_t pagedir);

/* defined in kernel/arch/i386/switch.s */
extern void switch_task(registers_t *reg_old, registers_t *reg_new);

#endif /* end of include guard: __TASK_H__ */

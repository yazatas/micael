#ifndef __TASK_H__
#define __TASK_H__

#include <stdint.h>

typedef struct {
	uint32_t eax, ebx, ecx, edx, esi, edi;
	uint32_t esp, ebp, eip, eflags, cr3;
} registers_t;

typedef struct tcb {
	registers_t regs;
	struct tcb *next;
	const char *name;
	uint32_t *stack_start;
} tcb_t;

void yield(void);
void start_tasking(void);
void delete_task(void) __attribute__((noreturn));
void create_task(void(*func)(), uint32_t stack_size, const char *name);

/* defined in kernel/arch/i386/switch.s */
extern void switch_task(registers_t *reg_old, registers_t *reg_new);

#endif /* end of include guard: __TASK_H__ */

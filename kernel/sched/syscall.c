#include <sched/syscall.h>
#include <kernel/kprint.h>
#include <kernel/kpanic.h>

#define MAX_SYSCALLS 1
#define SYSCALL_NAME(name) sys_##name
#define DEFINE_SYSCALL(name) uint32_t SYSCALL_NAME(name)(isr_regs_t *cpu)

typedef uint32_t (*syscall_t)(isr_regs_t *cpu);

DEFINE_SYSCALL(print_to_screen)
{
	char ptr = *(char*)cpu->ebx;
	kprint("%c ", ptr);
	return 0;
}

static syscall_t syscalls[MAX_SYSCALLS] = {
	[0] = SYSCALL_NAME(print_to_screen),
};

void syscall_handler(isr_regs_t *cpu)
{
	if (cpu->eax >= MAX_SYSCALLS) {
		kpanic("unsupported system call");
	} else {
		uint32_t ret = syscalls[cpu->eax](cpu);
	}
}

#include <sched/syscall.h>
#include <kernel/kprint.h>
#include <kernel/kpanic.h>

#define MAX_SYSCALLS 2
#define SYSCALL_NAME(name) sys_##name
#define DEFINE_SYSCALL(name) uint32_t SYSCALL_NAME(name)(isr_regs_t *cpu)

typedef uint32_t (*syscall_t)(isr_regs_t *cpu);

DEFINE_SYSCALL(read)
{
	int fd = cpu->ecx;
}

DEFINE_SYSCALL(write)
{
	char *ptr = (char*)cpu->ebx;
	for (size_t i = 0; i < cpu->ecx; ++i)
		kprint("%c", ptr[i]);
	return 0;
}

static syscall_t syscalls[MAX_SYSCALLS] = {
	[0] = SYSCALL_NAME(read),
	[1] = SYSCALL_NAME(write),
};

void syscall_handler(isr_regs_t *cpu)
{
	if (cpu->eax >= MAX_SYSCALLS) {
		kpanic("unsupported system call");
	} else {
		uint32_t ret = syscalls[cpu->eax](cpu);
	}
}

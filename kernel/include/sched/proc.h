#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <stdint.h>

typedef int8_t pid_t;

enum {
    P_RUNNING,
    P_PAUSED,
    P_SOMETHING /* FIXME:  */
};

typedef struct pcb {
	void *page_dir;
	pid_t pid;
    uint8_t state; /* running, paused, not started */

	struct registers {
		uint32_t eax, ebx, ecx, edx, esi, edi;
		uint32_t esp, ebp, eip, eflags, cr3;
	} regs;
} pcb_t;

void tasking_init(void);
pcb_t *process_create_remove(const char *file);
pcb_t *process_create(uint8_t *file, size_t len);

#endif /* end of include guard: __PROCESS_H__ */

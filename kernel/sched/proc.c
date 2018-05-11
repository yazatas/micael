#include <kernel/kprint.h>
#include <kernel/compiler.h>
#include <kernel/output.h>
#include <mm/vmm.h>
#include <mm/kheap.h>
#include <sched/proc.h>
#include <sync/mutex.h>
#include <string.h>

extern void enter_usermode();

/* const uint8_t program_code[] = { */
/* 	0x54, 0x55, 0x50, 0x53, 0x51, 0xe8, 0x03, 0x00, 0x00, 0x00, 0xeb, 0xfe, 0xc3, 0x8d, 0x4c, 0x24, */
/* 	0x04, 0x83, 0xe4, 0xf0, 0xff, 0x71, 0xfc, 0x55, 0x89, 0xe5, 0x51, 0x83, 0xec, 0x14, 0xc6, 0x45, */
/* 	0xf7, 0x61, 0xeb, 0x1a, 0x0f, 0xbe, 0x45, 0xf7, 0x83, 0xec, 0x0c, 0x50, 0xe8, 0x20, 0x00, 0x00, */
/* 	0x00, 0x83, 0xc4, 0x10, 0x0f, 0xb6, 0x45, 0xf7, 0x83, 0xc0, 0x01, 0x88, 0x45, 0xf7, 0x80, 0x7d, */
/* 	0xf7, 0x7a, 0x7e, 0xe0, 0xb8, 0xee, 0xdb, 0xea, 0x0d, 0x8b, 0x4d, 0xfc, 0xc9, 0x8d, 0x61, 0xfc, */
/* 	0xc3, 0x55, 0x89, 0xe5, 0x83, 0xec, 0x10, 0x8b, 0x45, 0x08, 0x88, 0x45, 0xff, 0x8d, 0x45, 0xff, */
/* 	0x89, 0x45, 0xf8, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x8b, 0x5d, 0xf8, 0xb9, 0x01, 0x00, 0x00, 0x00, */
/* 	0xcd, 0x80, 0x8b, 0x45, 0x08, 0xc9, 0xc3, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, */
/* 	0x01, 0x7a, 0x52, 0x00, 0x01, 0x7c, 0x08, 0x01, 0x1b, 0x0c, 0x04, 0x04, 0x88, 0x01, 0x00, 0x00, */
/* 	0x28, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x75, 0xff, 0xff, 0xff, 0x44, 0x00, 0x00, 0x00, */
/* 	0x00, 0x44, 0x0c, 0x01, 0x00, 0x47, 0x10, 0x05, 0x02, 0x75, 0x00, 0x43, 0x0f, 0x03, 0x75, 0x7c, */
/* 	0x06, 0x71, 0x0c, 0x01, 0x00, 0x41, 0xc5, 0x43, 0x0c, 0x04, 0x04, 0x00, 0x14, 0x00, 0x00, 0x00, */
/* 	0x00, 0x00, 0x00, 0x00, 0x01, 0x7a, 0x52, 0x00, 0x01, 0x7c, 0x08, 0x01, 0x1b, 0x0c, 0x04, 0x04, */
/* 	0x88, 0x01, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x75, 0xff, 0xff, 0xff, */
/* 	0x26, 0x00, 0x00, 0x00, 0x00, 0x41, 0x0e, 0x08, 0x85, 0x02, 0x42, 0x0d, 0x05, 0x62, 0xc5, 0x0c, */
/* 	0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, */
/* }; */

/* TODO: use vmm_kalloc_frame, this is not a solution in the long run */
static pdir_t PDIR_V[1024]      __align_4k;
static ptbl_t code_pt[1024]     __align_4k;
static ptbl_t stack_pt[1024]    __align_4k;
static uint8_t page_tmp[0x1000] __align_4k;

pcb_t *process_create(const char *file)
{
	kdebug("allocating process control block...");
	pcb_t *p = kmalloc(sizeof(pcb_t));
	p->pid   = 77;

	kdebug("initializing page tables for process...");
	p->page_dir = vmm_v_to_p(PDIR_V);

	memcpy(page_tmp, program_code, sizeof(program_code));

	code_pt[0]     = ((uint32_t)vmm_v_to_p(page_tmp)) | P_PRESENT | P_READWRITE | P_USER;
	stack_pt[1023] = vmm_kalloc_frame()               | P_PRESENT | P_READWRITE | P_USER;

	PDIR_V[0]           = ((uint32_t)vmm_v_to_p(&code_pt))  | P_PRESENT | P_READWRITE | P_USER;
	PDIR_V[KSTART - 1]  = ((uint32_t)vmm_v_to_p(&stack_pt)) | P_PRESENT | P_READWRITE | P_USER;
	PDIR_V[KSTART]      = ((uint32_t)vmm_get_kernel_pdir()) | P_PRESENT | P_READWRITE;
	PDIR_V[KSTART_HEAP] = ((uint32_t)vmm_get_kheap_pdir())  | P_PRESENT | P_READWRITE; /* TODO: remove */
	PDIR_V[1023]        = ((uint32_t)p->page_dir)           | P_PRESENT | P_READONLY;

	/* TODO: where should the scheduler be called? */

	kdebug("switching page directories...");
	vmm_change_pdir(p->page_dir);

	kdebug("switching to ring 3...");
	enter_usermode();

	return p;
}

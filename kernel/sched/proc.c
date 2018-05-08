#include <sched/proc.h>
#include <kernel/kprint.h>
/* #include <kernel/kernel/program.h> */

#include <mm/vmm.h>
#include <mm/kheap.h>

extern void enter_usermode();

const uint8_t program_code[] = {
	0x31, 0xc0, 0x31, 0xdb, 0x31,
	0xc9, 0xeb, 0xf8, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00
};

static pdir_t PDIR_V[1024]      __attribute__((aligned(4096)));
static ptbl_t code_pt[1024]     __attribute__((aligned(4096)));
static ptbl_t stack_pt[1024]    __attribute__((aligned(4096)));
static uint8_t page_tmp[0x1000] __attribute__((aligned(4096)));

pcb_t *process_create(const char *file)
{
	kdebug("allocating process control block...");
	pcb_t *p = kmalloc(sizeof(pcb_t));
	p->pid   = 77;

	kdebug("initializing page tables for process...");
	p->page_dir = vmm_v_to_p(PDIR_V);

	memcpy(page_tmp, program_code, sizeof(program_code));

	code_pt[0]     = ((uint32_t)vmm_v_to_p(page_tmp)) | P_PRESENT | P_READWRITE | P_USER;
	stack_pt[1023] = vmm_kalloc_frame() | P_PRESENT | P_READWRITE;// | P_USER;

	PDIR_V[0]           = ((uint32_t)vmm_v_to_p(&code_pt))  | P_PRESENT | P_READWRITE | P_USER;
	PDIR_V[KSTART - 1]  = ((uint32_t)vmm_v_to_p(&stack_pt)) | P_PRESENT | P_READWRITE;// | P_USER;
	PDIR_V[KSTART]      = ((uint32_t)vmm_get_kernel_pdir()) | P_PRESENT | P_READWRITE | P_USER;
	PDIR_V[KSTART_HEAP] = ((uint32_t)vmm_get_kheap_pdir())  | P_PRESENT | P_READWRITE | P_USER;
	PDIR_V[1023]        = ((uint32_t)p->page_dir)           | P_PRESENT | P_READONLY  | P_USER;

	kprint("changing page directory...");
	vmm_change_pdir(p->page_dir);
	kprint("... changed!\n");

	kdebug("switching to ring 3...");
	enter_usermode();

	return p;
}

#include <kernel/kprint.h>
#include <kernel/compiler.h>
#include <mm/vmm.h>
#include <mm/kheap.h>
#include <sched/proc.h>
#include <sched/sched.h>
#include <sync/mutex.h>
#include <string.h>

extern void enter_usermode();

static pid_t get_next_pid(void)
{
    static pid_t pid = 0;
    return pid++;
}

pcb_t *process_create_remove(const char *file)
{
    kdebug("allocating process control block for %s...", file);
    pcb_t *p = kmalloc(sizeof(pcb_t));
    p->pid   = get_next_pid();

    kdebug("initializing page tables for process...");
    uint32_t *PDIR_V        = vmm_kalloc_mapped_page(P_PRESENT | P_READWRITE),
             *stack_pt_virt = vmm_kalloc_mapped_page(P_PRESENT | P_READWRITE),
             *code_pt_virt  = vmm_kalloc_mapped_page(P_PRESENT | P_READWRITE),
             *code_pte_virt = vmm_kalloc_mapped_page(P_PRESENT | P_READWRITE);

    memset(code_pt_virt, 0xff, 0x200);

    code_pt_virt[0]     = ((uint32_t)vmm_v_to_p(code_pte_virt)) | P_READWRITE | P_USER | P_PRESENT;
    stack_pt_virt[1023] = vmm_kalloc_frame()                    | P_READWRITE | P_USER | P_PRESENT;

    PDIR_V[0]           = ((uint32_t)vmm_v_to_p(code_pt_virt))  | P_PRESENT | P_READWRITE | P_USER;
    PDIR_V[KSTART - 1]  = ((uint32_t)vmm_v_to_p(stack_pt_virt)) | P_PRESENT | P_READWRITE | P_USER;
    PDIR_V[KSTART]      = ((uint32_t)vmm_get_kernel_pdir())     | P_PRESENT | P_READWRITE;
    PDIR_V[KSTART_HEAP] = ((uint32_t)vmm_get_kheap_pdir())      | P_PRESENT | P_READWRITE; /* TODO: remove */
    PDIR_V[1023]        = ((uint32_t)vmm_v_to_p(PDIR_V))        | P_PRESENT | P_READONLY;

    p->page_dir = vmm_v_to_p(PDIR_V);

    vmm_free_tmp_vpage(PDIR_V);
    vmm_free_tmp_vpage(stack_pt_virt);
    vmm_free_tmp_vpage(code_pt_virt);
    vmm_free_tmp_vpage(code_pte_virt);

    sched_add_task(p);

    kdebug("switching page directories...");
    vmm_change_pdir(p->page_dir);

    kdebug("switching to ring 3...");
    enter_usermode();

    return p;
}

/* TODO: refactor this so that it doesn't automatically start 
 * but is instead  */
pcb_t *process_create(uint8_t *memory, size_t len)
{
    pcb_t *p = kmalloc(sizeof(pcb_t));
    p->pid   = get_next_pid();

    kdebug("initializing page tables for process...");
    uint32_t *PDIR_V        = vmm_kalloc_mapped_page(P_PRESENT | P_READWRITE),
             *stack_pt_virt = vmm_kalloc_mapped_page(P_PRESENT | P_READWRITE),
             *code_pt_virt  = vmm_kalloc_mapped_page(P_PRESENT | P_READWRITE),
             *code_pte_virt = vmm_kalloc_mapped_page(P_PRESENT | P_READWRITE);

    memcpy(code_pte_virt, memory, len);

    code_pt_virt[0]     = ((uint32_t)vmm_v_to_p(code_pte_virt)) | P_READWRITE | P_USER | P_PRESENT;
    stack_pt_virt[1023] = vmm_kalloc_frame()                    | P_READWRITE | P_USER | P_PRESENT;

    PDIR_V[0]           = ((uint32_t)vmm_v_to_p(code_pt_virt))  | P_PRESENT | P_READWRITE | P_USER;
    PDIR_V[KSTART - 1]  = ((uint32_t)vmm_v_to_p(stack_pt_virt)) | P_PRESENT | P_READWRITE | P_USER;
    PDIR_V[KSTART]      = ((uint32_t)vmm_get_kernel_pdir())     | P_PRESENT | P_READWRITE;
    PDIR_V[KSTART_HEAP] = ((uint32_t)vmm_get_kheap_pdir())      | P_PRESENT | P_READWRITE; /* TODO: remove */
    PDIR_V[1023]        = ((uint32_t)vmm_v_to_p(PDIR_V))        | P_PRESENT | P_READONLY;

    p->page_dir = vmm_v_to_p(PDIR_V);
    p->state    = P_SOMETHING;

    kdebug("proc %u pdir address 0x%x", p->pid, p->page_dir);

    vmm_free_tmp_vpage(PDIR_V);
    vmm_free_tmp_vpage(stack_pt_virt);
    vmm_free_tmp_vpage(code_pt_virt);
    vmm_free_tmp_vpage(code_pte_virt);

    sched_add_task(p);

    return p;
}

#include <kernel/kprint.h>
#include <kernel/compiler.h>
#include <mm/vmm.h>
#include <mm/kheap.h>
#include <sched/proc.h>
#include <sync/mutex.h>
#include <string.h>

extern void enter_usermode();

const uint8_t program_code[] = {
 0x54, 0x55, 0x50, 0x53, 0x51, 0xe8, 0x03, 0x00, 0x00, 0x00, 0xeb, 0xfe, 0xc3, 0x8d, 0x4c, 0x24,
 0x04, 0x83, 0xe4, 0xf0, 0xff, 0x71, 0xfc, 0x55, 0x89, 0xe5, 0x51, 0x83, 0xec, 0x14, 0xc6, 0x45,
 0xf7, 0x61, 0xeb, 0x1a, 0x0f, 0xbe, 0x45, 0xf7, 0x83, 0xec, 0x0c, 0x50, 0xe8, 0x20, 0x00, 0x00,
 0x00, 0x83, 0xc4, 0x10, 0x0f, 0xb6, 0x45, 0xf7, 0x83, 0xc0, 0x01, 0x88, 0x45, 0xf7, 0x80, 0x7d,
 0xf7, 0x7a, 0x7e, 0xe0, 0xb8, 0xee, 0xdb, 0xea, 0x0d, 0x8b, 0x4d, 0xfc, 0xc9, 0x8d, 0x61, 0xfc,
 0xc3, 0x55, 0x89, 0xe5, 0x83, 0xec, 0x10, 0x8b, 0x45, 0x08, 0x88, 0x45, 0xff, 0x8d, 0x45, 0xff,
 0x89, 0x45, 0xf8, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x8b, 0x5d, 0xf8, 0xb9, 0x01, 0x00, 0x00, 0x00,
 0xcd, 0x80, 0x8b, 0x45, 0x08, 0xc9, 0xc3, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x01, 0x7a, 0x52, 0x00, 0x01, 0x7c, 0x08, 0x01, 0x1b, 0x0c, 0x04, 0x04, 0x88, 0x01, 0x00, 0x00,
 0x28, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x75, 0xff, 0xff, 0xff, 0x44, 0x00, 0x00, 0x00,
 0x00, 0x44, 0x0c, 0x01, 0x00, 0x47, 0x10, 0x05, 0x02, 0x75, 0x00, 0x43, 0x0f, 0x03, 0x75, 0x7c,
 0x06, 0x71, 0x0c, 0x01, 0x00, 0x41, 0xc5, 0x43, 0x0c, 0x04, 0x04, 0x00, 0x14, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x01, 0x7a, 0x52, 0x00, 0x01, 0x7c, 0x08, 0x01, 0x1b, 0x0c, 0x04, 0x04,
 0x88, 0x01, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x75, 0xff, 0xff, 0xff,
 0x26, 0x00, 0x00, 0x00, 0x00, 0x41, 0x0e, 0x08, 0x85, 0x02, 0x42, 0x0d, 0x05, 0x62, 0xc5, 0x0c,
 0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static pid_t get_next_pid(void)
{
    static pid_t pid = 0;
    return pid++;
}

pcb_t *process_create(const char *file)
{
    kdebug("allocating process control block for %s...", file);
    pcb_t *p = kmalloc(sizeof(pcb_t));
    p->pid   = get_next_pid();

    kdebug("initializing page tables for process...");
    uint32_t *PDIR_V        = vmm_kalloc_mapped_page(P_PRESENT | P_READWRITE),
             *stack_pt_virt = vmm_kalloc_mapped_page(P_PRESENT | P_READWRITE),
             *code_pt_virt  = vmm_kalloc_mapped_page(P_PRESENT | P_READWRITE),
             *code_pte_virt = vmm_kalloc_mapped_page(P_PRESENT | P_READWRITE);

    memcpy(code_pte_virt, program_code, sizeof(program_code));

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

    kdebug("switching page directories...");
    vmm_change_pdir(p->page_dir);

    kdebug("switching to ring 3...");
    enter_usermode();

    return p;
}

pcb_t *process_create_bin(uint8_t *memory, size_t len)
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

    vmm_free_tmp_vpage(PDIR_V);
    vmm_free_tmp_vpage(stack_pt_virt);
    vmm_free_tmp_vpage(code_pt_virt);
    vmm_free_tmp_vpage(code_pte_virt);

    kdebug("switching page directories...");
    vmm_change_pdir(p->page_dir);

    kdebug("switching to ring 3...");
    enter_usermode();

    return p;
}

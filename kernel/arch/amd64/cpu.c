#include <kernel/cpu.h>
#include <kernel/gdt.h>
#include <kernel/kprint.h>
#include <sched/task.h>

#define REG(reg) reg, reg

void native_dump_registers(isr_regs_t *cpu_state)
{
    if (!cpu_state) {

        uint64_t eax, ebx, ecx, edx, edi, cr3, cr2;

        asm volatile ("mov %%rax, %0\n\
                       mov %%rbx, %1\n\
                       mov %%rcx, %2\n\
                       mov %%rdx, %3\n\
                       mov %%rdi, %4\n\
                       mov %%cr2, %%rax\n\
                       mov %%rax, %5\n\
                       mov %%cr3, %%rax\n\
                       mov %%rax, %6"
                       : "=g"(eax), "=g"(ebx), "=g"(ecx), "=g"(edx),
                         "=g"(edi), "=g"(cr2), "=g"(cr3));

        kprint("\nregister contents:\n");
        kprint("\trax: 0x%016x %20u\n\trbx: 0x%016x %20u\n\trcx: 0x%016x %20u\n"
               "\trdx: 0x%016x %20u\n\trdi: 0x%016x %20u\n\tcr2: 0x%016x %20u\n"
               "\tcr3: 0x%016x %20u\n\n", REG(eax), REG(ebx), REG(ecx),
               REG(edx), REG(edi),    REG(cr2), REG(cr3));
    } else {
        kprint("\nRegister contents:\n");
        kprint("\trax: 0x%016x %20u\n"
               "\trcx: 0x%016x %20u\n"
               "\trdx: 0x%016x %20u\n"
               "\trbx: 0x%016x %20u\n"
               "\trsi: 0x%016x %20u\n"
               "\trdi: 0x%016x %20u\n"
               "\trbp: 0x%016x %20u\n"
               "\trsp: 0x%016x %20u\n"
               "\trip: 0x%016x %20u\n",     cpu_state->eax, cpu_state->eax,
            cpu_state->ecx, cpu_state->ecx, cpu_state->edx, cpu_state->edx,
            cpu_state->ebx, cpu_state->ebx, cpu_state->esi, cpu_state->esi,
            cpu_state->edi, cpu_state->edi, cpu_state->ebp, cpu_state->ebp,
            cpu_state->esp, cpu_state->esp, cpu_state->eip, cpu_state->eip);
    }
}

void native_context_prepare(task_t *task, void *ip, void *sp)
{
    task->threads->bootstrap.eip    = (unsigned long)ip;
    task->threads->bootstrap.ebp    = (unsigned long)sp;
    task->threads->bootstrap.esp    = (unsigned long)sp;
    task->threads->bootstrap.eflags = (1 << 9); /* enable interrupts */

    task->threads->bootstrap.cs     = SEG_USER_CODE;
    task->threads->bootstrap.ss     = SEG_USER_DATA;
    task->threads->state            = T_RUNNING;
}

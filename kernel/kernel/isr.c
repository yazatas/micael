#include <kernel/common.h>
#include <kernel/isr.h>
#include <kernel/kassert.h>
#include <kernel/kpanic.h>
#include <stdint.h>

#define MAX_INT 256

extern void mmu_pf_handler(isr_regs_t *cpu);
extern void gpf_handler(isr_regs_t *cpu);
extern void syscall_handler(isr_regs_t *cpu);

typedef void (*isr_handler_t)(isr_regs_t *cpu);

static isr_handler_t handlers[MAX_INT] = {
    [VECNUM_SYSCALL]    = syscall_handler,
    [VECNUM_PAGE_FAULT] = mmu_pf_handler,
    [VECNUM_GPF]        = gpf_handler
};

const char *interrupts[] = {
    "division by zero",            "debug exception",          "non-maskable interrupt",
    "breakpoint",                  "into detected overflow",   "out of bounds",
    "invalid opcode",              "no coprocessor",           "double fault",
    "coprocessor segment overrun", "bad tss",                  "segment not present",
    "stack fault",                 "general protection fault", "page fault",
    "unknown interrupt",           "coprocessor fault",        "alignment check",
    "machine check",               "simd floating point",      "virtualization",
};

void isr_install_handler(int num, void (*handler)(isr_regs_t *regs))
{
    kassert((num >= 0 && num < MAX_INT) && (handler != NULL));
    handlers[num] = handler;
}

void isr_uninstall_handler(int num, void (*handler)(isr_regs_t *regs))
{
    kassert((num >= 0 && num < MAX_INT) && (handler != NULL));
    handlers[num] = NULL;
}

void interrupt_handler(isr_regs_t *cpu_state)
{
    if (cpu_state->isr_num > MAX_INT)
        kpanic("ISR number is too high");

    if (handlers[cpu_state->isr_num] != NULL)
        return handlers[cpu_state->isr_num](cpu_state);

    switch (cpu_state->isr_num) {
        case 0x00: case 0x01: case 0x02: case 0x03: 
        case 0x04: case 0x05: case 0x06: case 0x07: 
        case 0x08: case 0x09: case 0x0a: case 0x0b:
        case 0x0c: case 0x0f: case 0x10: case 0x11: 
        case 0x12: case 0x14:
            kpanic(interrupts[cpu_state->isr_num]);
            break;

        default:
            kpanic("unsupported interrut!");
            break;
    }
}

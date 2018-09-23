#include <kernel/kprint.h>
#include <kernel/kpanic.h>
#include <kernel/common.h>
#include <mm/vmm.h>
#include <sched/syscall.h>
#include <arch/i386/gpf.h>

#include <stdint.h>
#include <stdio.h>

#define ISR_SYSCALL    0x80
#define ISR_PAGE_FAULT 0x0e
#define ISR_GPF        0x0d

const char *interrupts[] = {
    "division by zero",            "debug exception",          "non-maskable interrupt",
    "breakpoint",                  "into detected overflow",   "out of bounds",
    "invalid opcode",              "no coprocessor",           "double fault",
    "coprocessor segment overrun", "bad tss",                  "segment not present",
    "stack fault",                 "general protection fault", "page fault",
    "unknown interrupt",           "coprocessor fault",        "alignment check",
    "machine check",               "simd floating point",      "virtualization",
};

extern void interrupt_handler(isr_regs_t *cpu_state)
{
    switch (cpu_state->isr_num) {
        case 0x00: case 0x01: case 0x02: case 0x03: 
        case 0x04: case 0x05: case 0x06: case 0x07: 
        case 0x08: case 0x09: case 0x0a: case 0x0b:
        case 0x0c: case 0x0f: case 0x10: case 0x11: 
        case 0x12: case 0x14:
            kpanic(interrupts[cpu_state->isr_num]);
            __builtin_unreachable();
            break;

        case ISR_GPF:
            gpf_handler(cpu_state->err_num);
            __builtin_unreachable();
            break;

        case ISR_PAGE_FAULT:
            vmm_pf_handler(cpu_state->err_num);
            break;

        default:
            kpanic("unsupported interrut!");
            break;
    }
}


#include <kernel/common.h>
#include <kernel/compiler.h>
#include <kernel/kprint.h>
#include <kernel/percpu.h>

void __noreturn gpf_handler(isr_regs_t *cpu_state)
{
    uint32_t error_number = cpu_state->err_num;

    kprint("\nGeneral Protection Fault\n");
    kprint("Error number: %d 0x%x\n", error_number, error_number);
    kprint("CPUID: %u", get_thiscpu_id());

    uint32_t table_index = (error_number >> 1) & 0x3;

    switch (table_index) {
        case 0b00:
            kprint("Faulty descriptor in GDT!\n");
            break;

        case 0b01:
            kprint("Faulty descriptor in IDT!\n");
            break;

        case 0b10:
            kprint("Faulty descriptor in LDT!\n");
            break;

        case 0b11:
            kprint("Faulty descriptor in IDT!\n");
            break;
    }

    uint32_t desc_index = (error_number >> 3) & 0x1fff;
    kprint("Faulty descriptor index: %u\n", desc_index);

    while (1);
    __builtin_unreachable();
}

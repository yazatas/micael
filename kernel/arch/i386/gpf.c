#include <arch/i386/gpf.h>
#include <kernel/kprint.h>

void gpf_handler(uint32_t error_number)
{
    kprint("General Protection Fault");
    kprint("Error number: %d 0x%x", error_number, error_number);

    uint32_t table_index = (error_number & 0x6) >> 1;

    switch (table_index) {
        case 0b00:
            kdebug("Faulty descriptor in GDT!");
            break;

        case 0b01:
            kdebug("Faulty descriptor in IDT!");
            break;

        case 0b10:
            kdebug("Faulty descriptor in LDT!");
            break;

        case 0b11:
            kdebug("Faulty descriptor in IDT!");
            break;
    }

    uint32_t desc_index = (error_number & 0xfff8) >> 3;
    kdebug("Faulty descriptor index: %u", desc_index);

    while (1);
}

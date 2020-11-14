#ifndef __IO_APIC_H__
#define __IO_APIC_H__

#include <stdint.h>

/* Disable 8259 PIC and initialize all I/O APICs discovered
 * by parsing the ACPI MADT Table */
void ioapic_initialize_all(void);

/* Register I/O APIC device. This is called when parsing the MADT
 * The actual initialization happens later when parsing has finished */
void ioapic_register_dev(uint8_t ioapic_id, uint32_t ioapic_addr, uint32_t intr_base);

/* Enable "irq" on the Local APIC denoted by "cpu" */
void ioapic_enable_irq(unsigned cpu, unsigned irq);

/* TODO:  */
void ioapic_assign_ioint(uint8_t bus_id, uint8_t bus_irq, uint8_t ioapic_id, uint8_t ioapic_irq);

unsigned long ioapic_get_base(void);

#endif /* __IO_APIC_H__ */

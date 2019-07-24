#ifndef __LAPIC_H__
#define __LAPIC_H__

#include <stdint.h>

/* TODO:  */
void lapic_initialize(void);

/* Register CPU to Local APIC subsystem */
void lapic_register_dev(int cpu_id, int loapic_id);

void lapic_send_sipi(unsigned cpu, unsigned vec);
void lapic_send_ipi(uint32_t high, uint32_t low);
void lapic_send_init(unsigned cpu);

/* Return the number of detected CPUs */
unsigned lapic_get_cpu_count(void);

/* Return the number of initialized CPUs (APs) */
unsigned lapic_get_init_cpu_count(void);

/* Return the Local APIC ID of "cpu" 
 * Return -ENXIO if "cpu" doesn't not exist */
int lapic_get_lapic_id(unsigned cpu);

#endif /* __LAPIC_H__ */

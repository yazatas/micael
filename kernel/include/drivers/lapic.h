#ifndef __LAPIC_H__
#define __LAPIC_H__

/* TODO:  */
void lapic_initialize(void);

/* Register CPU to Local APIC subsystem */
int lapic_register(int cpu_id, int loapic_id);

#endif /* __LAPIC_H__ */

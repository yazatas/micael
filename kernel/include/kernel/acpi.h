#ifndef __ACPI_H__
#define __ACPI_H__

/* Read the ACPI info from RAM, set fadt point to the
 * Fixed ACPI Descriptor Table and initialize ACPI if
 * it has not been initialized yet.
 *
 * Return 0 on success
 * Return -ENXIO  if ACPI-related info was not found
 * Return -EINVAL if the ACPI-related info is invalid */
int acpi_initialize(void);

/* Return address of Local APIC discovered from MADT on success
 * Return INVALID_ADDRESS on error and et errno to ENXIO if MADT was not found */
unsigned long acpi_get_local_apic_addr(void);

/* Parse Multiple APIC Description Table (if it exists)
 * and register all Local APIC and I/O APIC devices
 *
 * Return 0 on success
 * Return -EINVAL if MADT does not exists */
int acpi_parse_madt(void);

#endif /* __ACPI_H__ */

#ifndef __ACPI_H__
#define __ACPI_H__

/* Initialize ACPI using ACPICA
 *
 * Return 0 on success
 * Return -ENXIO  if ACPI-related info was not found
 * Return -ENOTSUP if ACPICA initialization failed
 * Return -EINVAL if the ACPI-related info is invalid */
int acpi_init(void);

/* TODO:  */
int acpi_init_pci(void);

/* Return address of Local APIC discovered from MADT on success
 * Return INVALID_ADDRESS on error and et errno to ENXIO if MADT was not found */
unsigned long acpi_get_local_apic_addr(void);

#endif /* __ACPI_H__ */

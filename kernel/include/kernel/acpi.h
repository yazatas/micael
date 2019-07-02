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

#endif /* __ACPI_H__ */

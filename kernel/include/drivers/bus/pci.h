#ifndef __PCI_H__
#define __PCI_H__

#include <lib/list.h>
#include <sys/types.h>

enum PCI_OFFSETS {
    PCI_OFF_VENDOR   = 0x00,
    PCI_OFF_DEVICE   = 0x02,
    PCI_OFF_CMD      = 0x04,
    PCI_OFF_STATUS   = 0x06,
    PCI_OFF_REV_ID   = 0x08,
    PCI_OFF_PROG_IF  = 0x09,
    PCI_OFF_SCLASS   = 0x0a,
    PCI_OFF_CLASS    = 0x0b,
    PCI_OFF_CLS      = 0x0c,
    PCI_OFF_LAT_TMR  = 0x0d,
    PCI_OFF_HEADER   = 0x0e,
    PCI_OFF_BIST     = 0x0f,
    PCI_OFF_BAR0     = 0x10,
    PCI_OFF_BAR1     = 0x14,
    PCI_OFF_BAR2     = 0x18,
    PCI_OFF_BAR3     = 0x1c,
    PCI_OFF_BAR4     = 0x20,
    PCI_OFF_BAR5     = 0x24,
    PCI_OFF_INT_LINE = 0x3c,
    PCI_OFF_INT_PIN  = 0x3d
};

typedef struct pci_dev {
    uint16_t bus;
    uint16_t dev;
    uint16_t func;

    uint16_t device;
    uint16_t vendor;
    uint16_t status;
    uint16_t command;

    uint8_t class;
    uint8_t subclass;
    uint8_t prog_if;   /* register-level programming interface */
    uint8_t rev_id;    /* revision id */
    uint8_t bist;      /* built-in self test */
    uint8_t hdr_type;  /* header type */
    uint8_t lat_tmr;   /* latency timer */
    uint8_t cls;       /* cache line size */

    /* Base addresses */
    uint32_t bar0;
    uint32_t bar1;
    uint32_t bar2;
    uint32_t bar3;
    uint32_t bar4;
    uint32_t bar5;

    uint16_t int_pin;
    uint16_t int_line;

    list_head_t list;
} pci_dev_t;

typedef struct pci_irq_route {
    bool named;   /* see kernel/acpi/pci.c for more details */

    uint8_t bus;
    uint8_t dev;
    uint8_t pin;

    /* If "named" is true, "name" contains the link's name
     * Otherwise "irq" is set */
    uint8_t irq;
    char *name;
} pci_irq_route_t;

typedef struct pci_link {
    char name[32];       /* link name */
    uint32_t current;    /* active interrupt */
} pci_link_t;

/* Probe all busses and create entries for all found devices
 * that can be queried later on by other subysystems
 * Return 0 on success */
int pci_init(void);

/* Read 8, 16, or 32-bit value from PCI register */
uint8_t  pci_read_u8(uint8_t  bus, uint8_t dev, uint8_t func, uint8_t reg);
uint16_t pci_read_u16(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg);
uint32_t pci_read_u32(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg);

/* Write 8, 16, or 32-bit value from PCI register */
void pci_write_u8(uint8_t  bus, uint8_t dev, uint8_t func, uint8_t reg, uint8_t val);
void pci_write_u16(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg, uint16_t val);
void pci_write_u32(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg, uint32_t val);

/* Query device using vendor and device ids
 *
 * Return pointer to PCI dev on success
 * Return NULL on error and set errno to:
 *    ENOENT if device does not exist */
pci_dev_t *pci_get_dev(uint16_t vendor, uint16_t device);

/* Allocate pci_irq_route struct and let caller
 * fill it with PCI IRQ routing related info
 *
 * The struct doesn't need to be given back explicitly
 *
 * Return pointer to struct on success
 * Return NULL on error and set errno to:
 *    ENOMEM if max amount of routes have been allocated already */
pci_irq_route_t *pci_alloc_route(void);

/* Allocate pci_link struct and let caller
 * fill it with PCI link related info
 *
 * The struct doesn't need to be given back explicitly
 *
 * Return pointer to struct on success
 * Return NULL on error and set errno to:
 *    ENOMEM if max amount of routes have been allocated already */
pci_link_t *pci_alloc_link(void);

/* Find IRQ routing information for a device
 *
 * This function should be called by the device when its init()
 * function is called to register an IRQ routine
 *
 * Return the IRQ the device is using on success
 * Return PCI_NO_ROUTE on error and set errno to:
 *    ENOENT if routing info for bus:dev:pin does not exist */
uint8_t pci_find_irq_route(uint8_t bus, uint8_t dev, uint8_t pin);

#endif /* __PCI_H__ */

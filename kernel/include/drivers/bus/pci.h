#ifndef __PCI_H__
#define __PCI_H__

#include <lib/list.h>
#include <sys/types.h>

#define VBE_VENDOR_ID     0x1234
#define VBE_DEVICE_ID     0x1111

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

/* Probe all busses and create entries for all found devices
 * that can be queried later on by other subysystems
 * Return 0 on success */
int pci_init(void);

/* Query device using vendor and device ids
 *
 * Return pointer to PCI dev on success
 * Return NULL on error and set errno to:
 *    ENOENT if device does not exist */
pci_dev_t *pci_get_dev(uint16_t vendor, uint16_t device);

#endif /* __PCI_H__ */

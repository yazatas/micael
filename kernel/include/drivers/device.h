#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <lib/list.h>
#include <sys/types.h>

enum DEVICE_TYPES {
    DT_PCI = 0
};

typedef struct driver {
    int (*init)(void *);
    int (*destroy)(void *);

    int count;
    int device;
    list_head_t list;
} driver_t;

typedef struct device {
    driver_t *driver;  /* pointer to driver struct of the device */
    int type;          /* device type */
    void *ctx;         /* private ctx for the device */
    char *name;        /* name of the device */

    list_head_t list;
} device_t;

/* Initialize the device subsystem */
int dev_init(void);

/* Allocate entry for a new device
 * and initialize everything to zero
 *
 * Return pointer to memory on success
 * Return -ENOMEM if allocation failed */
device_t *dev_alloc_device(void);

/* Allocate entry for a new driver
 * and initialize everything to zero
 *
 * Return pointer to memory on success
 * Return -ENOMEM if allocation failed */
driver_t *dev_alloc_driver(void);

/* Register device to subsystem by creating
 * an entry to devfs and call init() if it exists
 *
 * Return 0 on success
 * Return -EINVAL if "device" is NULL */
int dev_register_device(device_t *device);

/* Destroy device by removing the entry from devfs
 * and deallocate all memory consumed by it and
 * deinit the device by calling destroy() if it exists
 *
 * Return 0 on success
 * Return -EEXIST if a driver for "device" already exists
 * Return -EINVAL if "device" is NULL */
int dev_destroy_device(device_t *device);

/* Register a PCI driver for "device"
 *
 * When the PCI busses are probed and a device is found,
 * dev subsystem is checked if it contains a driver for
 * that device and if so, the device is registered and its
 * init function is called
 *
 * Return 0 on success
 * Return -EINVAL if "driver" is NULL */
int dev_register_pci_driver(int device, driver_t *driver);

/* Try to find drivers for a PCI by device id
 *
 * Return pointer to drivers if they're found
 * Return NULL on error and set errno to:
 *   ENOENT if driver was not found */
driver_t *dev_find_pci_driver(int device);

#endif /* __DEVICE_H__ */

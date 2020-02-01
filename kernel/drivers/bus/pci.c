#include <drivers/device.h>
#include <drivers/bus/pci.h>
#include <kernel/acpi/acpi.h>
#include <kernel/io.h>
#include <kernel/kassert.h>
#include <kernel/kpanic.h>
#include <kernel/kprint.h>
#include <kernel/util.h>
#include <lib/list.h>
#include <mm/heap.h>
#include <mm/slab.h>
#include <errno.h>

#define PCI_CONFIG_ADDR        0xcf8
#define PCI_CONFIG_DATA        0xcfc
#define PCI_ENABLE        0x80000000
#define PCI_NO_DEV            0xffff
#define PCI_MULTI_FUNC          0x80
#define PCI_MAX_ROUTES           128
#define PCI_MAX_LINKS             32
#define PCI_NO_ROUTE            0xff

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
    PCI_OFF_INT_PIN  = 0x3d,
};

static const char *pci_class[] = {
    "Unclassified",
    "Mass Storage Controller",
    "Network Controller",
    "Display Controller",
    "Multimedia Controller",
    "Memory Controller",
    "Bridge Device",
    "Simple Communication Controller",
    "Base System Peripheral",
    "Input Device Controller",
    "Docking Station",
    "Processor",
    "Serial Bus Controller",
    "Wireless Controller",
    "Intelligent Controller",
    "Satellite Communication Controller",
    "Crypto Controller",
    "Signal Processing Controller",
};

static struct pci_info {
    mm_cache_t *pci_cache;
    list_head_t pci_devs;

    size_t nroutes;
    pci_irq_route_t routes[PCI_MAX_ROUTES];

    size_t nlinks;
    pci_link_t links[PCI_MAX_LINKS];
} pci_info;

static inline uint32_t __get_pci_field_u32(int bus, int dev, int func, int offset)
{
    uint32_t addr = PCI_ENABLE | (bus << 16) | (dev << 11) | (func << 8) | (offset & 0xfc);

    /* write address */
    outl(PCI_CONFIG_ADDR, addr);

    /* read data */
    return inl(PCI_CONFIG_DATA);
}

static inline uint16_t __get_pci_field_u16(int bus, int dev, int func, int offset)
{
    uint32_t value = __get_pci_field_u32(bus, dev, func, offset & ~0x3);

    return (uint16_t)(value >> ((offset & 0x2) * 8));
}

static inline uint8_t __get_pci_field_u8(int bus, int dev, int func, int offset)
{
    uint16_t value = __get_pci_field_u16(bus, dev, func, offset & ~0x1);

    return (uint8_t)(value >> ((offset & 0x1) * 8));
}

static void __probe_func(int bus, int dev, int func)
{
    pci_dev_t *pdev = mmu_cache_alloc_entry(pci_info.pci_cache, MM_NO_FLAGS);

    pdev->bus  = bus;
    pdev->dev  = dev;
    pdev->func = func;

    pdev->device = __get_pci_field_u16(bus, dev, func, PCI_OFF_DEVICE);
    pdev->vendor = __get_pci_field_u16(bus, dev, func, PCI_OFF_VENDOR);
    pdev->class  = __get_pci_field_u8(bus,  dev, func, PCI_OFF_CLASS);

    pdev->bar0 = __get_pci_field_u32(bus, dev, func, PCI_OFF_BAR0);
    pdev->bar1 = __get_pci_field_u32(bus, dev, func, PCI_OFF_BAR1);
    pdev->bar2 = __get_pci_field_u32(bus, dev, func, PCI_OFF_BAR2);
    pdev->bar3 = __get_pci_field_u32(bus, dev, func, PCI_OFF_BAR3);
    pdev->bar4 = __get_pci_field_u32(bus, dev, func, PCI_OFF_BAR4);
    pdev->bar5 = __get_pci_field_u32(bus, dev, func, PCI_OFF_BAR5);

    pdev->int_pin  = __get_pci_field_u8(bus, dev, func, PCI_OFF_INT_PIN);
    pdev->int_line = __get_pci_field_u8(bus, dev, func, PCI_OFF_INT_LINE);

    kprint("pci - 0x%x:0x%x %s\n", pdev->vendor, pdev->device,
            (pdev->class < 17) ? pci_class[pdev->class] : "Unknown class");

    if (pdev->int_pin) {
        kdebug("device uses interrupts: 0x%x:0x%x", pdev->int_pin, pdev->int_line);

        uint8_t pin = pci_find_irq_route(bus, dev, pdev->int_pin);

        if (pin != PCI_NO_ROUTE)
            kdebug("something found using pin: %d %x", pin, pin);
        else
            kdebug("did not find route for device");
    }

    /* Initialize the PCI device by creating a new device struct,
     * adding an entry to devfs and possibly calling device's init() */
    driver_t *driver = dev_find_pci_driver(pdev->device);
    device_t *device = dev_alloc_device();

    device->type   = DT_PCI;
    device->ctx    = pdev;
    device->driver = driver;

    /* dev_register_device() calls init() if it exists */
    (void)dev_register_device(device);

    list_init(&pdev->list);
    list_append(&pci_info.pci_devs, &pdev->list);
}

static void __probe_dev(int bus, int dev)
{
    if (__get_pci_field_u16(bus, dev, 0, PCI_OFF_VENDOR) == PCI_NO_DEV)
        return;

     __probe_func(bus, dev, 0);

     /* If this is a multi-function device, check all 8 functions */
    if (__get_pci_field_u32(bus, dev, 0, PCI_OFF_HEADER) & PCI_MULTI_FUNC) {
        for (int f = 1; f < 8; ++f) {
            __probe_func(bus, dev, f);
        }
    }
}

static void __probe_bus(int bus)
{
    for (int dev = 0; dev < 32; ++dev) {
        __probe_dev(bus, dev);
    }
}

static uint8_t __find_named_route(char *link)
{
    for (size_t i = 0; i < pci_info.nlinks; ++i) {
        if (kstrcmp(link, pci_info.links[i].name) == 0)
            return pci_info.links[i].current;
    }

    errno = ENOENT;
    return PCI_NO_ROUTE;
}

uint8_t  pci_read_u8(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg)
{
    return __get_pci_field_u8(bus, dev, func, reg);
}

uint16_t pci_read_u16(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg)
{
    return __get_pci_field_u16(bus, dev, func, reg);
}

uint32_t pci_read_u32(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg)
{
    return __get_pci_field_u32(bus, dev, func, reg);
}

int pci_init(void)
{
    if ((pci_info.pci_cache = mmu_cache_create(sizeof(pci_dev_t), MM_NO_FLAGS)) == NULL)
        kpanic("Failed to allocate space for PCI's SLAB cache");

    /* initialize internal pci state */
    list_init(&pci_info.pci_devs);
    pci_info.nroutes = 0;
    pci_info.nlinks  = 0;

    /* initialize pci irqs according to info found from ACPI */
    acpi_init_pci();

    /* probe the first bus and find all devices attached to it */
    __probe_bus(0);

    return 0;
}

pci_dev_t *pci_get_dev(uint16_t vendor, uint16_t device)
{
    pci_dev_t *dev = NULL;

    FOREACH(pci_info.pci_devs, iter) {
        dev = container_of(iter, pci_dev_t, list);

        if (dev->vendor == vendor && dev->device == device)
            return dev;
    }

    errno = ENOENT;
    return NULL;
}

pci_irq_route_t *pci_alloc_route(void)
{
    if (pci_info.nroutes >= PCI_MAX_ROUTES) {
        errno = ENOMEM;
        return NULL;
    }

    return &pci_info.routes[pci_info.nroutes++];
}

pci_link_t *pci_alloc_link(void)
{
    if (pci_info.nlinks >= PCI_MAX_ROUTES) {
        errno = ENOMEM;
        return NULL;
    }

    return &pci_info.links[pci_info.nlinks++];
}

uint8_t pci_find_irq_route(uint8_t bus, uint8_t dev, uint8_t pin)
{
    for (size_t i = 0; i < pci_info.nroutes; ++i) {
        pci_irq_route_t *route = &pci_info.routes[i];

        if (route->bus == bus && route->dev == dev && route->pin == pin) {
            if (route->named)
                return __find_named_route(route->name);
            return route->irq;
        }
    }

    errno = ENOENT;
    return PCI_NO_ROUTE;
}

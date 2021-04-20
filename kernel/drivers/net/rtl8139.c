#include <drivers/bus/pci.h>
#include <drivers/net/rtl8139.h>
#include <drivers/device.h>
#include <drivers/ioapic.h>
#include <drivers/lapic.h>
#include <kernel/io.h>
#include <kernel/irq.h>
#include <kernel/kassert.h>
#include <kernel/kpanic.h>
#include <kernel/kprint.h>
#include <kernel/util.h>
#include <mm/heap.h>
#include <mm/mmu.h>
#include <mm/page.h>
#include <net/eth.h>
#include <net/netdev.h>

#define TX_BUFFER_SIZE 4

typedef struct rtl8139 rtl8139_t;

static struct rtl8139 {
    uint64_t mac;
    uint32_t base;
    int tx_number;
    int tx_flag;
    unsigned long cbr;
    unsigned long rx_buffer;
    unsigned long tx_buffer[TX_BUFFER_SIZE];
    size_t tx_size[TX_BUFFER_SIZE];
} __nic;

static uint32_t __handle_rx(rtl8139_t *rtl)
{
    uint16_t cbr = inw(rtl->base + RTL8139_CBR);
    uint16_t packet_offset = rtl->cbr;

    uint8_t *data      = mmu_p_to_v(rtl->rx_buffer + packet_offset);
    eth_frame_t *frame = (eth_frame_t *)&data[4];

    eth_handle_frame(frame, ((uint16_t *)data)[1]);
}

static void rtl8139_cmd_tx(rtl8139_t *rtl)
{
    int n = rtl->tx_number;

    rtl->tx_number++;
    if (rtl->tx_number == 4)
        rtl->tx_number = 0;

    outl(rtl->base + RTL8139_TSD(n), rtl->tx_size[n]);
}

static void __handle_tx(rtl8139_t *rtl)
{
    int tx = 3;

    if (rtl->tx_number)
        tx = rtl->tx_number - 1;

    rtl->tx_flag     &= ~(1 << tx);
    rtl->tx_size[tx]  = 0;

    while (rtl->tx_flag & (1 << rtl->tx_number))
        rtl8139_cmd_tx(rtl);
}

static uint8_t rtl8139_alloc_tx(rtl8139_t *rtl)
{
    for (int i = rtl->tx_number; i < 4; ++i) {
        if (!(rtl->tx_flag & (1 << i)))
            return i;
    }

    for (int i = 0; i < rtl->tx_number; ++i) {
        if (!(rtl->tx_flag & (1 << i)))
            return i;
    }

    return 0xff;
}

static uint32_t __irq_handler(void *ctx)
{
    rtl8139_t *rtl = ctx;
    uint16_t isr   = inw(rtl->base + RTL8139_ISR);

    if (isr) {
        if (isr & RTL8139_ISR_ROK)
            __handle_rx(rtl);
        else if (isr & (RTL8139_ISR_TOK | RTL8139_ISR_TER))
            __handle_tx(rtl);

        outw(rtl->base + RTL8139_ISR, isr);
        lapic_ack_interrupt();

        return IRQ_HANDLED;
    }

    lapic_ack_interrupt();
    return IRQ_UNHANDLED;
}

static int __destroy(void *arg)
{
    (void)arg;
    return 0;
}

static int __init(void *arg)
{
    kprint("rtl8139 - initializing device\n");

    device_t *dev   = arg;
    pci_dev_t *pdev = dev->ctx;

    unsigned cmd = pci_read_u32(pdev->bus, pdev->dev, pdev->func, PCI_OFF_CMD);
    cmd |= RTL8139_PCI_BUS_MASTER;
    pci_write_u32(pdev->bus, pdev->dev, pdev->func, PCI_OFF_CMD, cmd);

    /* two lowest bits must be cleared to get base address */
    pdev->bar0       &= ~0x3;
    __nic.base       = pdev->bar0;
    __nic.mac        = ((uint64_t)inl(pdev->bar0 + 0x0) << 16) | inw(pdev->bar0 + 0x4);
    __nic.cbr        = 0;
    __nic.tx_number  = 0;
    __nic.tx_flag    = 0;

    /* power on */
    outb(pdev->bar0 + RTL8139_CONFIG1, 0x0);

    /* software reset */
    outb(pdev->bar0 + RTL8139_CR, 0x10);

    while ((inb(pdev->bar0 + RTL8139_CR) & 0x10))
        ;

    /* rx buffer initilization */
    __nic.rx_buffer = mmu_block_alloc(MM_ZONE_NORMAL, 2);
    outl(pdev->bar0 + RTL8139_RBSTART, __nic.rx_buffer);

    /* tx buffer initialization */
    for (int i = 0; i < TX_BUFFER_SIZE; ++i) {
        __nic.tx_buffer[i] = mmu_page_alloc(MM_ZONE_NORMAL);
        __nic.tx_size[i]   = 0;
        outl(pdev->bar0 + RTL8139_TSAD(i), __nic.tx_buffer[i]);
    }

    /* enable packets */
    outl(pdev->bar0 + RTL8139_RCR, RTL8139_RCR_APM | RTL8139_RCR_AB | RTL8139_RCR_WRAP);

    /* enable rx/tx */
    outb(pdev->bar0 + RTL8139_CR, RTL8139_CR_TE | RTL8139_CR_RE);

    /* enable interrupts */
    outw(pdev->bar0 + RTL8139_IMR, RTL8139_IMR_ROK | RTL8139_IMR_RER |
                                   RTL8139_IMR_TOK | RTL8139_IMR_TER);

    /* clear isr */
    outw(pdev->bar0 + RTL8139_ISR, 0);

    ioapic_enable_irq(0, VECNUM_IRQ_START + pdev->int_line);
    irq_install_handler(VECNUM_IRQ_START + pdev->int_line, __irq_handler, &__nic);

    /* initialize network manager */
    netdev_set_mac(__nic.mac);
    netdev_init();

    return 0;
}

int rtl8139_init(void)
{
    kprint("rtl8139 - initializing network card\n");

    driver_t *drv = dev_alloc_driver();

    drv->count   = 1;
    drv->device  = RTL8139_DEV_ID;
    drv->vendor  = RTL8139_VENDOR_ID;
    drv->init    = __init;
    drv->destroy = __destroy;

    return dev_register_pci_driver(RTL8139_VENDOR_ID, RTL8139_DEV_ID, drv);
}

int rtl8139_send_pkt(uint8_t *data, size_t size)
{
    rtl8139_t *rtl = &__nic;
    uint8_t index  = rtl8139_alloc_tx(rtl);

    void *page = mmu_p_to_v(rtl->tx_buffer[index]);
    kmemcpy(page, data, size);

    rtl->tx_size[index] = size;

    if (index == rtl->tx_number)
        rtl8139_cmd_tx(rtl);

    return 0;
}

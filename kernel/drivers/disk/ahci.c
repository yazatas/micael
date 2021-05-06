#include <drivers/bus/pci.h>
#include <drivers/disk/ahci.h>
#include <drivers/device.h>
#include <kernel/common.h>
#include <kernel/compiler.h>
#include <kernel/irq.h>
#include <kernel/kassert.h>
#include <kernel/kpanic.h>
#include <kernel/util.h>
#include <mm/mmu.h>
#include <mm/page.h>
#include <mm/heap.h>

typedef struct ahci_port_regs {
    uint32_t p_clb;
    uint32_t p_clbu;
    uint32_t p_fb;
    uint32_t p_fbu;
    uint32_t p_is;
    uint32_t p_ie;
    uint32_t p_cmd;
    uint32_t __res0;
    uint32_t p_tfd;
    uint32_t p_sig;
    uint32_t p_ssts;
    uint32_t p_sctl;
    uint32_t p_serr;
    uint32_t p_sact;
    uint32_t p_ci;
    uint32_t p_sntf;
    uint32_t p_fbs;
    uint32_t p_devslp;
    uint32_t reserved[14];
} __packed ahci_port_regs_t;

typedef struct ahci_registers {
    uint32_t cap;
    uint32_t ghc;
    uint32_t is;
    uint32_t pi;
    uint32_t vs;
    uint32_t ccc_ctl;
    uint32_t ccc_ports;
    uint32_t em_loc;
    uint32_t em_ctl;
    uint32_t cap2;
    uint32_t bohc;
    uint32_t reserved[53];
    ahci_port_regs_t ports[32];
} __packed ahci_registers_t;

typedef struct ahci_fis_reg_h2d {
    uint8_t type;
    uint8_t cmd_port;
    uint8_t cmd;
    uint8_t feature_low;
    uint8_t lba0;
    uint8_t lba1;
    uint8_t lba2;
    uint8_t dev;
    uint8_t lba3;
    uint8_t lba4;
    uint8_t lba5;
    uint8_t feature_high;
    uint8_t countl;
    uint8_t counth;
    uint8_t icc;
    uint8_t control;
    uint8_t reserved[4];
} ahci_fis_reg_h2d_t;

typedef struct ahci_cmd_header {
    uint16_t attr;
    uint16_t prdtl;
    volatile uint32_t prdbc;
    uint32_t ctba;
    uint32_t ctbau;
    uint32_t reserved[4];
} __packed ahci_cmd_header_t;

typedef struct ahci_prdt_entry {
    uint32_t dba;
    uint32_t dbau;
    uint32_t reserved;
    uint32_t dbc;
} __packed ahci_prdt_entry_t;

typedef struct ahci_cmd_tbl_entry {
    union {
        ahci_fis_reg_h2d_t fis_reg_h2d;
        uint8_t cmd[64];
    } __packed;
    uint8_t acmd[16];
    uint8_t reserved[48];
    struct ahci_prdt_entry prdt[];
} __packed ahci_cmd_tbl_entry_t;

static uint32_t __irq_handler(void *ctx)
{
    (void)ctx;

    kpanic("ahci interrupt handler called");
}

static int __destroy(void *arg)
{
    (void)arg;

    return 0;
}

static void __sata_access(int write, struct ahci_port_regs *port, void *buf, uint32_t nsect, uint64_t lba)
{
    port->p_is = -1;

    /* TODO: support multiple disks */

    /* command lists, headers and entries are stored
     * in the port memory created during port initialization */
    ahci_cmd_header_t *cmd_hdr    = mmu_p_to_v(((uint64_t)port->p_clbu   << 32) | port->p_clb);
    ahci_cmd_tbl_entry_t *cmd_tbl = mmu_p_to_v(((uint64_t)cmd_hdr->ctbau << 32) | cmd_hdr->ctba);

    size_t byte_cnt   = nsect * 512;
    size_t bytes_left = byte_cnt;
    size_t prd_cnt    = (byte_cnt + 8191) / 8192;

    cmd_hdr->attr  = sizeof(ahci_fis_reg_h2d_t) / sizeof(uint32_t);
    cmd_hdr->prdtl = prd_cnt;
    cmd_hdr->prdbc = 0;

    uint64_t buf_phys = mmu_v_to_p(buf);
    kmemset(cmd_tbl, 0, sizeof(ahci_cmd_tbl_entry_t) + sizeof(ahci_prdt_entry_t) * prd_cnt);

    for (size_t i = 0; i < prd_cnt; ++i) {
        size_t prd_size  = MIN(8192, bytes_left);
        uint64_t prd_buf = buf_phys + 8192 * i;

        cmd_tbl->prdt[i].dba  = prd_buf & 0xffffffff;
        cmd_tbl->prdt[i].dbau = prd_buf >> 32;
        cmd_tbl->prdt[i].dbc  = ((prd_size - 1) << 1) | 1;

        if (i == prd_cnt - 1)
            cmd_tbl->prdt[i].dbc |= 1 << 31;

        bytes_left -= prd_size;
    }

    ahci_fis_reg_h2d_t *fis_reg_h2d = &cmd_tbl->fis_reg_h2d;
    kmemset(fis_reg_h2d, 0, sizeof(fis_reg_h2d));

    fis_reg_h2d->type     = FIS_TYPE_H2D;
    fis_reg_h2d->cmd      = write ? ATA_CMD_DMA_WRITE_EXT : ATA_CMD_DMA_READ_EXT;
    fis_reg_h2d->cmd_port = 1 << 7;
    fis_reg_h2d->lba0     = lba & 0xff;
    fis_reg_h2d->lba1     = (lba >> 8) & 0xff;
    fis_reg_h2d->lba2     = (lba >> 16) & 0xff;
    fis_reg_h2d->dev      = (1 << 6);
    fis_reg_h2d->lba3     = (lba >> 24) & 0xff;
    fis_reg_h2d->lba4     = (lba >> 32) & 0xff;
    fis_reg_h2d->lba5     = (lba >> 40) & 0xff;
    fis_reg_h2d->countl   = nsect & 0xff;
    fis_reg_h2d->counth   = (nsect >> 8) & 0xff;

    while (port->p_tfd & (ATA_SR_BSY | ATA_SR_DRQ))
        ;

    port->p_ci |= 0x1;

    for (;;) {
        if (!(port->p_ci & 0x1))
            break;
    }
}

static void __sata_write(struct ahci_port_regs *port, void *buf, uint32_t nsect, uint64_t lba)
{
    return __sata_access(1, port, buf, nsect, lba);
}

static void __sata_read(struct ahci_port_regs *port, void *buf, uint32_t nsect, uint64_t lba)
{
    return __sata_access(0, port, buf, nsect, lba);
}

static inline void __sata_port_stop(struct ahci_port_regs *port)
{
    port->p_cmd &= ~(AHCI_PORT_CMD_ST | AHCI_PORT_CMD_FRE);

    while (port->p_cmd & AHCI_PORT_CMD_FR)
        ;
}

static inline void __sata_port_start(struct ahci_port_regs *port)
{
    while (port->p_cmd & AHCI_PORT_CMD_CR)
        ;

    port->p_cmd |= AHCI_PORT_CMD_FRE | AHCI_PORT_CMD_ST;
}

static int *__init_sata_port(device_t *dev, struct ahci_port_regs *port)
{
    kprint("ahci - initialize sata port\n");

    /* the ahci device must be stopped before modifying lb/fb */
    __sata_port_stop(port);

    unsigned long page = mmu_page_alloc(MM_ZONE_NORMAL, 0);

    port->p_clb  = page & 0xffffffff;
    port->p_clbu = page >> 32;

    port->p_fb = (page + 0x400) & 0xffffffff;
    port->p_fb = (page + 0x400) >> 32;

    unsigned long block = mmu_block_alloc(1, MM_ZONE_NORMAL, 0);
    kmemset(mmu_p_to_v(block), 0, PAGE_SIZE * 2);

    struct ahci_cmd_header *cmd_list = mmu_p_to_v(page);
    for (size_t i = 0; i < 32; ++i) {
        unsigned long addr = block + i * 256;

        cmd_list[i].prdtl = 8;
        cmd_list[i].ctba  = addr & 0xFFFFFFFF;
        cmd_list[i].ctbau = addr >> 32;
    }

    /* TODO: create entry to devfs */

    /* start the device and enable interrupts */
    __sata_port_start(port);
    port->p_ie |= (1 << 0);
}

static int __init(void *arg)
{
    kprint("ahci - probing ports\n");

    device_t *dev   = arg;
    pci_dev_t *pdev = dev->ctx;

    mmu_map_page(pdev->bar5, (unsigned long)pdev->bar5, MM_PRESENT | MM_READWRITE);

    ahci_registers_t *regs = (ahci_registers_t *)(intptr_t)pdev->bar5;
    int ports              = regs->pi;

    for (int i = 0; i < AHCI_MAX_PORTS; ++i, ports >>= 1) {
        if (ports & 0x1) {
            struct ahci_port_regs *port = &regs->ports[i];

            /* skip non-active, non-sata ports */
            if (((port->p_ssts >> 0) & 0x0f) != PORT_STATUS_DET_PRESENT ||
                ((port->p_ssts >> 8) & 0x0f) != PORT_STATUS_IPM_ACTIVE  ||
                ((port->p_sig)               != PORT_SIG_SATA))
            {
                continue;
            }

            __init_sata_port(dev, port);
        }
    }

    irq_install_handler(pdev->int_pin, __irq_handler, NULL);

    return 0;
}

int ahci_init(void)
{
    kprint("ahci - initializing ahci subsystem\n");

    driver_t *drv = dev_alloc_driver();

    drv->count   = 1;
    drv->device  = AHCI_DEV_ID;
    drv->vendor  = AHCI_VENDOR_ID;
    drv->init    = __init;
    drv->destroy = __destroy;

    return dev_register_pci_driver(AHCI_VENDOR_ID, AHCI_DEV_ID, drv);
}

#ifndef __AHCI_H__
#define __AHCI_H__

enum {
    FIS_TYPE_H2D       = 0x27,
    FIS_TYPE_D2H       = 0x34,
    FIS_TYPE_DMA_ACT   = 0x39,
    FIS_TYPE_DMA_SETUP = 0x41,
    FIS_TYPE_DATA      = 0x46,
    FIS_TYPE_BIST      = 0x58,
    FIS_TYPE_PIO_SETUP = 0x5f,
    FIS_TYPE_DEV_BITS  = 0xa1,
};

#define AHCI_PORT_CMD_ST   (1 <<  0)
#define AHCI_PORT_CMD_FRE  (1 <<  4)
#define AHCI_PORT_CMD_FR   (1 << 14)
#define AHCI_PORT_CMD_CR   (1 << 15)
#define AHCI_PORT_TFES     (1 << 30)

#define AHCI_DEV        0x2922
#define AHCI_MAX_PORTS      32

#define AHCI_NUM_PRDT_ENTRIES 16

#define PORT_SIG_SATA   0x00000101
#define PORT_SIG_SATAPI 0xeb140101
#define PORT_SIG_SEMB   0xc33c0101
#define PORT_SIG_PM     0x96690101

#define PORT_STATUS_DET_PRESENT  0x3
#define PORT_STATUS_IPM_ACTIVE   0x1

#define ATA_SECTOR_SIZE   512
#define ATA_MODEL_LENGTH   40

#define ATA_CMD_PIO_READ        0x20
#define ATA_CMD_PIO_READ_EXT    0x24
#define ATA_CMD_PIO_WRITE       0x30
#define ATA_CMD_PIO_WRITE_EXT   0x34
#define ATA_CMD_DMA_READ        0xc8
#define ATA_CMD_DMA_READ_EXT    0x25
#define ATA_CMD_DMA_WRITE       0xca
#define ATA_CMD_DMA_WRITE_EXT   0x35
#define ATA_CMD_CACHE_FLUSH     0xe7
#define ATA_CMD_CACHE_FLUSH_EXT 0xea
#define ATA_CMD_PACKET          0xa0
#define ATA_CMD_IDENTIFY_PACKET 0xa1
#define ATA_CMD_IDENTIFY        0xec
#define ATA_CMD_SET_FEATURES    0xef

#define ATA_IDENT_DEVICE_TYPE   0x00
#define ATA_IDENT_CYLINDERS     0x02
#define ATA_IDENT_HEADS         0x06
#define ATA_IDENT_SECTORS       0x0C
#define ATA_IDENT_SERIAL        0x14
#define ATA_IDENT_MODEL         0x36
#define ATA_IDENT_CAPS          0x62
#define ATA_IDENT_MAX_LBA       0x78
#define ATA_IDENT_CMD_SETS      0xA4
#define ATA_IDENT_MAX_LBAEXT    0xC8

#define ATA_SR_BSY     0x80
#define ATA_SR_DRDY    0x40
#define ATA_SR_DF      0x20
#define ATA_SR_DSC     0x10
#define ATA_SR_DRQ     0x08
#define ATA_SR_CORR    0x04
#define ATA_SR_IDX     0x02
#define ATA_SR_ERR     0x01

#define ATA_ER_BBK      0x80
#define ATA_ER_UNC      0x40
#define ATA_ER_MC       0x20
#define ATA_ER_IDNF     0x10
#define ATA_ER_MCR      0x08
#define ATA_ER_ABRT     0x04
#define ATA_ER_TK0NF    0x02
#define ATA_ER_AMNF     0x01

int ahci_init(void);

#endif /* __AHCI_H__ */

#ifndef __RTL8139_H__
#define __RTL8139_H__

#define RTL8139_VENDOR_ID  0x10ec
#define RTL8139_DEV_ID     0x8139

#define RTL8139_PCI_BUS_MASTER  0x4

#define RTL8139_IDR0        0x00
#define RTL8139_MAR0        0x08
#define RTL8139_TSD(n)      (0x10 + 4 * (n))
#define RTL8139_TSAD(n)     (0x20 + 4 * (n))
#define RTL8139_RBSTART     0x30
#define RTL8139_ERBCR       0x34
#define RTL8139_ERSR        0x36
#define RTL8139_CR          0x37
#define RTL8139_CAPR        0x38
#define RTL8139_CBR         0x3a
#define RTL8139_IMR         0x3c
#define RTL8139_ISR         0x3e
#define RTL8139_TCR         0x40
#define RTL8139_RCR         0x44
#define RTL8139_TCTR        0x48
#define RTL8139_MPC         0x4c
#define RTL8139_CONFIG0     0x51
#define RTL8139_CONFIG1     0x52
#define RTL8139_MSR         0x58

#define RTL8139_TSD_OWN     (1 << 13)

#define RTL8139_CR_RST      (1 << 4)
#define RTL8139_CR_RE       (1 << 3)
#define RTL8139_CR_TE       (1 << 2)

#define RTL8139_RCR_WRAP    (1 << 7)
#define RTL8139_RCR_AB      (1 << 0)
#define RTL8139_RCR_AM      (1 << 1)
#define RTL8139_RCR_APM     (1 << 2)
#define RTL8139_RCR_AAP     (1 << 3)
#define RTL8139_RCR_ALL     (RTL8139_RCR_AB | RTL8139_RCR_AM | RTL8139_RCR_APM | RTL8139_RCR_AAP)

#define RTL8139_ISR_TER     (1 << 3)
#define RTL8139_ISR_TOK     (1 << 2)
#define RTL8139_ISR_RER     (1 << 1)
#define RTL8139_ISR_ROK     (1 << 0)

#define RTL8139_IMR_TER     (1 << 3)
#define RTL8139_IMR_TOK     (1 << 2)
#define RTL8139_IMR_RER     (1 << 1)
#define RTL8139_IMR_ROK     (1 << 0)

int rtl8139_init(void);

#endif /* __RTL8139_H__ */

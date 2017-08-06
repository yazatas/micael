#ifndef __PIC_H__
#define __PIC_H__

#define PIC_MASTER_CMD_PORT  0x20
#define PIC_MASTER_DATA_PORT 0x21
#define PIC_SLAVE_CMD_PORT   0xa0
#define PIC_SLAVE_DATA_PORT  0xa1
#define PIC_ACK              0x20

void irq_init(void);

#endif /* end of include guard: __PIC_H__ */

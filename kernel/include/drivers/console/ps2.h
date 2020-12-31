#ifndef __PS2_H__
#define __PS2_H__

#include <kernel/cpu.h>

int ps2_init(void);
unsigned char ps2_read_next(void);
void ps2_isr_handler(isr_regs_t *cpu);

#endif /* __PS2_H__ */

#ifndef __ISR_H__
#define __ISR_H__

#include <kernel/common.h>

#define VECNUM_SPURIOUS   0xff
#define VECNUM_IRQ_START  0x20
#define VECNUM_TIMER      0x20
#define VECNUM_SYSCALL    0x80
#define VECNUM_PAGE_FAULT 0x0e
#define VECNUM_GPF        0x0d

void isr_install_handler(int num, void (*handler)(isr_regs_t *regs));
void isr_uninstall_handler(int num, void (*handler)(isr_regs_t *regs));

#endif /* __ISR_H__ */

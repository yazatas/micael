#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H

#include <stddef.h>

void term_init(void);
void term_putc(char c);
void term_puts(char *data);

#endif

#include <kernel/io.h>
#include <kernel/irq.h>
#include <drivers/timer.h>
#include <kernel/kprint.h>

#include <stddef.h>
#include <stdio.h>

static size_t ticks = 0;

void timer_phase(size_t hz)
{
	size_t div = 1193180 / hz;
	outb(0x43, 0x36);
	outb(0x40, div & 0xff);
	outb(0x40, div >> 8);
}

void timer_handler()
{
    static int i = 0; 
    if (i++ == 7)
        kdebug("in timer handler"), i = 0;
	ticks++;
}

void timer_wait(size_t wait)
{
	size_t diff = ticks + wait;
	while (ticks < diff);
}

void timer_install(void)
{
	irq_install_handler(timer_handler, 0);
}

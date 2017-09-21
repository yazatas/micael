#include <kernel/io.h>
#include <kernel/irq.h>
#include <kernel/drivers/timer.h>

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

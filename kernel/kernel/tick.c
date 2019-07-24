#include <kernel/kprint.h>
#include <kernel/tick.h>

/* TODO: percpu? */
static volatile unsigned long tick = 0;
static unsigned long scale = 0; /* how many ticks is 1ms */

void tick_init(unsigned long ticks, unsigned long ms)
{
    scale = ticks / ms;
}

void tick_inc(void)
{
    tick += scale;
}

void tick_wait(unsigned long ticks)
{
    unsigned long start = tick;

    while (start + ticks > tick)
        ;
}

unsigned long tick_ms_to_ticks(unsigned long ms)
{
    return ms * scale;
}

unsigned long tick_us_to_ticks(unsigned long us)
{
    return (us / 1000) * scale;
}

unsigned long tick_ns_to_ticks(unsigned long ns)
{
    return (ns / 1000 / 1000) * scale;
}

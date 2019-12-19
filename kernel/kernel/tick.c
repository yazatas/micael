#include <kernel/compiler.h>
#include <kernel/kprint.h>
#include <kernel/percpu.h>
#include <kernel/tick.h>
#include <sync/spinlock.h>

__percpu static unsigned long __pcpu_tick = 0;
static unsigned long scale = 0; /* how many ticks is 1ms */
static spinlock_t tick_spin;

void tick_init(unsigned long ticks, unsigned long ms)
{
    scale = ticks / ms;
}

void tick_inc(void)
{
    get_thiscpu_var(__pcpu_tick) += scale;
    put_thiscpu_var(__pcpu_tick);
}

void tick_wait(unsigned long ticks)
{
    unsigned long start = get_thiscpu_var(__pcpu_tick);

    while (start + ticks > get_thiscpu_var(__pcpu_tick))
        asm volatile ("pause");
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

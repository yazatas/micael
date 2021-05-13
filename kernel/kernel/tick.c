#include <kernel/compiler.h>
#include <kernel/cpu.h>
#include <kernel/kprint.h>
#include <kernel/percpu.h>
#include <kernel/tick.h>
#include <lib/list.h>
#include <sync/spinlock.h>

__percpu static list_head_t timers;
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

    unsigned long ticks = get_thiscpu_var(__pcpu_tick);
    list_head_t list    = get_thiscpu_var(timers);
    list_head_t *iter   = &list;

    if (!iter->next)
        return;

    while ((iter = iter->next)) {
        timer_t *tmr = container_of(iter, timer_t, list);

        if (ticks >= tmr->expr) {
            tmr->callback(tmr->ctx);
            list_remove(&tmr->list);
        }
    }
}

void tick_wait(unsigned long ticks)
{
    unsigned long start = get_thiscpu_var(__pcpu_tick);

    while (start + ticks > get_thiscpu_var(__pcpu_tick))
        cpu_relax();
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

void tick_init_timer(void)
{
    (get_thiscpu_var(timers)).next = NULL;
    (get_thiscpu_var(timers)).prev = NULL;
}

void tick_install_timer(timer_t *tmr)
{
    tmr->expr = get_thiscpu_var(__pcpu_tick) + scale * tmr->wait;
    list_append(get_thiscpu_ptr(timers), &tmr->list);
}

#ifndef __TICK_H__
#define __TICK_H__

#include <lib/list.h>

typedef struct timer {
    unsigned long wait;       /* how long to wait, caller fills */
    unsigned long expr;       /* when the timer expires, tick fils */
    void (*callback)(void *); /* function to call when the timer expires */
    void *ctx;                /* context that is given to the callback */
    list_head_t list;
} timer_t;

/* Initialize the tick counter
 * "ticks" tells how many ticks "ms" milliseconds took */
void tick_init(unsigned long ticks, unsigned long ms);

/* Increase the tick counter */
void tick_inc(void);

/* Busy wait */
void tick_wait(unsigned long ticks);

/* Convert milliseconds, microseconds and nanoseconds to ticks */
unsigned long tick_ms_to_ticks(unsigned long ms);
unsigned long tick_us_to_ticks(unsigned long us);
unsigned long tick_ns_to_ticks(unsigned long ns);

void tick_init_timer(void);

/* Install timer whose callback is called when the timer expires */
void tick_install_timer(timer_t *tmr);

#endif /* __TICK_H__ */

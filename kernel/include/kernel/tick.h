#ifndef __TICK_H__
#define __TICK_H__

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

#endif /* __TICK_H__ */

#ifndef __TIMER_H__
#define __TIMER_H__

#include <stddef.h>

void timer_phase(size_t hz);
void timer_wait(size_t ticks);
void timer_install(void);

#endif /* end of include guard: __TIMER_H__ */

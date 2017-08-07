#ifndef __TIMER_H__
#define __TIMER_H__

void timer_phase(size_t hz);
void timer_wait(int ticks);
void timer_install(void);

#endif /* end of include guard: __TIMER_H__ */

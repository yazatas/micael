#ifndef __PERCPU_H__
#define __PERCPU_H__

#include <kernel/cpu.h>

static unsigned long __percpu_size = 0xdeadbeef;

static inline void percpu_init(int cpu)
{
    extern uint8_t _percpu_start, _percpu_end;

    if (__percpu_size == 0xdeadbeef)
        __percpu_size = (uint64_t)&_percpu_end - (uint64_t)&_percpu_start;

    set_msr(GS_BASE, cpu * __percpu_size);
}

#define __this_cpu_ptr(var)     ((typeof(var) *)(((uint8_t *)(&(var))) + get_msr(GS_BASE)))
#define __any_cpu_ptr(var, cpu) ((typeof(var) *)(((uint8_t *)(&(var))) + cpu * __percpu_size))

#define get_thiscpu_var(var) *(__this_cpu_ptr(var))
#define get_thiscpu_ptr(var)  (__this_cpu_ptr(var))

#define put_thiscpu_var(var) ((void)(var))
#define put_thiscpu_ptr(var) ((void)(var))

#define get_percpu_var(var, cpu) *(__any_cpu_ptr(var, cpu))
#define get_percpu_ptr(var, cpu)  (__any_cpu_ptr(var, cpu))

#define put_percpu_var(var, cpu) ((void)(&(var)))
#define put_percpu_ptr(var, cpu) ((void)(var))

#endif /* __PERCPU_H__ */

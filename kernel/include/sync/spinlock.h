#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

typedef unsigned char spinlock_t;

static inline void spin_acquire(spinlock_t *s)
{
    spinlock_t tmp = 1;

    do {
        asm volatile ("xchgb %0, %1"
                : "=r" (tmp), "=m" (*s)
                : "0"  (tmp), "1"  (*s)
        );
    } while (tmp);
}

static inline void spin_release(spinlock_t *s)
{
    spinlock_t tmp = 0;

    asm volatile ("xchgb %0, %1"
            : "=r" (tmp), "=m" (*s)
            : "0"  (tmp), "1"  (*s)
    );
}

static inline void irq_spin_acquire(spinlock_t *s)
{
    disable_irq();
    spin_acquire(s);
}

static inline void irq_spin_release(spinlock_t *s)
{
    spin_release(s);
    enable_irq();
}

#endif /* __SPINLOCK_H__ */


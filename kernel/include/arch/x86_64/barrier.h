#ifndef __X86_64_BARRIER_H__
#define __X86_64_BARRIER_H__

/* all reads and writes are visible after this */
static inline void barrier_rw(void)
{
    asm volatile ("mfence");
}

/* all reads are visible after this barrier */
static inline void barrier_read(void)
{
    asm volatile ("lfence");
}

/* all memory writes are visible after this barrier */
static inline void barrier_write(void)
{
    asm volatile ("sfence");
}

/* compiler barrier */
static inline void barrier(void)
{
    asm volatile ("" ::: "memory");
}

#endif /* __X86_64_BARRIER_H__ */

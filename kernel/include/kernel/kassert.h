#ifndef __KASSERT_H__
#define __KASSERT_H__

#ifndef NDEBUG
#define kassert(cond)
#else
#define kassert(cond) __kernel_assert(cond)
#define __kernel_assert(cond) \
    if (!(cond)) {            \
        kprint("assertion \"%s\" failed at %s:%d\n", #cond, __FILE__, __LINE__); \
        ktrace(); \
        while (1); \
    }
#endif

#endif /* __KASSERT_H__ */

#ifndef __COMPILER_H__
#define __COMPILER_H__

#define __packed   __attribute__((packed))
#define __align_4k __attribute__((aligned(4096)))
#define __noreturn __attribute__((noreturn))
#define __percpu   __attribute__((section(".percpu")))

#endif /* end of include guard: __COMPILER_H__ */

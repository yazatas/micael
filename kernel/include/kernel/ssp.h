#ifndef __SSP_H__
#define __SSP_H__

#include <stdint.h>
#include <stdlib.h>

#if UINT32_MAX == UINTPTR_MAX
#define STACK_CHK_GUARD 0xdeadbeef
#else
#define STACK_CHK_GUARD 0x123456789abcdef0
#endif

uintptr_t __stack_chk_guard = STACK_CHK_GUARD;

__attribute__((noreturn))
void __stack_chk_fail(void)
{
#if __STDC_HOSTED__
	exit(EXIT_FAILURE);
#elif __is_libk
	abort(); /* TODO: proper kernel panic */
#else
	abort();
#endif
}

#endif /* end of include guard: __SSP_H__ */

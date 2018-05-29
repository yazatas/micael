#ifndef __GPF_H__
#define __GPF_H__

#include <stdint.h>

void gpf_handler(uint32_t error_number) __attribute__((noreturn));

#endif /* end of include guard: __GPF_H__ */

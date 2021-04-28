#ifndef __RANDOM_H__
#define __RANDOM_H__

#include <stdint.h>

uint32_t random_gen32(void);
uint16_t random_gen16(void);
void random_init32(uint32_t seed);

#endif /* __RANDOM_H__ */

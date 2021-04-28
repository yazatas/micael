#include <crypto/random.h>

static uint64_t __next = 1;

uint32_t random_gen32(void)
{
    __next = __next * 1103515245 + 12345;
    return (unsigned int)(__next / 65536) % 32768;
}

uint16_t random_gen16(void)
{
    return random_gen32() & 0xffff;
}

void random_init32(uint32_t seed)
{
    __next = seed;
}

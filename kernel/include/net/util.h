#ifndef __NET_UTIL_H__
#define __NET_UTIL_H__

#include <stdint.h>

static inline uint16_t n2h_16(uint16_t n)
{
    return ((n & 0xff) << 8) | ((n >> 8) & 0xff);
}

static inline uint32_t n2h_32(uint32_t n)
{
    return 
        ((n & 0x000000ff) << 24) |
        ((n & 0x0000ff00) <<  8) |
        ((n & 0x00ff0000) >>  8) |
        ((n & 0xff000000) >> 24);
}

static inline uint16_t h2n_16(uint16_t n)
{
    return ((n & 0xff) << 8) | ((n >> 8) & 0xff);
}

static inline uint32_t h2n_32(uint32_t n)
{
    return 
        ((n & 0x000000ff) << 24) |
        ((n & 0x0000ff00) <<  8) |
        ((n & 0x00ff0000) >>  8) |
        ((n & 0xff000000) >> 24);
}

#endif /* __NET_UTIL_H__ */

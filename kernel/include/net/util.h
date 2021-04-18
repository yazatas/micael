#ifndef __NET_UTIL_H__
#define __NET_UTIL_H__

#include <stdint.h>

enum {
    IPV4_ADDR,
    IPV6_ADDR
};

typedef struct ip {
    uint8_t ipv4[4];
    uint8_t ipv6[16];
    char *addr;
    int type;
} ip_t;

typedef struct mac {
    char *addr;
    union {
        uint8_t b[6];
        uint64_t f:48;
    };
} mac_t;

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

static inline void net_ipv4_addr2bin(char *src, uint8_t *dst)
{
    char *ptr = src;
    int cnt = 0;

    while (*ptr) {
        uint8_t val = 0;

        while (*ptr && *ptr != '.') {
            val *= 10;
            val += *ptr - '0';
            ptr++;
        }

        dst[cnt++] = val;
        ptr++;
    }
}

#endif /* __NET_UTIL_H__ */

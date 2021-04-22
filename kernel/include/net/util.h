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

static ip_t IPV4_BROADCAST = {
    .addr = "255.255.255.255",
    .ipv4 = { 255, 255, 255, 255 },
    .type = IPV4_ADDR
};

static ip_t IPV4_UNSPECIFIED = {
    .addr = "0.0.0.0",
    .ipv4 = { 0, 0, 0, 0 },
    .type = IPV4_ADDR
};

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

static inline void net_ipv4_bin2addr(uint8_t *src, char *dst)
{
    int i, k, p;

    for (i = k = p = 0; i < 4; ++i, p = 0) {
        int val = src[i];

        if (val > 100)
            dst[k + 2] = '0' + val % 10, val /= 10, p++;
        if (val > 10)
            dst[k + 1] = '0' + val % 10, val /= 10, p++;
        dst[k + 0] = '0' + val % 10, val /= 10, p++;

        dst[k + p] = '.';
        k += p + 1;
    }
    dst[k - 1] = '\0';
}

static inline void net_ipv4_print(uint32_t addr)
{
    kprint("%d.%d.%d.%d\n",
        (addr >> (0 * 8) & 0xff),
        (addr >> (1 * 8) & 0xff),
        (addr >> (2 * 8) & 0xff),
        (addr >> (3 * 8) & 0xff)
    );
}

#endif /* __NET_UTIL_H__ */

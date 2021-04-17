#ifndef __IPV4_H__
#define __IPV4_H__

#include <kernel/compiler.h>
#include <kernel/common.h>

typedef struct ipv4_datagram {
    uint8_t dst[6];
    uint8_t src[6];
    uint16_t type;
    char payload[0];
} __packed ipv4_datagram_t;

int ipv4_handle_datagram(ipv4_datagram_t *dgram, size_t size);

#endif /* __IPV4_H__ */

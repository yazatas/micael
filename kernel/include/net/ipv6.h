#ifndef __IPV6_H__
#define __IPV6_H__

#include <kernel/compiler.h>
#include <kernel/common.h>
#include <net/util.h>

typedef struct ipv6_datagram {
    uint8_t version:4;
    uint8_t traffic;
    uint32_t flow:20;
    uint16_t size;
    uint8_t next_hdr;
    uint8_t hop_limit;
    uint8_t src[16];
    uint8_t dst[16];
    char payload[0];
} __packed ipv6_datagram_t;

int ipv6_handle_datagram(ipv6_datagram_t *dgram, size_t size);

int ipv6_send_datagram(ip_t *src, ip_t *dst, void *payload, size_t size);

#endif /* __IPV6_H__ */

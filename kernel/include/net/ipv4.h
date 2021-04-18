#ifndef __IPV4_H__
#define __IPV4_H__

#include <kernel/compiler.h>
#include <kernel/common.h>
#include <net/util.h>

typedef struct ipv4_datagram {
    uint8_t version:4;
    uint8_t ihl:4;
    uint8_t dscp:6;
    uint8_t ecn:2;
    uint16_t len;
    uint16_t iden;
    uint8_t flags:3;
    uint16_t frag_off:13;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t src;
    uint32_t dst;
    char payload[0]; /* may contain IHL */
} __packed ipv4_datagram_t;

int ipv4_handle_datagram(ipv4_datagram_t *dgram, size_t size);

int ipv4_send_datagram(ip_t *src, ip_t *dst, void *payload, size_t size);

#endif /* __IPV4_H__ */

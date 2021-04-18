#ifndef __UDP_H__
#define __UDP_H__

#include <kernel/compiler.h>
#include <kernel/common.h>
#include <net/util.h>

typedef struct udp_pkt {
    uint16_t src;
    uint16_t dst;
    uint16_t len;
    uint16_t checksum;
    char payload[0];
} __packed udp_pkt_t;

int udp_handle_pkt(udp_pkt_t *pkt, size_t size);

int udp_send_pkt(ip_t *src_addr, int src_port, ip_t *dst_addr, int dst_port, void *payload, size_t size);

#endif /* __UDP_H__ */

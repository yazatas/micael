#ifndef __ICMP_H__
#define __ICMP_H__

#include <kernel/compiler.h>
#include <kernel/common.h>

typedef struct icmp_pkt {
    int value;
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    char payload[0];
} __packed icmp_pkt_t;

int icmp_handle_pkt(icmp_pkt_t *pkt, size_t size);

#endif /* __ICMP_H__ */

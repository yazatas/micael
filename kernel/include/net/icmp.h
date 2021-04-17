#ifndef __ICMP_H__
#define __ICMP_H__

#include <kernel/compiler.h>
#include <kernel/common.h>

typedef struct icmp_pkt {
    int value;
    char payload[0];
} __packed icmp_pkt;

int icmp_handle_pkt(icmp_pkt *pkt, size_t size);

#endif /* __ICMP_H__ */

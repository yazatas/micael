#ifndef __UDP_H__
#define __UDP_H__

#include <kernel/compiler.h>
#include <kernel/common.h>

typedef struct udp_pkt {
    int value;
    char payload[0];
} __packed udp_pkt;

int udp_handle_pkt(udp_pkt *pkt, size_t size);

#endif /* __UDP_H__ */

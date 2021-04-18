#ifndef __ETH_H__
#define __ETH_H__

#include <kernel/compiler.h>
#include <kernel/common.h>

enum {
    ETH_TYPE_IPV4 = 0x0800,
    ETH_TYPE_ARP  = 0x0806,
    ETH_TYPE_IPV6 = 0x86dd,
};

typedef struct eth_frame {
    uint8_t dst[6];
    uint8_t src[6];
    uint16_t type;
    char payload[0];
} __packed eth_frame_t;

int eth_handle_frame(eth_frame_t *frame, size_t size);
int eth_send_frame(uint8_t dst[6], uint16_t type, uint8_t *payload, size_t size);

#endif /* __ETH_H__ */

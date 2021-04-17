#ifndef __ETH_H__
#define __ETH_H__

#include <kernel/compiler.h>
#include <kernel/common.h>

typedef struct eth_frame {
    uint8_t dst[6];
    uint8_t src[6];
    uint16_t type;
    char payload[0];
} __packed eth_frame_t;

int eth_handle_frame(eth_frame_t *frame, size_t size);

#endif /* __ETH_H__ */

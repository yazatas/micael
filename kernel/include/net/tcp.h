#ifndef __TCP_H__
#define __TCP_H__

#include <kernel/compiler.h>
#include <kernel/common.h>

typedef struct tcp_pkt {
    int value;
    char payload[0];
} __packed tcp_pkt_t;

int tcp_handle_pkt(tcp_pkt_t *pkt, size_t size);

#endif /* __TCP_H__ */

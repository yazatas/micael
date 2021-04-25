#ifndef __TCP_H__
#define __TCP_H__

#include <kernel/compiler.h>
#include <kernel/common.h>
#include <sync/spinlock.h>

typedef struct tcp_pkt {
    int value;
    char payload[0];
} __packed tcp_pkt_t;

typedef struct tcp_skb {
    spinlock_t lock; /*  */
    size_t size;     /*  */
    void *data;      /*  */
} tcp_skb_t;

int tcp_handle_pkt(tcp_pkt_t *pkt, size_t size);

#endif /* __TCP_H__ */

#ifndef __TCP_H__
#define __TCP_H__

#include <kernel/compiler.h>
#include <kernel/common.h>
#include <sync/spinlock.h>
#include <net/util.h>
#include <net/types.h>

typedef struct tcp_skb {
    spinlock_t lock; /*  */
    size_t size;     /*  */
    void *data;      /*  */
} tcp_skb_t;

int tcp_handle_pkt(tcp_pkt_t *pkt, size_t size);

#endif /* __TCP_H__ */

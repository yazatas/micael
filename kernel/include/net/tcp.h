#ifndef __TCP_H__
#define __TCP_H__

#include <fs/fs.h>
#include <kernel/compiler.h>
#include <kernel/common.h>
#include <net/util.h>
#include <net/types.h>
#include <sync/spinlock.h>

#define SKB_MAX_SIZE 10

typedef struct socket socket_t;

typedef struct tcp_skb {
    spinlock_t lock;                 /* spinlock protecting the socket buffer */
    size_t rptr;                     /* read pointer */
    size_t wptr;                     /* write pointer */
    size_t sptr;                     /* tcp stream ptr */
    size_t npkts;                    /* number of unread packets */
    packet_t *packets[SKB_MAX_SIZE]; /* packets */
} tcp_skb_t;

int tcp_handle_pkt(tcp_pkt_t *pkt, size_t size);

#endif /* __TCP_H__ */

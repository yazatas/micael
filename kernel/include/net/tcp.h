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

int tcp_handle_pkt(packet_t *pkt);
int tcp_send_pkt(packet_t *pkt);
void tcp_init_socket(file_t *fd);
void tcp_init_socket_ops(file_t *fd);
int tcp_read_skb(socket_t *sock, void *buf, size_t size);
int tcp_write_skb(socket_t *sock, packet_t *pkt);

int       tcp_connect(file_t *fd, saddr_in_t *addr, socklen_t addrlen);
int       tcp_listen(file_t *fd, int backlog);
socket_t *tcp_accept(file_t *fd, saddr_in_t *addr, socklen_t *addrlen);

#endif /* __TCP_H__ */

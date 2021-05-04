#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <fs/file.h>
#include <net/tcp.h>
#include <net/types.h>
#include <net/udp.h>
#include <sync/spinlock.h>
#include <sync/wait.h>
#include <sys/socket.h>

typedef struct socket {
    int domain;      /* ipv4/ipv6 */
    int proto;       /* udp/tcp */
    int flags;       /* socket flags */

    ip_t *src_addr;       /* ip address where the socket is bound to */
    short src_port;       /* port where to socket is bound to */
    spinlock_t lock;      /* spinlock protecting the socket */
    wait_queue_head_t wq; /* wait queue used for threads waiting on the socket */
} socket_t;

/* TODO:  */
int socket_init(void);

/* Allocate a socket and save it to "ctx"
 *
 * "domain" specifies ipv4/ipv6, type specifies udp/tcp, and for now,
 * proto must be set to 0
 *
 * Return file descriptor of the socket on success
 * Return -EINVAL if one of parameters is invalid */
int socket_alloc(file_ctx_t *ctx, int domain, int type, int proto);

/* TODO:  */
int socket_handle_pkt(packet_t *pkt);

/* TODO:  */
int socket_bind(file_ctx_t *ctx, int sockfd, saddr_in_t *addr, socklen_t addrlen);

#endif /* __SOCKET_H__ */

#include <errno.h>
#include <fs/fs.h>
#include <kernel/kassert.h>
#include <kernel/kpanic.h>
#include <kernel/tick.h>
#include <kernel/util.h>
#include <lib/bitmap.h>
#include <mm/heap.h>
#include <net/netdev.h>
#include <net/socket.h>
#include <net/udp.h>
#include <sys/socket.h>

#define MAX_SOCKETS 10

struct sock {
    int port;
    bool active;
    socket_t *sock;
} sockets[MAX_SOCKETS];

static int __add_listener(ip_t *src_addr, short src_port, socket_t *sock)
{
    (void)src_addr; /* TODO:  */

    int i = 0;

    for (; i < MAX_SOCKETS && sockets[i].active; ++i)
        ;

    if (i == MAX_SOCKETS)
        return -ENOMEM;

    sockets[i].port   = src_port;
    sockets[i].active = true;
    sockets[i].sock   = sock;

    return 0;
}

int socket_init(void)
{
    kmemset(sockets, 0, sizeof(sockets));
    return 0;
}

int socket_handle_pkt(packet_t *pkt)
{
    int src = ((udp_pkt_t *)pkt->transport.packet)->src;
    int dst = ((udp_pkt_t *)pkt->transport.packet)->dst;
    int ret = -ENOTSUP;

    for (int i = 0; i < MAX_SOCKETS; ++i) {
        if (sockets[i].active && sockets[i].port == dst) {
            if (pkt->transport.proto == PROTO_UDP) {
                ret = udp_write_skb(sockets[i].sock, pkt);
                wq_wakeup(&sockets[i].sock->wq);
                return ret;
            } else if (pkt->transport.proto == PROTO_TCP) {
                ret = tcp_write_skb(sockets[i].sock, pkt);
                wq_wakeup(&sockets[i].sock->wq);
                return ret;
            }
        }
    }

    netdev_dealloc_pkt(pkt);
    return ret;
}

int socket_alloc(file_ctx_t *ctx, int domain, int type, int proto)
{
    kassert(!proto && domain == AF_INET && (type == SOCK_DGRAM || type == SOCK_STREAM));

    if (!ctx || domain != AF_INET || !(type == SOCK_DGRAM || type == SOCK_STREAM) || proto)
        return -EINVAL;

    int fd = vfs_alloc_fd(ctx);

    if (fd < 0) {
        kprint("socket - failed to allocate a file descriptor\n");
        return fd;
    }

    /* initialize a socket object and save it to the file's private data */
    socket_t *sock = kzalloc(sizeof(socket_t));

    sock->domain = domain;
    sock->proto  = type;
    wq_init_head(&sock->wq);

    ctx->fd[fd]->f_private = sock;

    switch (type) {
        case SOCK_DGRAM:
            udp_init_socket(ctx->fd[fd]);
            sock->udp = kzalloc(sizeof(udp_skb_t));
            break;

        case SOCK_STREAM:
            tcp_init_socket(ctx->fd[fd]);
            sock->tcp = kzalloc(sizeof(tcp_skb_t));
            break;
    }

    return fd;
}

int socket_bind(file_ctx_t *ctx, int sockfd, saddr_in_t *addr, socklen_t addrlen)
{
    if (!ctx || sockfd < 2 || sockfd >= ctx->numfd || !addr || !addrlen)
        return -EINVAL;

    /* TODO: validate address & port */

    socket_t *sock = ctx->fd[sockfd]->f_private;
    sock->src_port = addr->sin_port;

    if (addr->sin_addr.s_addr == INADDR_ANY)
        sock->src_addr = netdev_get_ip();
    else
        kpanic("validate address");

    return __add_listener(sock->src_addr, sock->src_port, sock);
}

int socket_send(file_ctx_t *ctx, int sockfd, void *buf, size_t len,
                int flags, saddr_in_t *dest_addr, socklen_t addrlen)
{
    if (!ctx || sockfd < 2 || sockfd >= ctx->numfd || !buf || !len)
        return -EINVAL;

    /* TODO: check that socket is valid */
    /* TODO: make sure len makes sense */
    packet_t *pkt;
    socket_t *sock = ctx->fd[sockfd]->f_private;

    switch (sock->proto) {
        case SOCK_DGRAM:
            pkt = netdev_alloc_pkt_L5(PROTO_IPV4, PROTO_UDP, len);
            break;

        case SOCK_STREAM:
            pkt = netdev_alloc_pkt_L5(PROTO_IPV4, PROTO_TCP, len);
            break;
    }

    pkt->src_addr = sock->src_addr;
    pkt->src_port = sock->src_port;

    if (sock->proto == SOCK_DGRAM) {
        if (!dest_addr || !addrlen) {
            kprint("socket - destination address required for udp sockets!\n");
            return -EINVAL;
        }

        pkt->dst_port = dest_addr->sin_port;
        pkt->dst_addr = kzalloc(sizeof(ip_t));

        kmemcpy(pkt->dst_addr->ipv4, &dest_addr->sin_addr.s_addr, sizeof(uint32_t));
    }

    if (sock->proto == SOCK_STREAM) {
        if (dest_addr || addrlen) {
            kprint("socket - calling sendto(2) for SOCK_STREAM!\n");
            return -ENOTSUP;
        }

        if (!sock->dst_addr || !sock->dst_port) {
            kprint("%d %d\n", sock->dst_addr, sock->dst_port);
            kprint("socket - calling sendto(2) for SOCK_STREAM but socket is not connected!\n");
            return -ENOTSUP;
        }

        pkt->dst_port = sock->dst_port;
        pkt->dst_addr = kzalloc(sizeof(ip_t));

        kmemcpy(pkt->dst_addr->ipv4, sock->dst_addr->ipv4, sizeof(uint32_t));
    }

    kmemcpy(pkt->app.packet, buf, len);
    return file_write(ctx->fd[sockfd], flags, pkt->size, pkt);
}

int socket_recv(file_ctx_t *ctx, int sockfd, void *buf, size_t len,
                int flags, saddr_in_t *dest_addr, socklen_t *addrlen)
{
    if (!ctx || sockfd < 2 || sockfd >= ctx->numfd)
        return -EINVAL;

    kassert(!dest_addr && !addrlen);

    return file_read(ctx->fd[sockfd], flags, len, buf);
}

int socket_connect(file_ctx_t *ctx, int sockfd, saddr_in_t *dest_addr, socklen_t addrlen)
{
    if (!ctx || sockfd < 2 || sockfd >= ctx->numfd)
        return -EINVAL;

    int ret;
    socket_t *sock = ctx->fd[sockfd]->f_private;

    if (sock->proto != SOCK_STREAM)
        return -ENOTSUP;

    if (!sock->src_addr || !sock->src_port || !dest_addr || !addrlen)
        return -EFAULT;

    if ((ret = tcp_connect(ctx->fd[sockfd], dest_addr, addrlen)) < 0) {
        kprint("socket - failed to connect to remote host!");
        return ret;
    }

    sock->dst_port = dest_addr->sin_port;
    sock->dst_addr = kzalloc(sizeof(ip_t));
    kmemcpy(sock->dst_addr->ipv4, &dest_addr->sin_addr.s_addr, sizeof(uint32_t));

    return ret;
}

int socket_listen(file_ctx_t *ctx, int sockfd, int backlog)
{
    if (!ctx || sockfd < 2 || sockfd >= ctx->numfd)
        return -EINVAL;

    socket_t *sock = ctx->fd[sockfd]->f_private;

    if (sock->proto != SOCK_STREAM)
        return -ENOTSUP;

    return tcp_listen(ctx->fd[sockfd], backlog);
}

int socket_accept(file_ctx_t *ctx, int sockfd, saddr_in_t *addr, socklen_t *addrlen)
{
    if (!ctx || sockfd < 2 || sockfd >= ctx->numfd)
        return -EINVAL;

    socket_t *sock = tcp_accept(ctx->fd[sockfd], addr, addrlen);

    if (!sock)
        return -errno;

    int fd = vfs_alloc_fd(ctx);

    if (fd < 0) {
        kprint("socket - failed to allocate a file descriptor\n");
        return fd;
    }

    ctx->fd[fd]->f_private = sock;
    tcp_init_socket_ops(ctx->fd[fd]);
    return fd;
}

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
    }

    return fd;
}

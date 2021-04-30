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

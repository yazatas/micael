#include <errno.h>
#include <kernel/kassert.h>
#include <kernel/util.h>
#include <mm/heap.h>
#include <net/dhcp.h>
#include <net/ipv4.h>
#include <net/ipv6.h>
#include <net/netdev.h>
#include <net/socket.h>
#include <net/udp.h>
#include <sched/sched.h>
#include <sync/wait.h>
#include <sys/socket.h>

#define UDP_PAYLOAD_MAX 65507

static ssize_t __udp_write(file_t *file, off_t flags, size_t size, void *buf)
{
    if (size > UDP_PAYLOAD_MAX)
        return -EINVAL;

    socket_t *sock = file->f_private;
    packet_t *pkt  = buf;

    return udp_send_pkt(pkt);
}

static ssize_t __udp_read(file_t *file, off_t flags, size_t size, void *buf)
{
    int ret;
    socket_t *sock = file->f_private;

    if (!sock->udp->npkts) {
        if (flags & MSG_DONTWAIT)
            return -EAGAIN;

        task_t *current = sched_get_active();
        wq_wait_event(&sock->wq, current, NULL);
    }

    if ((ret = udp_read_skb(sock, buf, size)) < 0) {
        kprint("udp - failed to read a datagram from the socket buffer\n");
        return ret;
    }

    return ret;
}

void udp_init_socket(file_t *fd)
{
    fd->f_ops->read  = __udp_read;
    fd->f_ops->write = __udp_write;

    socket_t *sock  = fd->f_private;
    sock->s_private = NULL;
}

int udp_handle_pkt(packet_t *pkt)
{
    udp_pkt_t *udp = pkt->transport.packet;

    udp->src = n2h_16(udp->src);
    udp->dst = n2h_16(udp->dst);
    udp->len = n2h_16(udp->len);

    pkt->app.packet = udp->payload;
    pkt->app.size   = udp->len - UDP_HDR_SIZE;

    switch (udp->dst) {
        case DHCP_CLIENT_PORT:
            return dhcp_handle_pkt(pkt);
    }
}

int udp_send_pkt(packet_t *pkt)
{
    kassert(pkt);

    udp_pkt_t *udp = pkt->transport.packet;

    udp->src      = h2n_16(pkt->src_port);
    udp->dst      = h2n_16(pkt->dst_port);
    udp->len      = h2n_16(pkt->transport.size);
    udp->checksum = 0;

    if (pkt->net.proto == PROTO_IPV4)
        return ipv4_send_pkt(pkt);
    else
        return ipv6_send_pkt(pkt);
}

int udp_read_skb(socket_t *sock, void *buf, size_t size)
{
    udp_skb_t *skb = sock->udp;

    if (!skb || !buf || !size)
        return -EINVAL;

    if (!skb->npkts)
        return 0;

    spin_acquire(&skb->lock);

    packet_t *pkt   = skb->packets[skb->rptr];
    size_t cpy_size = MIN(size, ((size_t)pkt->app.size));

    kmemcpy(buf, pkt->app.packet, cpy_size);
    netdev_dealloc_pkt(pkt);
    skb->packets[skb->rptr] = NULL;

    skb->npkts--;
    skb->rptr = (skb->rptr + 1) % SKB_MAX_SIZE;

    spin_release(&skb->lock);
    return cpy_size;
}

int udp_write_skb(socket_t *sock, packet_t *pkt)
{
    udp_skb_t *skb = sock->udp;

    if (!skb || !pkt)
        return -EINVAL;

    spin_acquire(&skb->lock);

    if (skb->packets[skb->wptr])
        netdev_dealloc_pkt(skb->packets[skb->wptr]);

    skb->packets[skb->wptr] = pkt;
    skb->wptr = (skb->wptr + 1) % SKB_MAX_SIZE;
    skb->npkts++;

    spin_release(&skb->lock);

    return 0;
}

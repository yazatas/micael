#include <errno.h>
#include <crypto/random.h>
#include <fs/fs.h>
#include <kernel/common.h>
#include <kernel/kassert.h>
#include <kernel/kpanic.h>
#include <kernel/util.h>
#include <mm/heap.h>
#include <net/ipv4.h>
#include <net/ipv6.h>
#include <net/netdev.h>
#include <net/socket.h>
#include <net/tcp.h>
#include <sched/sched.h>
#include <sync/wait.h>

#define TO_U32(ip)        ((ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0])
#define TCP_DATA_OFF(x)   ((x & 0x00F0) >> 4)
#define TCP_NO_OPTS       (5 << 4)

#define TCP_MAX_CLIENTS        5
#define TCP_SYN_RETRIES        5
#define TCP_WINDOW_SIZE     1460
#define TCP_FLAG_MASK     0xff00

enum {
    TCP_FLAG_FIN = 1 <<  8,
    TCP_FLAG_SYN = 1 <<  9,
    TCP_FLAG_RST = 1 << 10,
    TCP_FLAG_PSH = 1 << 11,
    TCP_FLAG_ACK = 1 << 12,
    TCP_FLAG_URG = 1 << 13,
    TCP_FLAG_ECE = 1 << 14,
    TCP_FLAG_CWR = 1 << 15
};

enum {
    TCP_STATE_NONE         = 0 << 0,
    TCP_STATE_PASSIVE      = 1 << 0,
    TCP_STATE_SYN_SENT     = 1 << 1,
    TCP_STATE_SYN_RECEIVED = 1 << 2,
    TCP_STATE_SYN_ACK_SENT = 1 << 3,
    TCP_STATE_PORT_CLOSED  = 1 << 4,
    TCP_STATE_CONNECTED    = 1 << 5,
};

typedef struct tcp_ctx {
    uint32_t seq;
    uint32_t aseq;
    unsigned flags;
    int nclients;

    /* TODO: accepted clients? */
    socket_t *clients[TCP_MAX_CLIENTS];
} tcp_ctx_t;

static uint16_t __calculate_checksum(packet_t *pkt)
{
    struct {
        uint32_t src;
        uint32_t dst;
        uint8_t zero;
        uint8_t pctl;
        uint16_t len;
    } __packed pseudo;

    uint32_t sum   = 0;
    uint16_t carry = 0;

    pseudo.src  = TO_U32(pkt->src_addr->ipv4);
    pseudo.dst  = TO_U32(pkt->dst_addr->ipv4);
    pseudo.zero = 0;
    pseudo.pctl = PROTO_TCP;
    pseudo.len  = h2n_16(pkt->transport.size);

    uint16_t *ptr = (uint16_t *)&pseudo;

    for (size_t i = 0; i < sizeof(pseudo) / 2; ++i)
        sum += ptr[i];

    ptr = (uint16_t *)pkt->transport.packet;

    for (size_t i = 0; i < pkt->transport.size / 2; ++i)
        sum += ptr[i];

    return ~(((sum & 0xff0000) >> 16) + sum);
}

static int __skb_put(tcp_skb_t *skb, packet_t *pkt)
{
    kassert(skb && pkt);
    spin_acquire(&skb->lock);

    if (skb->packets[skb->wptr])
        netdev_dealloc_pkt(skb->packets[skb->wptr]);

    skb->packets[skb->wptr] = pkt;
    skb->wptr = (skb->wptr + 1) % SKB_MAX_SIZE;
    skb->npkts++;

    spin_release(&skb->lock);
    return 0;
}

static packet_t *__skb_get(tcp_skb_t *skb)
{
    kassert(skb && skb->npkts);
    spin_acquire(&skb->lock);

    packet_t *pkt = skb->packets[skb->rptr];
    skb->packets[skb->rptr] = NULL;
    skb->rptr = (skb->rptr + 1) % SKB_MAX_SIZE;
    skb->npkts--;

    spin_release(&skb->lock);
    return pkt;
}

static ssize_t __tcp_write(file_t *file, off_t flags, size_t size, void *buf)
{
    int ret;
    packet_t *pkt  = buf;
    socket_t *sock = file->f_private;
    tcp_pkt_t *tcp = pkt->transport.packet;
    tcp_ctx_t *ctx = sock->s_private;

    tcp->src  = h2n_16(pkt->src_port);
    tcp->dst  = h2n_16(pkt->dst_port);
    tcp->off  = TCP_NO_OPTS | TCP_FLAG_PSH | TCP_FLAG_ACK;
    tcp->len  = h2n_16(TCP_WINDOW_SIZE);
    tcp->seq  = h2n_32(ctx->seq);
    tcp->aseq = h2n_32(ctx->aseq);
    tcp->cks  = 0;
    tcp->cks  = __calculate_checksum(pkt);

    ctx->seq += pkt->transport.size - sizeof(tcp_pkt_t);

    return tcp_send_pkt(pkt);
}

static ssize_t __tcp_read(file_t *file, off_t flags, size_t size, void *buf)
{
    int ret;
    socket_t *sock = file->f_private;
    tcp_skb_t *skb = sock->tcp;

    if (!skb->npkts) {
        task_t *current = sched_get_active();
        wq_wait_event(&sock->wq, current, NULL);
    }

    uint8_t *mem  = NULL;
    packet_t *cur = NULL;
    ssize_t left  = size;
    ssize_t ptr   = 0;
    ssize_t psize = 0;

    while (left > 0 && skb->npkts) {
        cur   = skb->packets[skb->rptr];
        psize = cur->transport.size - sizeof(tcp_pkt_t) - skb->sptr;

        if (psize <= left) {
            cur = __skb_get(skb);
            mem = (uint8_t *)cur->transport.packet + sizeof(tcp_pkt_t) + skb->sptr;
            kmemcpy((uint8_t *)buf, mem, psize);

            left      -= psize;
            ptr       += psize;
            skb->sptr  = 0;
            netdev_dealloc_pkt(cur);
        } else {
            mem = (uint8_t *)cur->transport.packet + sizeof(tcp_pkt_t) + skb->sptr;
            kmemcpy((uint8_t *)buf, mem, left);

            skb->sptr += left;
            ptr       += left;
            left      -= left;
        }
    }

    return ptr;
}

void tcp_init_socket_ops(file_t *fd)
{
    fd->f_ops->read  = __tcp_read;
    fd->f_ops->write = __tcp_write;
}

void tcp_init_socket(file_t *fd)
{
    fd->f_ops->read  = __tcp_read;
    fd->f_ops->write = __tcp_write;

    socket_t *sock  = fd->f_private;
    sock->s_private = kzalloc(sizeof(tcp_ctx_t));
    tcp_ctx_t *ctx = sock->s_private;

    ctx->seq      = 0;
    ctx->aseq     = 0;
    ctx->nclients = 0;
    ctx->flags    = TCP_STATE_NONE;
}

int tcp_handle_pkt(packet_t *pkt)
{
    tcp_pkt_t *tcp = pkt->transport.packet;

    tcp->src  = n2h_16(tcp->src);
    tcp->dst  = n2h_16(tcp->dst);
    tcp->seq  = n2h_32(tcp->seq);
    tcp->aseq = n2h_32(tcp->aseq);
    tcp->len  = n2h_16(tcp->len);
    tcp->cks  = n2h_16(tcp->cks);
    tcp->uptr = n2h_16(tcp->uptr);

    switch (tcp->dst) {
        default:
            return socket_handle_pkt(pkt);
    }
}

int tcp_send_pkt(packet_t *pkt)
{
    if (pkt->net.proto == PROTO_IPV4)
        return ipv4_send_pkt(pkt);
    else
        return ipv6_send_pkt(pkt);
}

static int __handle_connected(socket_t *sock, packet_t *pkt)
{
    tcp_pkt_t *tcp = pkt->transport.packet;
    tcp_skb_t *skb = sock->tcp;
    tcp_ctx_t *ctx = sock->s_private;

    if ((tcp->off & TCP_FLAG_MASK) == (TCP_FLAG_PSH | TCP_FLAG_ACK)) {

        /* packet was lost */
        if (tcp->seq != ctx->aseq) {
            netdev_dealloc_pkt(pkt);
            return 0;
        }
        __skb_put(skb, pkt);

        packet_t *ack = netdev_alloc_pkt_L4(PROTO_IPV4, sizeof(tcp_pkt_t));

        tcp_pkt_t *o_tcp = ack->transport.packet;
        ack->net.proto = PROTO_IPV4;

        ack->src_addr = sock->src_addr;
        ack->dst_addr = sock->dst_addr;

        ack->transport.proto = PROTO_TCP;
        ack->transport.size  = sizeof(tcp_pkt_t);

        o_tcp->src  = h2n_16(sock->src_port);
        o_tcp->dst  = h2n_16(sock->dst_port);
        o_tcp->off  = TCP_NO_OPTS | TCP_FLAG_ACK;
        o_tcp->len  = h2n_16(TCP_WINDOW_SIZE);
        o_tcp->seq  = h2n_32(ctx->seq);
        o_tcp->aseq = h2n_32(ctx->aseq + pkt->transport.size - sizeof(tcp_pkt_t) - 4);
        o_tcp->cks  = 0;
        o_tcp->cks  = __calculate_checksum(ack);

        ctx->aseq += pkt->transport.size - sizeof(tcp_pkt_t) - 4;

        return __send_pkt(sock, ack);
    } else  {
        if ((tcp->off & TCP_FLAG_MASK) == TCP_FLAG_ACK) {
            /* TODO:  */
        } else {
            kprint("unknown packet type %x\n", tcp->off & TCP_FLAG_MASK);
            kassert(1 == 0);
        }
    }
}

static int __handle_accept(socket_t *sock, packet_t *pkt)
{
    tcp_pkt_t *tcp = pkt->transport.packet;
    tcp_skb_t *skb = sock->tcp;
    tcp_ctx_t *ctx = sock->s_private;

    if ((tcp->off & TCP_FLAG_MASK) == TCP_FLAG_SYN) {
        kprint("syn received\n");
        __skb_put(skb, pkt);
        sock->flags |= TCP_STATE_SYN_RECEIVED;
        /* TODO: send syn + ack */
        return 0;
    }
    
    if ((tcp->off & TCP_FLAG_MASK) == TCP_FLAG_ACK) {
        kprint("ack received, connection established\n");
        sock->flags |= TCP_STATE_CONNECTED;
        /* TODO: wq_wakeup() */
        return 0;
    } 
    
    /* TODO: refactor */
    if (ctx->nclients) {
        /* TODO: disgusting */
        for (int i = 0; i < ctx->nclients; ++i) {
            socket_t *__tmp        = ctx->clients[i];
            tcp_ctx_t *__ctx       = __tmp->s_private;
            ipv4_datagram_t *dgram = pkt->net.packet;

            if (h2n_32(dgram->src) == (uint32_t)TO_U32(__tmp->dst_addr->ipv4) &&
                n2h_16(tcp->src) == n2h_16(__tmp->dst_port))
            {
                __skb_put(__tmp->tcp, pkt);
                packet_t *ack = netdev_alloc_pkt_L4(PROTO_IPV4, sizeof(tcp_pkt_t));

                tcp_pkt_t *o_tcp = ack->transport.packet;
                ack->net.proto = PROTO_IPV4;

                ack->src_addr = __tmp->src_addr;
                ack->dst_addr = __tmp->dst_addr;

                ack->transport.proto = PROTO_TCP;
                ack->transport.size  = sizeof(tcp_pkt_t);

                o_tcp->src  = h2n_16(__tmp->src_port);
                o_tcp->dst  = h2n_16(__tmp->dst_port);
                o_tcp->off  = TCP_NO_OPTS | TCP_FLAG_ACK;
                o_tcp->len  = h2n_16(TCP_WINDOW_SIZE);
                o_tcp->seq  = h2n_32(__ctx->seq);
                o_tcp->aseq = h2n_32(__ctx->aseq + pkt->transport.size - sizeof(tcp_pkt_t) - 4);
                o_tcp->cks  = 0;
                o_tcp->cks  = __calculate_checksum(ack);

                __ctx->aseq += pkt->transport.size - sizeof(tcp_pkt_t) - 4;

                __send_pkt(sock, ack);
                wq_wakeup(&__tmp->wq);
                return 0;
            }
        }
    }
}

static int __handle_syn(socket_t *sock, packet_t *pkt)
{
    tcp_pkt_t *tcp = pkt->transport.packet;
    tcp_skb_t *skb = sock->tcp;
    tcp_ctx_t *ctx = sock->s_private;

    /* port is not open */
    if ((tcp->off & TCP_FLAG_MASK) == (TCP_FLAG_ACK | TCP_FLAG_RST)) {
        sock->flags &= ~TCP_STATE_SYN_SENT;
        sock->flags |= TCP_STATE_PORT_CLOSED;
        netdev_dealloc_pkt(pkt);
        wq_wakeup(&sock->wq);
        return -EFAULT;
    }

    /* response to our handshake */
    if ((tcp->off & TCP_FLAG_MASK) == (TCP_FLAG_ACK | TCP_FLAG_SYN)) {
        __skb_put(skb, pkt);
        wq_wakeup(&sock->wq);
        return 0;
    }

    kprint("tcp - discard invalid packet, syn + ack not set: %x\n", tcp->off & TCP_FLAG_MASK);
    netdev_dealloc_pkt(pkt);
    return -EINVAL;
}

int tcp_write_skb(socket_t *sock, packet_t *pkt)
{
    tcp_pkt_t *tcp = pkt->transport.packet;
    tcp_skb_t *skb = sock->tcp;
    tcp_ctx_t *ctx = sock->s_private;

    if (sock->flags & TCP_STATE_SYN_SENT)
        return __handle_syn(sock, pkt);

    if (sock->flags & TCP_STATE_PASSIVE)
        return __handle_accept(sock, pkt);

    if (sock->flags & TCP_STATE_CONNECTED)
        return __handle_connected(sock, pkt);

    kprint("unhandled combination of flags/states: %x\n", sock->flags);
    kassert(NULL);
}

int tcp_connect(file_t *fd, saddr_in_t *dest_addr, socklen_t addrlen)
{
    kassert(fd && dest_addr && addrlen);

    packet_t *pkt   = netdev_alloc_pkt_L4(PROTO_IPV4, sizeof(tcp_pkt_t)), *in_pkt;
    socket_t *sock  = fd->f_private;
    tcp_pkt_t *tcp  = pkt->transport.packet, *in_tcp;
    tcp_ctx_t *ctx  = sock->s_private;
    task_t *current = sched_get_active();

    pkt->src_addr = sock->src_addr;
    pkt->src_port = sock->src_port;
    pkt->dst_port = dest_addr->sin_port;
    pkt->dst_addr = kzalloc(sizeof(ip_t));
    pkt->flags    = NF_REUSE | NF_NO_RTO;

    kmemcpy(pkt->dst_addr->ipv4, &dest_addr->sin_addr.s_addr, sizeof(uint32_t));

    pkt->transport.proto = PROTO_TCP;
    pkt->transport.size  = sizeof(tcp_pkt_t);

    tcp->src = h2n_16(pkt->src_port);
    tcp->dst = h2n_16(pkt->dst_port);
    tcp->off = TCP_NO_OPTS | TCP_FLAG_SYN;
    tcp->len = h2n_16(TCP_WINDOW_SIZE);
    tcp->cks = 0;
    tcp->cks = __calculate_checksum(pkt);

    sock->flags |= TCP_STATE_SYN_SENT;

    tcp_send_pkt(pkt);
    wq_wait_event(&sock->wq, current, NULL);

    if (!sock->tcp->npkts && sock->flags & TCP_STATE_SYN_SENT) {
        if (sock->flags & TCP_STATE_PORT_CLOSED)
            return -EFAULT;
        return -EINVAL;
    }

    in_pkt = __skb_get(sock->tcp);
    in_tcp = in_pkt->transport.packet;

    if (sock->flags & TCP_STATE_CONNECTED)
        goto connected;

    tcp->off  = TCP_NO_OPTS | TCP_FLAG_ACK;
    tcp->seq  = h2n_32(1);
    tcp->aseq = h2n_32(in_tcp->seq + 1);
    tcp->cks  = 0;
    tcp->cks  = __calculate_checksum(pkt);

    /* the connection has been established, update tcp context
     * and save the destination address/port to the socket object */
    sock->flags &= ~TCP_STATE_SYN_SENT;
    sock->flags |=  TCP_STATE_CONNECTED;

    /* send ack and update our tcp session */
    tcp_send_pkt(pkt);

connected:
    ctx->seq   = 1;
    ctx->aseq  = in_tcp->seq + 1;
    ctx->flags = TCP_STATE_CONNECTED;

    sock->dst_addr = kmemdup(pkt->dst_addr, sizeof(ip_t));
    sock->dst_port = dest_addr->sin_port;

    netdev_dealloc_pkt(pkt);
    netdev_dealloc_pkt(in_pkt);

    return 0;
}

int tcp_listen(file_t *fd, int backlog)
{
    kassert(fd);

    socket_t *sock = fd->f_private;
    sock->flags |= TCP_STATE_PASSIVE;

    return 0;
}

socket_t *tcp_accept(file_t *fd, saddr_in_t *addr, socklen_t *addrlen)
{
    socket_t *sock = fd->f_private;
    tcp_skb_t *skb = sock->tcp;
    tcp_ctx_t *ctx = sock->s_private;

    if (!(sock->flags & TCP_STATE_PASSIVE)) {
        errno = ENOTSUP;
        return NULL;
    }

    if (ctx->nclients == TCP_MAX_CLIENTS) {
        errno = ENOMEM;
        return NULL;
    }

    for (;;) {
        for (volatile int k = 0; k < 200000; ++k)
            cpu_relax();

        if (READ_ONCE(sock->flags) & TCP_STATE_SYN_RECEIVED)
            break;
    }

    packet_t *in_pkt  = __skb_get(skb);
    tcp_pkt_t *in_tcp = in_pkt->transport.packet;

    packet_t *out_pkt  = netdev_alloc_pkt_L4(PROTO_IPV4, sizeof(tcp_pkt_t));
    tcp_pkt_t *out_tcp = out_pkt->transport.packet;
    out_pkt->net.proto = PROTO_IPV4;
    out_pkt->flags = NF_REUSE | NF_NO_RTO;

    out_pkt->src_addr = sock->src_addr;
    ipv4_datagram_t *dgram = in_pkt->net.packet;
    out_pkt->dst_addr = kzalloc(sizeof(ip_t));
    kmemcpy(out_pkt->dst_addr->ipv4, &(uint32_t){ n2h_32(dgram->src) }, sizeof(uint32_t));

    out_pkt->transport.proto = PROTO_TCP;
    out_pkt->transport.size  = sizeof(tcp_pkt_t);

    out_tcp->src  = h2n_16(sock->src_port);
    out_tcp->dst  = h2n_16(in_tcp->src);
    out_tcp->off  = TCP_NO_OPTS | TCP_FLAG_SYN | TCP_FLAG_ACK;
    out_tcp->len  = h2n_16(TCP_WINDOW_SIZE);
    out_tcp->seq  = 0;
    out_tcp->aseq = h2n_32(in_tcp->seq + 1);
    out_tcp->cks  = 0;
    out_tcp->cks  = __calculate_checksum(out_pkt);

    for (;;) {
        tcp_send_pkt(out_pkt);

        for (volatile int k = 0; k < 200000; ++k)
            cpu_relax();

        if (READ_ONCE(sock->flags) & TCP_STATE_CONNECTED)
            break;
    }

    socket_t *client = kzalloc(sizeof(socket_t));

    client->src_addr = kzalloc(sizeof(ip_t));
    client->dst_addr = kzalloc(sizeof(ip_t));
    client->src_port = sock->src_port;
    client->dst_port = in_tcp->src;

    kmemcpy(client->src_addr, sock->src_addr,    sizeof(ip_t));
    kmemcpy(client->dst_addr, out_pkt->dst_addr, sizeof(ip_t));

    client->domain = sock->domain;
    client->proto  = sock->proto;
    client->tcp    = kzalloc(sizeof(tcp_skb_t));
    wq_init_head(&client->wq);

    client->s_private = kzalloc(sizeof(tcp_ctx_t));
    tcp_ctx_t *c_ctx = client->s_private;

    c_ctx->seq      = 1;
    c_ctx->aseq     = in_tcp->seq + 1;
    c_ctx->nclients = 0;
    c_ctx->flags    = TCP_STATE_CONNECTED;

    ctx->clients[ctx->nclients] = client;
    return ctx->clients[ctx->nclients++];
}

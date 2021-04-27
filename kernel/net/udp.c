#include <errno.h>
#include <kernel/kassert.h>
#include <kernel/util.h>
#include <mm/heap.h>
#include <net/dhcp.h>
#include <net/ipv4.h>
#include <net/ipv6.h>
#include <net/udp.h>

#define UDP_HDR_SIZE 8

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

int udp_send_pkt_old(ip_t *src_addr, int src_port, ip_t *dst_addr, int dst_port, void *payload, size_t size)
{
    kassert(dst_addr && payload && size);

    if (!dst_addr || !payload || !size) {
        kprint("udp - invalid dst addr (0x%x), payload (0x%x), size (%u) or ports (%d, %d)\n",
                dst_addr, payload, size, dst_port, src_port);
        return -EINVAL;
    }

    int ret;
    udp_pkt_t *pkt = kzalloc(sizeof(udp_pkt_t) + size);

    pkt->src      = h2n_16(src_port);
    pkt->dst      = h2n_16(dst_port);
    pkt->len      = h2n_16(size + UDP_HDR_SIZE);
    pkt->checksum = 0;

    kmemcpy(pkt->payload, payload, size);

    if (dst_addr->type == PROTO_IPV4)
        ret = ipv4_send_pkt(src_addr, dst_addr, pkt, sizeof(udp_pkt_t) + size);
    else
        ret = ipv6_send_pkt(src_addr, dst_addr, pkt, sizeof(udp_pkt_t) + size);

    kfree(pkt);
    return 0;
}

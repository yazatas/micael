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

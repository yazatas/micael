#include <kernel/kpanic.h>
#include <net/icmp.h>
#include <net/ipv6.h>
#include <net/tcp.h>
#include <net/udp.h>

enum {
    IPV6_TYPE_ICMP = 0x01,
    IPV6_TYPE_TCP  = 0x06,
    IPV6_TYPE_UDP  = 0x11,
};

int ipv6_handle_pkt(packet_t *pkt)
{
    ipv6_datagram_t *dgram = pkt->net.packet;

    pkt->transport.packet = dgram->payload;
    pkt->transport.size   = pkt->net.size - sizeof(ipv6_datagram_t);

    switch (dgram->next_hdr) {
        case PROTO_ICMP:
            pkt->transport.proto = PROTO_ICMP;
            return icmp_handle_pkt((icmp_pkt_t *)dgram->payload, n2h_16(dgram->len));

        case PROTO_UDP:
            pkt->transport.proto = PROTO_UDP;
            return udp_handle_pkt(pkt);

        case PROTO_TCP:
            pkt->transport.proto = PROTO_TCP;
            return tcp_handle_pkt((tcp_pkt_t *)dgram->payload, n2h_16(dgram->len));
    }

    return 0;
}

int ipv6_send_pkt(packet_t *pkt)
{
    kpanic("ipv6 new todo");
}

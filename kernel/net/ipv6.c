#include <net/icmp.h>
#include <net/ipv6.h>
#include <net/tcp.h>
#include <net/udp.h>

enum {
    IPV6_TYPE_ICMP = 0x01,
    IPV6_TYPE_TCP  = 0x06,
    IPV6_TYPE_UDP  = 0x11,
};

int ipv6_handle_datagram(ipv6_datagram_t *dgram, size_t size)
{
    switch (dgram->next_hdr) {
        case IPV6_TYPE_ICMP:
            return icmp_handle_pkt((icmp_pkt *)dgram->payload, dgram->size);

        case IPV6_TYPE_UDP:
            return udp_handle_pkt((udp_pkt *)dgram->payload, dgram->size);

        case IPV6_TYPE_TCP:
            return tcp_handle_pkt((tcp_pkt *)dgram->payload, dgram->size);
    }

    return 0;
}

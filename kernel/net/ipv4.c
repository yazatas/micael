#include <errno.h>
#include <kernel/kassert.h>
#include <kernel/kpanic.h>
#include <kernel/util.h>
#include <mm/heap.h>
#include <net/arp.h>
#include <net/eth.h>
#include <net/icmp.h>
#include <net/ipv4.h>
#include <net/tcp.h>
#include <net/udp.h>

static uint16_t __calculate_checksum(uint16_t *header)
{
    uint32_t sum  = 0;
    uint8_t carry = 0;

    for (int i = 0; i < 10; ++i)
        sum += header[i];

    carry = (sum & 0xff0000) >> 16;

    return ~(carry + sum);
}

int ipv4_handle_pkt(packet_t *pkt)
{
    ipv4_datagram_t *dgram = pkt->net.packet;

    pkt->transport.packet = dgram->payload;
    pkt->transport.proto  = dgram->proto;
    pkt->transport.size   = pkt->net.size - 20;

    switch (dgram->proto) {
        case PROTO_ICMP:
            return icmp_handle_pkt((icmp_pkt_t *)dgram->payload, n2h_16(dgram->len));

        case PROTO_UDP:
            return udp_handle_pkt(pkt);

        case PROTO_TCP:
            return tcp_handle_pkt((tcp_pkt_t *)dgram->payload, n2h_16(dgram->len));
    }

    /* kprint("ipv4 - unsupported packet type: 0x%x\n", dgram->proto); */
    return -ENOTSUP;
}

int ipv4_send_pkt_old(ip_t *src, ip_t *dst, void *payload, size_t size)
{
    kassert(dst && payload && size)

    if (!dst || !payload || !size) {
        kprint("ipv4 - invalid dst addr (0x%x), payload (0x%x) or size (%u)\n",
                dst, payload, size);
        return -EINVAL;
    }

    int ret;
    ipv4_datagram_t *dgram = kzalloc(sizeof(ipv4_datagram_t) + size);
    mac_t *eth_dst         = NULL;

    if (!kstrcmp(dst->addr, IPV4_BROADCAST.addr)) {
        eth_dst = &ETH_BROADCAST;
    } else {
        kpanic("ipv4 - resolve address, todo");
    }

    kmemcpy(&dgram->src, src->ipv4, sizeof(dgram->src));
    kmemcpy(&dgram->dst, dst->ipv4, sizeof(dgram->dst));

    dgram->version  = 5;
    dgram->ihl      = 4;
    dgram->len      = h2n_16(sizeof(ipv4_datagram_t) + size);
    dgram->ttl      = 128;
    dgram->proto    = PROTO_UDP; /* TODO:  */
    dgram->checksum = __calculate_checksum((uint16_t *)dgram);

    kmemcpy(&dgram->payload, payload, size);
    ret = eth_send_frame(eth_dst, PROTO_IPV4, dgram, sizeof(ipv4_datagram_t) + size);

    kfree(dgram);
    return ret;
}

int ipv4_send_pkt(packet_t *pkt)
{
    kassert(pkt);

    ipv4_datagram_t *dgram = pkt->net.packet;

    kmemcpy(&dgram->src, pkt->src_addr->ipv4, sizeof(dgram->src));
    kmemcpy(&dgram->dst, pkt->dst_addr->ipv4, sizeof(dgram->dst));

    dgram->version  = 5;
    dgram->ihl      = 4;
    dgram->len      = h2n_16(pkt->net.size);
    dgram->ttl      = 128;
    dgram->proto    = pkt->transport.proto;
    dgram->checksum = __calculate_checksum((uint16_t *)dgram);

    return eth_send_pkt(pkt);
}

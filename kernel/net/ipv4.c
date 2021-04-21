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

enum {
    PROTO_ICMP = 0x01,
    PROTO_TCP  = 0x06,
    PROTO_UDP  = 0x11,
};

uint8_t E2TH_BROADCAST[6] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

static ip_t IPV4_BROADCAST = {
    .addr = "255.255.255.255",
    .ipv4 = { 255, 255, 255, 255 }
};

static uint16_t __calculate_checksum(uint16_t *header)
{
    uint32_t sum  = 0;
    uint8_t carry = 0;

    for (int i = 0; i < 10; ++i)
        sum += header[i];

    carry = (sum & 0xff0000) >> 16;

    return ~(carry + sum);
}

int ipv4_handle_datagram(ipv4_datagram_t *dgram, size_t size)
{
    switch (dgram->protocol) {
        case PROTO_ICMP:
            return icmp_handle_pkt((icmp_pkt_t *)dgram->payload, n2h_16(dgram->len));

        case PROTO_UDP:
            return udp_handle_pkt((udp_pkt_t *)dgram->payload, n2h_16(dgram->len));

        case PROTO_TCP:
            return tcp_handle_pkt((tcp_pkt_t *)dgram->payload, n2h_16(dgram->len));
    }

    return 0;
}

int ipv4_send_datagram(ip_t *src, ip_t *dst, void *payload, size_t size)
{
    kassert(dst && payload && size)

    if (!dst || !payload || !size) {
        kprint("ipv4 - invalid dst addr (0x%x), payload (0x%x) or size (%u)\n",
                dst, payload, size);
        return -EINVAL;
    }

    int ret;
    ipv4_datagram_t *dgram = kzalloc(sizeof(ipv4_datagram_t) + size);
    uint8_t hw_addr[6]     = { 0 };

    if (!kstrcmp(dst->addr, IPV4_BROADCAST.addr)) {
        kmemset(hw_addr, 0xff, sizeof(hw_addr));
    } else {
        kpanic("ipv4 - resolve address, todo");
    }

    kmemcpy(&dgram->src, src->ipv4, sizeof(dgram->src));
    kmemcpy(&dgram->dst, dst->ipv4, sizeof(dgram->dst));

    dgram->version  = 5;
    dgram->ihl      = 4;
    dgram->len      = h2n_16(sizeof(ipv4_datagram_t) + size);
    dgram->ttl      = 128;
    dgram->protocol = PROTO_UDP;
    dgram->checksum = __calculate_checksum((uint16_t *)dgram);

    kmemcpy(&dgram->payload, payload, size);
    ret = eth_send_frame(hw_addr, ETH_TYPE_IPV4, dgram, sizeof(ipv4_datagram_t) + size);

    kfree(dgram);
    return ret;
}

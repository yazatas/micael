#include <errno.h>
#include <kernel/kassert.h>
#include <kernel/kpanic.h>
#include <kernel/util.h>
#include <mm/heap.h>
#include <net/arp.h>
#include <net/eth.h>
#include <net/ipv4.h>
#include <net/udp.h>

enum {
    PROTO_UDP = 0x11,
};

uint8_t E2TH_BROADCAST[6] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

static ip_t IPV4_BROADCAST = {
    .addr = "255.255.255.255",
    .ipv4 = { 255, 255, 255, 255 }
};

int ipv4_handle_datagram(ipv4_datagram_t *dgram, size_t size)
{
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

    dgram->version  = 5;
    dgram->ihl      = 4;
    dgram->len      = h2n_16(sizeof(ipv4_datagram_t) + size);
    dgram->ttl      = 128;
    dgram->protocol = PROTO_UDP;
    dgram->checksum = 0; /* TODO:  */

    kmemcpy(&dgram->src, src->ipv4, sizeof(dgram->src));
    kmemcpy(&dgram->dst, dst->ipv4, sizeof(dgram->dst));
    kmemcpy(&dgram->payload, payload, size);

    ret = eth_send_frame(hw_addr, ETH_TYPE_IPV4, dgram, sizeof(ipv4_datagram_t) + size);

    kfree(dgram);
    return ret;
}

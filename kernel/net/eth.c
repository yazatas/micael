#include <errno.h>
#include <drivers/net/rtl8139.h>
#include <kernel/kassert.h>
#include <kernel/util.h>
#include <mm/heap.h>
#include <net/arp.h>
#include <net/eth.h>
#include <net/ipv4.h>
#include <net/ipv6.h>
#include <net/util.h>

#define ETH_HDR_SIZE 14

int eth_handle_frame(eth_frame_t *frame, size_t size)
{
    switch (n2h_16(frame->type)) {
        case ETH_TYPE_IPV6:
            return ipv6_handle_datagram((ipv6_datagram_t *)frame->payload, size - ETH_HDR_SIZE);

        default:
            kprint("eth - unsupported frame type: 0x%x\n", n2h_16(frame->type));
            return -ENOTSUP;
    }

    return 0;
}

int eth_send_frame(uint8_t dst[6], uint16_t type, uint8_t *payload, size_t size)
{
    kassert(payload && size);

    if (!payload || !size) {
        kprint("eth - payload (0x%x) or size (0x%x) is invalid!");
        return -EINVAL;
    }

    eth_frame_t *eth = kzalloc(sizeof(eth_frame_t) + size);

    kmemcpy(eth->dst, dst, sizeof(eth->dst));
    kmemcpy(eth->src, &(uint64_t) { rtl8139_get_mac() }, sizeof(eth->src));
    kmemcpy(eth->payload, payload, size);

    eth->type = h2n_16(ETH_TYPE_ARP);

    rtl8139_send_pkt((uint8_t *)eth, sizeof(eth_frame_t) + size);

    kfree(eth);
}

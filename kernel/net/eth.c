#include <errno.h>
#include <drivers/net/rtl8139.h>
#include <kernel/kassert.h>
#include <kernel/util.h>
#include <mm/heap.h>
#include <net/arp.h>
#include <net/eth.h>
#include <net/ipv4.h>
#include <net/ipv6.h>
#include <net/netdev.h>
#include <net/util.h>

int eth_handle_frame(packet_t *pkt)
{
    /* update fields appropriately and set net.packet point to eth's payload */
    pkt->link->type = n2h_16(pkt->link->type);
    pkt->net.packet = pkt->link->payload;
    pkt->net.size   = pkt->size - ETH_HDR_SIZE;
    pkt->net.proto  = pkt->link->type;

    switch (pkt->link->type) {
        case PROTO_IPV4:
            return ipv4_handle_pkt(pkt);

        case PROTO_IPV6:
            return ipv6_handle_pkt(pkt);

        case PROTO_ARP:
            return arp_handle_pkt(pkt);
    }

    kprint("eth - unsupported frame type: 0x%x\n", pkt->link->type);
    return -ENOTSUP;
}

int eth_send_pkt(packet_t *pkt)
{
    kassert(pkt);

    eth_frame_t *eth = pkt->link;
    mac_t *dst       = netdev_resolve_address(pkt->dst_addr);

    kmemcpy(eth->dst,                  dst->b, sizeof(eth->dst));
    kmemcpy(eth->src,     netdev_get_mac()->b, sizeof(eth->src));

    eth->type = h2n_16(pkt->net.proto);

    rtl8139_send_pkt((uint8_t *)eth, pkt->size);
    netdev_dealloc_pkt(pkt);

    return 0;
}

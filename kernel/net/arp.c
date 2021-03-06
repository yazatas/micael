#include <errno.h>
#include <drivers/net/rtl8139.h>
#include <kernel/util.h>
#include <lib/hashmap.h>
#include <mm/heap.h>
#include <net/arp.h>
#include <net/eth.h>
#include <net/netdev.h>
#include <net/util.h>

#define HW_ADDR_SIZE     6
#define IPV4_ADDR_SIZE   4
#define IPV6_ADDR_SIZE  16

enum {
    ARP_ETHERNET = 1
};

enum {
    ARP_REQUEST = 1,
    ARP_REPLY   = 2
};

typedef struct arp_ipv6 {
    uint8_t srchw[HW_ADDR_SIZE];
    uint8_t srcpr[IPV6_ADDR_SIZE];
    uint8_t dsthw[HW_ADDR_SIZE];
    uint8_t dstpr[IPV6_ADDR_SIZE];
} __packed arp_ipv6_t;

typedef struct arp_ipv4 {
    uint8_t srchw[HW_ADDR_SIZE];
    uint8_t srcpr[IPV4_ADDR_SIZE];
    uint8_t dsthw[HW_ADDR_SIZE];
    uint8_t dstpr[IPV4_ADDR_SIZE];
} __packed arp_ipv4_t;

static hashmap_t *requests;

int arp_handle_pkt(packet_t *packet)
{
    arp_pkt_t *in_pkt = packet->net.packet;

    if (n2h_16(in_pkt->opcode) == ARP_REQUEST) {

        uint8_t *our_ip   = netdev_get_ip()->ipv4;
        arp_ipv4_t *in_pl = (arp_ipv4_t *)in_pkt->payload;

        if (!our_ip || kmemcmp(our_ip, in_pl->dstpr, sizeof(uint32_t)))
            return -ENOSYS;

        packet_t *pkt    = netdev_alloc_pkt_L3(sizeof(arp_pkt_t) + sizeof(arp_ipv4_t));
        eth_frame_t *eth = pkt->link;

        pkt->net.size  = sizeof(arp_pkt_t) + sizeof(arp_ipv4_t);
        pkt->net.proto = PROTO_ARP;

        arp_pkt_t *arp   = pkt->net.packet;
        arp_ipv4_t *ipv4 = (arp_ipv4_t *)arp->payload;

        arp->htype  = h2n_16(ARP_ETHERNET);
        arp->ptype  = h2n_16(PROTO_IPV4);
        arp->hlen   = HW_ADDR_SIZE;
        arp->plen   = IPV4_ADDR_SIZE;
        arp->opcode = h2n_16(ARP_REPLY);

        kmemcpy(ipv4->srchw, netdev_get_mac()->b,   sizeof(ipv4->srchw));
        kmemcpy(ipv4->srcpr, netdev_get_ip()->ipv4, sizeof(ipv4->srcpr));

        kmemcpy(eth->dst,    in_pl->srchw, sizeof(eth->dst));
        kmemcpy(ipv4->dsthw, in_pl->srchw, sizeof(ipv4->dsthw));
        kmemcpy(ipv4->dstpr, in_pl->srcpr, sizeof(ipv4->dstpr));

        return eth_send_pkt(pkt);

    } else if (n2h_16(in_pkt->opcode) == ARP_REPLY) {
        if (in_pkt->plen == IPV4_ADDR_SIZE) {
            arp_ipv4_t *pl = (arp_ipv4_t *)in_pkt->payload;
            char *addr     = kzalloc(16);

            net_ipv4_bin2addr(pl->srcpr, addr);

            if (hm_get(requests, addr)) {
                netdev_add_ipv4_addr_pair(
                    kmemdup(pl->srchw, HW_ADDR_SIZE),
                    pl->srcpr
                );
                hm_remove(requests, addr);
            }

            kfree(addr);
            return 0;
        }
    } else {
        kprint("arp - got unsupported message type \n");
    }

    return -ENOTSUP;
}

void arp_resolve(char *addr)
{
    if (!addr)
        return;

    if (!requests) {
        if (!(requests = hm_alloc_hashmap(8, HM_KEY_TYPE_STR))) {
            kprint("arp - failed to allocate space for arp request cache!\n");
            return;
        }
    }

    if ((errno = hm_insert(requests, addr, addr)) < 0) {
        kprint("arp - failed to add arp request to cache!\n");
        return;
    }

    packet_t *pkt    = netdev_alloc_pkt_L3(sizeof(arp_pkt_t) + sizeof(arp_ipv4_t));
    eth_frame_t *eth = pkt->link;

    pkt->net.size  = sizeof(arp_pkt_t) + sizeof(arp_ipv4_t);
    pkt->net.proto = PROTO_ARP;

    arp_pkt_t *arp   = pkt->net.packet;
    arp_ipv4_t *ipv4 = (arp_ipv4_t *)arp->payload;

    arp->htype  = h2n_16(ARP_ETHERNET);
    arp->ptype  = h2n_16(PROTO_IPV4);
    arp->hlen   = HW_ADDR_SIZE;
    arp->plen   = IPV4_ADDR_SIZE;
    arp->opcode = h2n_16(ARP_REQUEST);

    kmemcpy(ipv4->srchw, netdev_get_mac()->b, sizeof(ipv4->srchw));
    kmemset(ipv4->srcpr, 0, sizeof(ipv4->srcpr));
    kmemset(ipv4->dsthw, 0, sizeof(ipv4->dsthw));

    net_ipv4_addr2bin(addr, ipv4->dstpr);

    eth_send_pkt(pkt);
}

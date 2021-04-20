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

uint8_t ETH_BROADCAST[6] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

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

int arp_handle_pkt(arp_pkt_t *pkt, size_t size)
{
    kprint("arp - got arp %s\n", n2h_16(pkt->opcode) == 1 ? "request" : "reply");

    return -ENOTSUP;
}

void arp_resolve(char *addr)
{
    if (!addr)
        return;

    if (!requests) {
        if (!(requests = hm_alloc_hashmap(8, HM_KEY_TYPE_STR))) {
            kprint("arp - failed to allocate space for arp request cache!\n");
            return NULL;
        }
    }

    if ((errno = hm_insert(requests, addr, addr)) < 0) {
        kprint("arp - failed to add arp request to cache!\n");
        return;
    }

    size_t size      = sizeof(arp_pkt_t) + sizeof(arp_ipv4_t);
    arp_pkt_t *pkt   = kzalloc(size);
    arp_ipv4_t *ipv4 = (arp_ipv4_t *)pkt->payload;

    pkt->htype  = h2n_16(ARP_ETHERNET);
    pkt->ptype  = h2n_16(ETH_TYPE_IPV4);
    pkt->hlen   = HW_ADDR_SIZE;
    pkt->plen   = IPV4_ADDR_SIZE;
    pkt->opcode = h2n_16(ARP_REQUEST);

    kmemcpy(ipv4->srchw, netdev_get_mac(), sizeof(ipv4->srchw));
    kmemset(ipv4->srcpr, 0, sizeof(ipv4->srcpr));
    kmemset(ipv4->dsthw, 0, sizeof(ipv4->dsthw));

    net_ipv4_addr2bin(addr, ipv4->dstpr);
    eth_send_frame(ETH_BROADCAST, ETH_TYPE_ARP, pkt, size);
    kfree(pkt);
}

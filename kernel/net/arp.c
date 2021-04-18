#include <errno.h>
#include <net/arp.h>
#include <net/eth.h>
#include <net/util.h>

#define HW_ADDR_SIZE     6
#define IPV4_ADDR_SIZE   4
#define IPV6_ADDR_SIZE  16

const uint8_t ETH_BROADCAST[6] = {
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

int arp_handle_pkt(arp_pkt_t *pkt, size_t size)
{
    kprint("arp - got arp %s\n", n2h_16(pkt->opcode) == 1 ? "request" : "reply");

    return -ENOTSUP;
}

int arp_resolve(char *addr)
{
    size_t size      = sizeof(arp_pkt_t) + sizeof(arp_ipv4_t);
    arp_pkt_t *pkt   = kzalloc(size);
    arp_ipv4_t *ipv4 = (arp_ipv4_t *)pkt->payload;

    pkt->htype  = h2n_16(ARP_ETHERNET);
    pkt->ptype  = h2n_16(ETH_TYPE_IPV4);
    pkt->hlen   = HW_ADDR_SIZE;
    pkt->plen   = IPV4_ADDR_SIZE;
    pkt->opcode = h2n_16(ARP_REQUEST);

    kmemcpy(ipv4->srchw, &(uint64_t){ rtl8139_get_mac() }, sizeof(ipv4->srchw));
    kmemset(ipv4->srcpr, 0, sizeof(ipv4->srcpr));
    kmemset(ipv4->dsthw, 0, sizeof(ipv4->dsthw));

    net_ipv4_addr2bin(addr, ipv4->dstpr);

    eth_send_frame(ETH_BROADCAST, ETH_TYPE_ARP, pkt, size);

    kfree(pkt);
}

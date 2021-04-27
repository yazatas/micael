#include <errno.h>
#include <kernel/kpanic.h>
#include <kernel/kassert.h>
#include <kernel/kprint.h>
#include <kernel/util.h>
#include <lib/hashmap.h>
#include <mm/heap.h>
#include <mm/slab.h>
#include <net/arp.h>
#include <net/dhcp.h>
#include <net/eth.h>
#include <net/netdev.h>
#include <net/socket.h>
#include <net/udp.h>

static struct {
    mac_t mac;
    hashmap_t *addrs;
    dhcp_info_t *dhcp;
    ip_t our_ip;
    bool dhcp_done;
} netdev_info;

static mm_cache_t *pkt_cache;

int netdev_init(void)
{
    kprint("net - initializing networking subsystem\n");

    if (!(pkt_cache = mmu_cache_create(sizeof(packet_t), 0)))
        kpanic("Failed to allocate SLAB cache for packets");

    if (!(netdev_info.addrs = hm_alloc_hashmap(32, HM_KEY_TYPE_NUM)))
        kpanic("Failed to allocate cache for address pairs");

    if ((errno = hm_insert(netdev_info.addrs, IPV4_BROADCAST.ipv4, &ETH_BROADCAST)))
        kpanic("netdev - failed to insert address pair to cache\n");

    netdev_info.dhcp_done = false;

    dhcp_discover();
}

void netdev_add_dhcp_info(dhcp_info_t *info)
{
    netdev_info.dhcp = info;
    kmemcpy(netdev_info.our_ip.ipv4, &info->addr, sizeof(uint32_t));

    kprint("netdev - our address: ");
    net_ipv4_print(info->addr);

    kprint("netdev - server address: ");
    net_ipv4_print(info->router);

    kprint("netdev - dns address: ");
    net_ipv4_print(info->router);

    kprint("netdev - broadcast address: ");
    net_ipv4_print(info->router);

    /* save router's ip/mac address pair to address map */
    if ((errno = hm_insert(netdev_info.addrs, &info->router, &info->mac))) {
        kprint("netdev - failed to insert address pair to cache\n");
        return;
    }

    kprint("netdev - lease time: %u seconds, %u hours\n",
            n2h_32(info->lease), n2h_32(info->lease) / 60 / 60);
}

mac_t *netdev_get_mac(void)
{
    return &netdev_info.mac;
}

ip_t *netdev_get_ip(void)
{
    if (!netdev_info.dhcp_done)
        return &netdev_info.our_ip;
}

void netdev_set_mac(uint64_t mac)
{
    netdev_info.mac.b[0] = (mac >> 16) & 0xff;
    netdev_info.mac.b[1] = (mac >> 24) & 0xff;
    netdev_info.mac.b[2] = (mac >> 32) & 0xff;
    netdev_info.mac.b[3] = (mac >> 40) & 0xff;
    netdev_info.mac.b[4] = (mac >>  0) & 0xff;
    netdev_info.mac.b[5] = (mac >>  8) & 0xff;
}

void netdev_add_ipv4_addr_pair(uint8_t *hw, uint8_t *ipv4)
{
    uint32_t ip = (ipv4[0] << 24) | (ipv4[1] << 16) | (ipv4[2] << 8) | ipv4[3];

    if ((errno = hm_insert(netdev_info.addrs, &ip, hw))) {
        kprint("netdev - failed to insert address pair to cache\n");
        return;
    }
}

packet_t *netdev_alloc_pkt_in(size_t size)
{
    kassert(size);

    if (!size)
        return NULL;

    packet_t *pkt = mmu_cache_alloc_entry(pkt_cache, MM_ZERO);
    pkt->link     = kzalloc(size);
    pkt->size     = size;

    return pkt;
}

packet_t *netdev_alloc_pkt_out(int net, int transport, size_t size)
{
    size_t pkt_size       = sizeof(eth_frame_t) + size;
    size_t net_size       = 0;
    size_t transport_size = 0;

    switch (net) {
        case PROTO_IPV4:
            net_size  = sizeof(ipv4_datagram_t);
            pkt_size += sizeof(ipv4_datagram_t);
            break;

        case PROTO_IPV6:
            net_size  = sizeof(ipv6_datagram_t);
            pkt_size += sizeof(ipv6_datagram_t);
            break;

        case PROTO_ARP:
            net_size  = sizeof(arp_pkt_t);
            pkt_size += sizeof(arp_pkt_t);
            break;
    }

    switch (transport) {
        case PROTO_UDP:
            transport_size  = sizeof(udp_pkt_t);
            pkt_size       += sizeof(udp_pkt_t);
            break;

        case PROTO_TCP:
            transport_size  = sizeof(tcp_pkt_t);
            pkt_size       += sizeof(tcp_pkt_t);
            break;

        case PROTO_ICMP:
            transport_size  = sizeof(icmp_pkt_t);
            pkt_size       += sizeof(icmp_pkt_t);
            break;
    }

    /* Allocate packet so that we only need to one allocation and one copy.
     * Set the headers of different layers point to correct locations and
     * set their sizes correctly */
    packet_t *pkt = mmu_cache_alloc_entry(pkt_cache, MM_ZERO);

    pkt->size = pkt_size;
    pkt->link = kzalloc(pkt_size);

    pkt->net.proto  = net;
    pkt->net.size   = pkt_size - sizeof(eth_frame_t);
    pkt->net.packet = (uint8_t *)pkt->link + sizeof(eth_frame_t);

    pkt->transport.proto  = transport;
    pkt->transport.size   = pkt->net.size - net_size;
    pkt->transport.packet = (uint8_t *)pkt->net.packet + net_size;

    pkt->app.proto  = PROTO_UNDEF;
    pkt->app.size   = pkt->transport.size - transport_size;
    pkt->app.packet = (uint8_t *)pkt->transport.packet + transport_size;

    return pkt;
}

int netdev_dealloc_pkt(packet_t *pkt)
{
    kassert(pkt);

    if (!pkt)
        return -EINVAL;

    kfree(pkt->link);
    return mmu_cache_free_entry(pkt_cache, pkt, 0);
}

mac_t *netdev_resolve_address(ip_t *ip)
{
    mac_t *ret = NULL;

    if (!(ret = hm_get(netdev_info.addrs, ip->ipv4)))
        ret = &netdev_info.dhcp->mac;

    return ret;
}

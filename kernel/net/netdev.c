#include <errno.h>
#include <kernel/kprint.h>
#include <kernel/util.h>
#include <lib/hashmap.h>
#include <net/arp.h>
#include <net/dhcp.h>
#include <net/eth.h>
#include <net/netdev.h>
#include <net/udp.h>

static struct {
    mac_t mac;
    hashmap_t *addrs;
    dhcp_info_t *dhcp;
    ip_t our_ip;
    bool dhcp_done;
} netdev_info;

int netdev_init(void)
{
    kprint("net - initializing networking subsystem\n");

    if (!(netdev_info.addrs = hm_alloc_hashmap(32, HM_KEY_TYPE_NUM))) {
        kprint("netdev - failed to allocate cache for address pairs\n");
        return -ENOMEM;
    }
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

    kprint("netdev - lease time: %u seconds, %u hours\n",
            n2h_32(info->lease), n2h_32(info->lease) / 60 / 60);
}

mac_t *netdev_get_mac(void)
{
    return &netdev_info.mac;
}

uint8_t *netdev_get_ipv4(void)
{
    if (!netdev_info.dhcp_done)
        return netdev_info.our_ip.ipv4;
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

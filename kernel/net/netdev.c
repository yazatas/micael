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
    uint8_t mac[8]; /* TODO: use mac_t */
    hashmap_t *addrs;
} netdev_info;

int netdev_init(void)
{
    kprint("net - initializing networking subsystem\n");

    if (!(netdev_info.addrs = hm_alloc_hashmap(32, HM_KEY_TYPE_NUM))) {
        kprint("netdev - failed to allocate cache for address pairs\n");
        return -ENOMEM;
    }
}

uint8_t *netdev_get_mac(void)
{
    return netdev_info.mac;
}

void netdev_set_mac(uint64_t mac)
{
    netdev_info.mac[0] = (mac >> 16) & 0xff;
    netdev_info.mac[1] = (mac >> 24) & 0xff;
    netdev_info.mac[2] = (mac >> 32) & 0xff;
    netdev_info.mac[3] = (mac >> 40) & 0xff;
    netdev_info.mac[4] = (mac >>  0) & 0xff;
    netdev_info.mac[5] = (mac >>  8) & 0xff;
}

void netdev_add_ipv4_addr_pair(uint8_t *hw, uint8_t *ipv4)
{
    uint32_t ip = (ipv4[0] << 24) | (ipv4[1] << 16) | (ipv4[2] << 8) | ipv4[3];

    if ((errno = hm_insert(netdev_info.addrs, &ip, hw))) {
        kprint("netdev - failed to insert address pair to cache\n");
        return;
    }
}

#include <kernel/kprint.h>
#include <kernel/util.h>
#include <net/arp.h>
#include <net/dhcp.h>
#include <net/eth.h>
#include <net/netdev.h>
#include <net/udp.h>

static struct {
    uint8_t mac[8];
} netdev_info;

int netdev_init(void)
{
    kprint("net - initializing networking subsystem\n");
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

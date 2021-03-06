#ifndef __NET_H__
#define __NET_H__

#include <net/dhcp.h>
#include <net/types.h>

typedef struct netdev_hw_info {
    uint64_t mac;
    size_t mtu;
} netdev_hw_info_t;

int netdev_init(void);
void netdev_set_hw_info(netdev_hw_info_t hwinfo);
void netdev_add_ipv4_addr_pair(uint8_t *hw, uint8_t *ipv4);
void netdev_add_dhcp_info(dhcp_info_t *info);

mac_t *netdev_get_mac(void);
ip_t  *netdev_get_ip(void);
size_t netdev_get_mtu(void);

/* Allocate memory for an incoming packet */
packet_t *netdev_alloc_pkt_in(size_t size);

/* Allocate memory for an outgoing packet */
packet_t *netdev_alloc_pkt_L5(int net, int transport, size_t size);
packet_t *netdev_alloc_pkt_L4(int net, size_t size);
packet_t *netdev_alloc_pkt_L3(size_t size);

/* Free memory occupied by the packet */
int netdev_dealloc_pkt(packet_t *pkt);

/* Return the MAC address of an IP address */
mac_t *netdev_resolve_address(ip_t *ip);

#endif /* __NET_H__ */

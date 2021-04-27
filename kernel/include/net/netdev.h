#ifndef __NET_H__
#define __NET_H__

#include <net/dhcp.h>
#include <net/types.h>

int netdev_init(void);
void netdev_set_mac(uint64_t mac);
void netdev_add_ipv4_addr_pair(uint8_t *hw, uint8_t *ipv4);
void netdev_add_dhcp_info(dhcp_info_t *info);

mac_t *netdev_get_mac(void);
ip_t  *netdev_get_ip(void);

/* Allocate memory for an incoming packet */
packet_t *netdev_alloc_pkt_in(size_t size);

/* Allocate memory for an outgoing packet */
packet_t *netdev_alloc_pkt_out(int net, int transport, size_t size);

/* Free memory occupied by the packet */
int netdev_dealloc_pkt(packet_t *pkt);

#endif /* __NET_H__ */

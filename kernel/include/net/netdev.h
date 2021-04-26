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

packet_t *netdev_alloc_pkt(void *mem, size_t size);
void      netdev_dealloc_pkt(packet_t *pkt);

#endif /* __NET_H__ */

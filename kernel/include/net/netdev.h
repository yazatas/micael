#ifndef __NET_H__
#define __NET_H__

int netdev_init(void);
void netdev_set_mac(uint64_t mac);
void netdev_add_ipv4_addr_pair(uint8_t *hw, uint8_t *ipv4);
uint8_t *netdev_get_mac(void);

#endif /* __NET_H__ */

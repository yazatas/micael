#ifndef __NET_H__
#define __NET_H__

int netdev_init(void);
void netdev_set_mac(uint64_t mac);
uint8_t *netdev_get_mac(void);

#endif /* __NET_H__ */

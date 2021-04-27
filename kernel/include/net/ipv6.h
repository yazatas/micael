#ifndef __IPV6_H__
#define __IPV6_H__

#include <kernel/compiler.h>
#include <kernel/common.h>
#include <net/util.h>
#include <net/types.h>

int ipv6_handle_pkt(packet_t *pkt);

int ipv6_send_pkt_old(ip_t *src, ip_t *dst, void *payload, size_t size);

int ipv6_send_pkt(packet_t *pkt);

#endif /* __IPV6_H__ */

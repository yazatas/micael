#ifndef __IPV6_H__
#define __IPV6_H__

#include <kernel/compiler.h>
#include <kernel/common.h>
#include <net/util.h>
#include <net/types.h>

int ipv6_handle_pkt(packet_t *pkt);

int ipv6_send_pkt(ip_t *src, ip_t *dst, void *payload, size_t size);

#endif /* __IPV6_H__ */

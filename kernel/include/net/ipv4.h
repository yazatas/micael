#ifndef __IPV4_H__
#define __IPV4_H__

#include <kernel/compiler.h>
#include <kernel/common.h>
#include <net/util.h>
#include <net/types.h>

int ipv4_handle_pkt(packet_t *pkt);

int ipv4_send_pkt(ip_t *src, ip_t *dst, void *payload, size_t size);

#endif /* __IPV4_H__ */

#ifndef __UDP_H__
#define __UDP_H__

#include <kernel/compiler.h>
#include <kernel/common.h>
#include <net/util.h>
#include <net/types.h>

/* TODO:  */
int udp_handle_pkt(packet_t *pkt);

/* TODO:  */
int udp_send_pkt(ip_t *src_addr, int src_port, ip_t *dst_addr, int dst_port, void *payload, size_t size);

#endif /* __UDP_H__ */

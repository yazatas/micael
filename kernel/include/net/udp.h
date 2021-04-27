#ifndef __UDP_H__
#define __UDP_H__

#include <kernel/compiler.h>
#include <kernel/common.h>
#include <net/util.h>
#include <net/types.h>

/* TODO:  */
int udp_handle_pkt(packet_t *pkt);


int udp_send_pkt(packet_t *pkt);

#endif /* __UDP_H__ */

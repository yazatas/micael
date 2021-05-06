#ifndef __ETH_H__
#define __ETH_H__

#include <kernel/compiler.h>
#include <kernel/common.h>
#include <net/types.h>

int eth_handle_frame(packet_t *pkt);
int eth_send_pkt(packet_t *pkt);

#endif /* __ETH_H__ */

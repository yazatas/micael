#ifndef __ICMP_H__
#define __ICMP_H__

#include <kernel/compiler.h>
#include <kernel/common.h>
#include <net/types.h>

int icmp_handle_pkt(icmp_pkt_t *pkt, size_t size);

#endif /* __ICMP_H__ */

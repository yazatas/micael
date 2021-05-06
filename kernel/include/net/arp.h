#ifndef __ARP_H__
#define __ARP_H__

#include <kernel/compiler.h>
#include <kernel/common.h>
#include <net/types.h>
#include <net/types.h>
#include <net/util.h>

/* Handle ARP packet
 *
 * Return 0 on success
 * Return -ENOTSUP if "pkt" is not supported
 * Return -EINVAL if "pkt" contains an invalid value */
int arp_handle_pkt(packet_t *pkt);

/* Resolve the physical (MAC) address of an IP address
 *
 * This function returns immediately, the resolving
 * is done asynchronously and the result is reported to netdev */
void arp_resolve(char *addr);

#endif /* __ARP_H__ */

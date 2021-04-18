#ifndef __ARP_H__
#define __ARP_H__

#include <kernel/compiler.h>
#include <kernel/common.h>
#include <net/util.h>

typedef struct arp_pkt {
    uint16_t htype;
    uint16_t ptype;
    uint8_t  hlen;
    uint8_t  plen;
    uint16_t opcode;
    char payload[0];
} __packed arp_pkt_t;

/* Handle ARP packet
 *
 * Return 0 on success
 * Return -ENOTSUP if "pkt" is not supported
 * Return -EINVAL if "pkt" contains an invalid value */
int arp_handle_pkt(arp_pkt_t *pkt, size_t size);

mac_t *arp_resolve(char *addr);

#endif /* __ARP_H__ */

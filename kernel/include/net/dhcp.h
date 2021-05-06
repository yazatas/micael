#ifndef __DHCP_H__
#define __DHCP_H__

#include <kernel/compiler.h>
#include <kernel/common.h>
#include <net/types.h>
#include <net/util.h>

#define DHCP_SERVER_PORT  67
#define DHCP_CLIENT_PORT  68

typedef struct dhcp_pkt {
    uint8_t op;         /* opcode */
    uint8_t htype;      /* hardware address type */
    uint8_t hlen;       /* hardware address length */
    uint8_t hops;       /* private relay agent data */
    uint32_t xid;       /* random transaction id */
    uint16_t secs;      /* seconds elapsed */
    uint16_t flags;     /* flags */
    uint32_t ciaddr;    /* client ip address */
    uint32_t yiaddr;    /* your ip address */
    uint32_t siaddr;    /* server ip address */
    uint32_t giaddr;    /* relay agent ip address */
    uint64_t chaddr;    /* client mac address */
    uint8_t zero[8];    /* zero */
    char sname[64];     /* server host name */
    char file[128];     /* boot file name */
    uint32_t cookie;    /* magic cookie */
    uint8_t payload[0]; /* options */
} __packed dhcp_pkt_t;

typedef struct dhcp_info {
    uint32_t addr;      /* our ipv4 address */
    uint32_t router;    /* ipv4 of router */
    uint32_t dns;       /* ipv4 of dns */
    uint32_t broadcast; /* ipv4 of broadcast */
    uint32_t sb_mask;   /* subnet mask */
    uint32_t lease;     /* lease time */
    mac_t    mac;
} dhcp_info_t;

int dhcp_discover(void);
int dhcp_handle_pkt(packet_t *pkt);

#endif /* __DHCP_H__ */

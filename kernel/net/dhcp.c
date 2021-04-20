#include <drivers/net/rtl8139.h>
#include <kernel/util.h>
#include <mm/heap.h>
#include <net/dhcp.h>
#include <net/udp.h>
#include <net/util.h>

#define DHCP_SERVER_PORT     68
#define DHCP_CLIENT_PORT     67

#define END_PADDING          28
#define DHCP_COOKIE  0x63825363

enum {
    OP_REQUEST = 1,
    OP_REPLY   = 2
};

enum {
    OPT_PAD               =   0,
    OPT_SUBNET_MASK       =   1,
    OPT_ROUTER            =   3,
    OPT_DNS               =   6,
    OPT_HOST_NAME         =  12,
    OPT_REQUESTED_IP_ADDR =  50,
    OPT_LEASE_TIME        =  51,
    OPT_MESSAGE_TYPE      =  53,
    OPT_SERVER_ID         =  54,
    OPT_PARAMETER_REQUEST =  55,
    OPT_RENEWAL_TIME      =  58,
    OPT_REBINDING_TIME    =  59,
    OPT_END               = 255
};

enum {
    MSG_DISCOVER = 1,
    MSG_OFFER    = 2,
    MSG_REQUEST  = 3,
    MSG_DECLINE  = 4,
    MSG_ACK      = 5,
    MSG_NAK      = 6,
    MSG_RELEASE  = 7,
    MSG_INFORM   = 8
};

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

typedef struct dhcp_options {
    ip_t *subnet_mask;
    ip_t *router_list;
    uint32_t router_len;
    ip_t *dns_list;
    uint32_t dns_len;
    ip_t *requested_ip;
    uint32_t lease;
    uint32_t message_type;
    ip_t *server;
    uint8_t *param_list;
    uint32_t param_len;
} dhcp_options_t;

int dhcp_discover(void)
{
    ip_t dst = {
        .addr = "255.255.255.255",
        .ipv4 = { 255, 255, 255, 255 },
        .type = IPV4_ADDR
    };

    ip_t src = {
        .addr = "0.0.0.0",
        .ipv4 = { 0, 0, 0, 0 },
        .type = IPV4_ADDR
    };

    int ret;
    dhcp_pkt_t *pkt = kzalloc(sizeof(dhcp_pkt_t) + 9);

    pkt->op     = OP_REQUEST;
    pkt->htype  = 1; /* eth */
    pkt->hlen   = 6;
    pkt->xid    = h2n_32(0x13371338);
    pkt->cookie = h2n_32(DHCP_COOKIE);

    kmemcpy(&pkt->chaddr, netdev_get_mac(), pkt->hlen);

    /* craft dhcp options */
    pkt->payload[0] = OPT_MESSAGE_TYPE;
    pkt->payload[1] = 1;
    pkt->payload[2] = MSG_DISCOVER;

    pkt->payload[3] = OPT_PARAMETER_REQUEST;
    pkt->payload[4] = 3;
    pkt->payload[5] = OPT_SUBNET_MASK;
    pkt->payload[6] = OPT_ROUTER;
    pkt->payload[7] = OPT_DNS;

    pkt->payload[8] = OPT_END;

    ret = udp_send_pkt(&src, DHCP_CLIENT_PORT, &dst, DHCP_SERVER_PORT, pkt, sizeof(dhcp_pkt_t) + 9);

    kfree(pkt);
    return ret;
}

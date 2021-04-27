#ifndef __NET_TYPES_H__
#define __NET_TYPES_H__

enum NETWORK {
    PROTO_IPV4 = 0x0800,
    PROTO_ARP  = 0x0806,
    PROTO_IPV6 = 0x86dd
};

enum TRANSPORT {
    PROTO_ICMP = 0x01,
    PROTO_TCP  = 0x06,
    PROTO_UDP  = 0x11
};

enum APPLICATION {
    PROTO_UNDEF,
    PROTO_DHCP
};

enum HEADER_SIZES {
    ETH_HDR_SIZE  = 14,
    IPV4_HDR_SIZE = 20,
    UDP_HDR_SIZE  =  8
};

typedef uint16_t proto_t;

typedef struct ip {
    uint8_t ipv4[4];
    uint8_t ipv6[16];
    char *addr;
    int type;
} ip_t;

typedef struct mac {
    char *addr;
    union {
        uint8_t b[6];
        uint64_t f:48;
    };
} mac_t;

typedef struct eth_frame {
    uint8_t dst[6];
    uint8_t src[6];
    uint16_t type;
    char payload[0];
} __packed eth_frame_t;

typedef struct icmp_pkt {
    int value;
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    char payload[0];
} __packed icmp_pkt_t;

typedef struct ipv4_datagram {
    uint8_t version:4;
    uint8_t ihl:4;
    uint8_t dscp:6;
    uint8_t ecn:2;
    uint16_t len;
    uint16_t iden;
    uint8_t flags:3;
    uint16_t frag_off:13;
    uint8_t ttl;
    uint8_t proto;
    uint16_t checksum;
    uint32_t src;
    uint32_t dst;
    char payload[0]; /* may contain IHL */
} __packed ipv4_datagram_t;

typedef struct ipv6_datagram {
    uint8_t version:4;
    uint8_t traffic;
    uint32_t flow:20;
    uint16_t len;
    uint8_t next_hdr;
    uint8_t hop_limit;
    uint8_t src[16];
    uint8_t dst[16];
    char payload[0];
} __packed ipv6_datagram_t;

typedef struct arp_pkt {
    uint16_t htype;
    uint16_t ptype;
    uint8_t  hlen;
    uint8_t  plen;
    uint16_t opcode;
    char payload[0];
} __packed arp_pkt_t;

typedef struct udp_pkt {
    uint16_t src;
    uint16_t dst;
    uint16_t len;
    uint16_t checksum;
    char payload[0];
} __packed udp_pkt_t;

typedef struct tcp_pkt {
    int value;
    char payload[0];
} __packed tcp_pkt_t;

typedef struct packet {

    short size;
    eth_frame_t *link;

    /* Connectivity information, either filled by
     * the socket or network/transport layers */
    ip_t *src_addr, *dst_addr;
    unsigned short src_port, dst_port;

    struct {
        proto_t proto;
        short size;
        void *packet;
    } __packed net;

    struct {
        proto_t proto;
        short size;
        void *packet;
    } __packed transport;

    struct {
        proto_t proto;
        short size;
        void *packet;
    } __packed app;

} __packed packet_t;

static ip_t IPV4_BROADCAST = {
    .addr = "255.255.255.255",
    .ipv4 = { 255, 255, 255, 255 },
    .type = PROTO_IPV4
};

static ip_t IPV4_UNSPECIFIED = {
    .addr = "0.0.0.0",
    .ipv4 = { 0, 0, 0, 0 },
    .type = PROTO_IPV4
};

static mac_t ETH_BROADCAST = {
    .addr = "FF:FF:FF:FF:FF:FF",
    .b    = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }
};

#endif /* __NET_TYPES_H__ */

#include <errno.h>
#include <drivers/net/rtl8139.h>
#include <lib/hashmap.h>
#include <kernel/kassert.h>
#include <kernel/util.h>
#include <mm/heap.h>
#include <net/dhcp.h>
#include <net/udp.h>
#include <net/util.h>
#include <net/netdev.h>

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
    OPT_BROADCAST         =  28,
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

static hashmap_t *requests;

static packet_t *__alloc_dhcp_pkt(size_t size)
{
    packet_t *pkt    = netdev_alloc_pkt_out(PROTO_IPV4, PROTO_UDP, sizeof(dhcp_pkt_t) + size);
    dhcp_pkt_t *dhcp = pkt->app.packet;

    dhcp->op     = OP_REQUEST;
    dhcp->htype  = 1; /* eth */
    dhcp->hlen   = 6;
    dhcp->xid    = h2n_32(0x13371338);
    dhcp->cookie = h2n_32(DHCP_COOKIE);

    kmemcpy(&dhcp->chaddr, netdev_get_mac()->b, dhcp->hlen);

    return pkt;
}

static int __handle_ack(packet_t *in_pkt)
{
    dhcp_pkt_t *pkt   = in_pkt->app.packet;
    dhcp_info_t *info = hm_get(requests, &pkt->xid);

    if (!info) {
        kprint("dhcp - received ack but no request sent!\n");
        return -ENOENT;
    }

    hm_remove(requests, &pkt->xid);
    netdev_add_dhcp_info(info);
}

static int __handle_offer(packet_t *in_pkt)
{
    dhcp_pkt_t *in_dhcp = in_pkt->app.packet;
    dhcp_info_t *info   = hm_get(requests, &in_dhcp->xid);

    if (!info) {
        kprint("dhcp - received offer but no discover sent!\n");
        return -ENOENT;
    } else {
        for (short i = 0; i < in_pkt->app.size;) {
            switch (in_dhcp->payload[i]) {
                case OPT_SUBNET_MASK:
                    kmemcpy(&info->sb_mask, &in_dhcp->payload[i + 2], in_dhcp->payload[i + 1]);
                    break;

                case OPT_BROADCAST:
                    kmemcpy(&info->broadcast, &in_dhcp->payload[i + 2], in_dhcp->payload[i + 1]);
                    break;

                case OPT_ROUTER:
                    kmemcpy(&info->router, &in_dhcp->payload[i + 2], in_dhcp->payload[i + 1]);
                    break;

                case OPT_DNS:
                    kmemcpy(&info->dns, &in_dhcp->payload[i + 2], in_dhcp->payload[i + 1]);
                    break;

                case OPT_LEASE_TIME:
                    kmemcpy(&info->lease, &in_dhcp->payload[i + 2], in_dhcp->payload[i + 1]);
                    break;

                case OPT_END:
                    goto end;

                default:
                    break;
            }

            i += 2 + in_dhcp->payload[i + 1];
        }
end:
        kmemcpy(&info->addr, &in_dhcp->yiaddr, sizeof(uint32_t));

        /* If the sender of the packet is the same as the server, save the mac address */
        kassert(in_pkt->net.proto == PROTO_IPV4);
        ipv4_datagram_t *dgram = in_pkt->net.packet;
        eth_frame_t *eth       = in_pkt->link;

        if (!kmemcmp(&dgram->src,  &info->router, sizeof(uint32_t)))
             kmemcpy(info->mac.b,   eth->src,     sizeof(info->mac));
    }

    int ret;
    packet_t *out_pkt = __alloc_dhcp_pkt(10);
    dhcp_pkt_t *dhcp  = out_pkt->app.packet;

    ipv4_datagram_t *dgram = out_pkt->net.packet;

    dhcp->yiaddr = in_dhcp->yiaddr;
    dhcp->siaddr = in_dhcp->siaddr;

    /* craft dhcp options */
    dhcp->payload[0] = OPT_MESSAGE_TYPE;
    dhcp->payload[1] = 1;
    dhcp->payload[2] = MSG_REQUEST;

    dhcp->payload[3] = OPT_REQUESTED_IP_ADDR;
    dhcp->payload[4] = 4;
    kmemcpy(&dhcp->payload[5], &dhcp->yiaddr, sizeof(dhcp->yiaddr));

    dhcp->payload[9] = OPT_END;

    out_pkt->src_addr = &IPV4_UNSPECIFIED;
    out_pkt->src_port = DHCP_CLIENT_PORT;

    out_pkt->dst_addr = &IPV4_BROADCAST;
    out_pkt->dst_port = DHCP_SERVER_PORT;

    kmemcpy(out_pkt->app.packet, dhcp, sizeof(dhcp_pkt_t) + 10);
    return udp_send_pkt(out_pkt);
}

int dhcp_discover(void)
{
    if (!requests) {
        /* TODO: move this code to netdev? */
        if (!(requests = hm_alloc_hashmap(4, HM_KEY_TYPE_NUM))) {
            kprint("dhcp - failed to allocate space for dhcp request cache!\n");
            return -ENOMEM;
        }
    }

    int ret;
    packet_t *pkt        = __alloc_dhcp_pkt(9);
    dhcp_pkt_t *dhcp     = pkt->app.packet;
    dhcp_options_t *opts = kzalloc(sizeof(dhcp_options_t));

    if ((errno = hm_insert(requests, &dhcp->xid, opts)) < 0) {
        kprint("dhcp - failed to cache request!\n");
        return ret;
    }

    /* craft dhcp options */
    dhcp->payload[0] = OPT_MESSAGE_TYPE;
    dhcp->payload[1] = 1;
    dhcp->payload[2] = MSG_DISCOVER;

    dhcp->payload[3] = OPT_PARAMETER_REQUEST;
    dhcp->payload[4] = 3;
    dhcp->payload[5] = OPT_SUBNET_MASK;
    dhcp->payload[6] = OPT_ROUTER;
    dhcp->payload[7] = OPT_DNS;

    dhcp->payload[8] = OPT_END;

    pkt->src_addr = &IPV4_UNSPECIFIED;
    pkt->src_port = DHCP_CLIENT_PORT;

    pkt->dst_addr = &IPV4_BROADCAST;
    pkt->dst_port = DHCP_SERVER_PORT;

    kmemcpy(pkt->app.packet, dhcp, sizeof(dhcp_pkt_t) + 9);

    return udp_send_pkt(pkt);
}

int dhcp_handle_pkt(packet_t *in_pkt)
{
    int ret;
    dhcp_pkt_t *pkt = in_pkt->app.packet;

    switch (pkt->payload[2]) {
        case MSG_OFFER:
            ret = __handle_offer(in_pkt);
            break;

        case MSG_ACK:
            ret = __handle_ack(in_pkt);
            break;

        case MSG_NAK:
            kprint("dhcp - handle nak\n");
            ret = -ENOTSUP;
            break;

        default:
            kprint("dhcp - unsupported message received\n");
            ret = -ENOTSUP;
            break;
    }

    netdev_dealloc_pkt(in_pkt);
    return ret;
}

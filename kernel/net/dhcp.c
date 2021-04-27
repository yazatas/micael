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

static dhcp_pkt_t *__alloc_pkt(size_t size)
{
    dhcp_pkt_t *pkt = kzalloc(sizeof(dhcp_pkt_t) + size);

    pkt->op     = OP_REQUEST;
    pkt->htype  = 1; /* eth */
    pkt->hlen   = 6;
    pkt->xid    = h2n_32(0x13371338);
    pkt->cookie = h2n_32(DHCP_COOKIE);

    kmemcpy(&pkt->chaddr, netdev_get_mac()->b, pkt->hlen);

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

static int __handle_offer(packet_t *pkt)
{
    dhcp_pkt_t *in_pkt = pkt->app.packet;
    dhcp_info_t *info  = hm_get(requests, &in_pkt->xid);

    if (!info) {
        kprint("dhcp - received offer but no discover sent!\n");
        return -ENOENT;
    } else {
        for (size_t i = 0; i < pkt->app.size;) {
            switch (in_pkt->payload[i]) {
                case OPT_SUBNET_MASK:
                    kmemcpy(&info->sb_mask, &in_pkt->payload[i + 2], in_pkt->payload[i + 1]);
                    break;

                case OPT_BROADCAST:
                    kmemcpy(&info->broadcast, &in_pkt->payload[i + 2], in_pkt->payload[i + 1]);
                    break;

                case OPT_ROUTER:
                    kmemcpy(&info->router, &in_pkt->payload[i + 2], in_pkt->payload[i + 1]);
                    break;

                case OPT_DNS:
                    kmemcpy(&info->dns, &in_pkt->payload[i + 2], in_pkt->payload[i + 1]);
                    break;

                case OPT_LEASE_TIME:
                    kmemcpy(&info->lease, &in_pkt->payload[i + 2], in_pkt->payload[i + 1]);
                    break;

                case OPT_END:
                    goto end;

                default:
                    break;
            }

            i += 2 + in_pkt->payload[i + 1];
        }
end:
        kmemcpy(&info->addr, &in_pkt->yiaddr, sizeof(uint32_t));

        /* If the sender of the packet is the same as the server, save the mac address */
        kassert(pkt->net.proto == PROTO_IPV4);
        ipv4_datagram_t *dgram = pkt->net.packet;
        eth_frame_t *eth       = pkt->link;

        if (!kmemcmp(&dgram->src,  &info->router, sizeof(uint32_t)))
             kmemcpy(info->mac.b,   eth->src,     sizeof(info->mac));
    }

    int ret;
    dhcp_pkt_t *out_pkt = __alloc_pkt(9);

    out_pkt->yiaddr = in_pkt->yiaddr;
    out_pkt->siaddr = in_pkt->siaddr;

    /* craft dhcp options */
    out_pkt->payload[0] = OPT_MESSAGE_TYPE;
    out_pkt->payload[1] = 1;
    out_pkt->payload[2] = MSG_REQUEST;

    out_pkt->payload[3] = OPT_REQUESTED_IP_ADDR;
    out_pkt->payload[4] = 4;
    kmemcpy(&out_pkt->payload[5], &out_pkt->yiaddr, sizeof(out_pkt->yiaddr));

    out_pkt->payload[9] = OPT_END;

   ret = udp_send_pkt_old(
        &IPV4_UNSPECIFIED,
        DHCP_CLIENT_PORT,
        &IPV4_BROADCAST,
        DHCP_SERVER_PORT,
        out_pkt,
        sizeof(dhcp_pkt_t) + 10
    );

    kfree(out_pkt);
    return ret;
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
    dhcp_pkt_t     *pkt  = __alloc_pkt(9);
    dhcp_options_t *opts = kzalloc(sizeof(dhcp_options_t));

    if ((errno = hm_insert(requests, &pkt->xid, opts)) < 0) {
        kprint("dhcp - failed to cache request!\n");
        return ret;
    }

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

    ret = udp_send_pkt_old(
        &IPV4_UNSPECIFIED,
        DHCP_CLIENT_PORT,
        &IPV4_BROADCAST,
        DHCP_SERVER_PORT,
        pkt,
        sizeof(dhcp_pkt_t) + 9
    );

    kfree(pkt);
    return ret;
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

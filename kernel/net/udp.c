#include <errno.h>
#include <kernel/kassert.h>
#include <kernel/util.h>
#include <mm/heap.h>
#include <net/dhcp.h>
#include <net/ipv4.h>
#include <net/ipv6.h>
#include <net/udp.h>

#define UDP_HDR_SIZE 8

int udp_handle_pkt(udp_pkt_t *pkt, size_t size)
{
    pkt->src = n2h_16(pkt->src);
    pkt->dst = n2h_16(pkt->dst);

    if (pkt->src == DHCP_SERVER_PORT && pkt->dst == DHCP_CLIENT_PORT) {
        return dhcp_handle_pkt((dhcp_pkt_t *)pkt->payload, pkt->len);
    } else {
        kprint("udp - unhandled packet, src port (%d), dst port (%d)\n",
                pkt->src, pkt->dst);
        return -ENOTSUP;
    }
}

int udp_send_pkt(ip_t *src_addr, int src_port, ip_t *dst_addr, int dst_port, void *payload, size_t size)
{
    kassert(dst_addr && payload && size);

    if (!dst_addr || !payload || !size) {
        kprint("udp - invalid dst addr (0x%x), payload (0x%x), size (%u) or ports (%d, %d)\n",
                dst_addr, payload, size, dst_port, src_port);
        return -EINVAL;
    }

    int ret;
    udp_pkt_t *pkt = kzalloc(sizeof(udp_pkt_t) + size);

    pkt->src      = h2n_16(src_port);
    pkt->dst      = h2n_16(dst_port);
    pkt->len      = h2n_16(size + UDP_HDR_SIZE);
    pkt->checksum = 0;

    kmemcpy(pkt->payload, payload, size);

    if (dst_addr->type == IPV4_ADDR)
        ret = ipv4_send_datagram(src_addr, dst_addr, pkt, sizeof(udp_pkt_t) + size);
    else
        ret = ipv6_send_datagram(src_addr, dst_addr, pkt, sizeof(udp_pkt_t) + size);

    kfree(pkt);
    return 0;
}

#include <errno.h>
#include <net/eth.h>
#include <net/ipv4.h>
#include <net/ipv6.h>
#include <net/util.h>

#define ETH_HDR_SIZE 14

enum {
    ETH_TYPE_IPV4 = 0x0800,
    ETH_TYPE_ARP  = 0x0806,
    ETH_TYPE_IPV6 = 0x86dd,
};

int eth_handle_frame(eth_frame_t *frame, size_t size)
{
    switch (n2h_16(frame->type)) {
        case ETH_TYPE_IPV6:
            return ipv6_handle_datagram((ipv6_datagram_t *)frame->payload, size - ETH_HDR_SIZE);

        default:
            kprint("eth - unsupported frame type: 0x%x\n", n2h_16(frame->type));
            return -ENOTSUP;
    }

    return 0;
}

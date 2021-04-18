#include <net/icmp.h>

enum {
    ICMP_ECHO_REPLY   = 0,
    ICMP_ECHO_REQUEST = 8
};

int icmp_handle_pkt(icmp_pkt_t *pkt, size_t size)
{
    kprint("icmp - handle packet, size %u\n");

    switch (pkt->type) {
        case ICMP_ECHO_REPLY:
            kprint("icmp - got echo reply to request\n");
            break;

        case ICMP_ECHO_REQUEST:
            kprint("icmp - got echo request, reply\n");
            break;

        default:
            kprint("icmp - got unsupported icmp packet, type %u\n", pkt->type);
            break;
    }

    return 0;
}

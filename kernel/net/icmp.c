#include <net/icmp.h>

int icmp_handle_pkt(icmp_pkt *pkt, size_t size)
{
    kprint("icmp - handle packet, size %u\n");

    return 0;
}

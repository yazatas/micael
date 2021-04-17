#include <net/udp.h>

int udp_handle_pkt(udp_pkt *pkt, size_t size)
{
    kprint("udp - handle packet, size %u\n", size);

    return 0;
}

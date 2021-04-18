#include <net/tcp.h>

int tcp_handle_pkt(tcp_pkt_t *pkt, size_t size)
{
    kprint("tcp - handle packet, size %u\n");

    return 0;
}

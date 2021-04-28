#include <errno.h>
#include <kernel/kassert.h>
#include <kernel/kpanic.h>
#include <kernel/util.h>
#include <lib/hashmap.h>
#include <mm/heap.h>
#include <mm/slab.h>
#include <net/arp.h>
#include <net/eth.h>
#include <net/icmp.h>
#include <net/ipv4.h>
#include <net/netdev.h>
#include <net/tcp.h>
#include <net/udp.h>

#define IPV4_FRAG_MASK    0x1fff
#define IPV4_MORE_FRAGS   0x2000
#define IPV4_DONT_FRAG    0x4000
#define IPV4_PAYLOAD_MAX  65515

typedef struct ipv4_dgram_buf {
    size_t fsize;        /* size of a fragment */
    int tsize;           /* total size of all fragments */
    int npkts;           /* total number of fragments */
    int e_idx;           /* index of the last datagram */
    packet_t *pkts[45];  /* fragments (1486 * 45 == 66870) */
} ipv4_fragbuf_t;

static hashmap_t *fragments;
static mm_cache_t *frag_cache;

void ipv4_init(void)
{
    if (!(fragments = hm_alloc_hashmap(8, HM_KEY_TYPE_NUM)))
        kpanic("Failed to allocate hashmap for IPv4 fragment buffers");

    if (!(frag_cache = mmu_cache_create(sizeof(ipv4_fragbuf_t), MM_NO_FLAGS)))
        kpanic("Failed to allocate SLAB cache for IPV4 fragment buffers");
}

static uint16_t __calculate_checksum(uint16_t *header)
{
    uint32_t sum  = 0;
    uint8_t carry = 0;

    for (int i = 0; i < 10; ++i)
        sum += header[i];

    carry = (sum & 0xff0000) >> 16;

    return ~(carry + sum);
}

int ipv4_handle_pkt(packet_t *pkt)
{
    ipv4_datagram_t *dgram = pkt->net.packet;
    ipv4_fragbuf_t *fb     = NULL;

    pkt->transport.packet = dgram->payload;
    pkt->transport.proto  = dgram->proto;
    pkt->transport.size   = pkt->net.size - IPV4_HDR_SIZE;

    dgram->iden  = n2h_16(dgram->iden);
    dgram->f_off = n2h_16(dgram->f_off);
    dgram->src   = n2h_32(dgram->src);
    dgram->dst   = n2h_32(dgram->dst);
    dgram->len   = n2h_16(dgram->len);

    if (dgram->f_off & IPV4_MORE_FRAGS || dgram->f_off & IPV4_FRAG_MASK) {
        if (!(fb = hm_get(fragments, &(uint32_t){ (uint32_t)dgram->iden }))) {
            fb = mmu_cache_alloc_entry(frag_cache, MM_ZERO);
            hm_insert(fragments, &(uint32_t){ (uint32_t)dgram->iden }, fb);

            fb->fsize = dgram->len - IPV4_HDR_SIZE;
            fb->e_idx = -1;
        }
    }

    if (fb) {
        int f_off = ((dgram->f_off & IPV4_FRAG_MASK) * 8);
        int p_len = (dgram->len - IPV4_HDR_SIZE);
        int off   = f_off / p_len;
        int ptr   = 0;

        /* the end fragment may have less data than previous fragments */
        if (!(dgram->f_off & IPV4_MORE_FRAGS) && dgram->f_off & IPV4_FRAG_MASK)
            fb->e_idx = off = f_off / fb->fsize;

        fb->pkts[off]  = pkt;
        fb->tsize     += p_len;
        fb->npkts     += 1;

        /* all fragments have not been received */
        if (fb->npkts - 1 != fb->e_idx || fb->e_idx == -1)
            return 0;

        packet_t *merged      = netdev_alloc_pkt_L4(PROTO_IPV4, fb->tsize);
        ipv4_datagram_t *ipv4 = merged->net.packet;
        uint8_t *payload      = merged->transport.packet;

        /* copy ip header from the last fragment and adjust fields correctly */
        kmemcpy(ipv4, pkt->link->payload, IPV4_HDR_SIZE);
        ipv4->f_off = 0;
        ipv4->len   = fb->tsize;

        for (int i = 0; i < fb->npkts; ++i) {
            ipv4_datagram_t *frag = fb->pkts[i]->net.packet;

            kmemcpy(payload + ptr, frag->payload, frag->len - IPV4_HDR_SIZE);
            ptr += frag->len - IPV4_HDR_SIZE;
            netdev_dealloc_pkt(fb->pkts[i]);
        }

        mmu_cache_free_entry(frag_cache, fb, MM_NO_FLAGS);
        hm_remove(fragments, &(uint32_t){ (uint32_t)ipv4->iden });
        pkt = merged;
    }

    switch (dgram->proto) {
        case PROTO_ICMP:
            return icmp_handle_pkt((icmp_pkt_t *)dgram->payload, n2h_16(dgram->len));

        case PROTO_UDP:
            return udp_handle_pkt(pkt);

        case PROTO_TCP:
            return tcp_handle_pkt((tcp_pkt_t *)dgram->payload, n2h_16(dgram->len));
    }

    /* kprint("ipv4 - unsupported packet type: 0x%x\n", dgram->proto); */
    return -ENOTSUP;
}

int ipv4_send_pkt(packet_t *pkt)
{
    kassert(pkt);

    ipv4_datagram_t *dgram = pkt->net.packet;

    kmemcpy(&dgram->src, pkt->src_addr->ipv4, sizeof(dgram->src));
    kmemcpy(&dgram->dst, pkt->dst_addr->ipv4, sizeof(dgram->dst));

    dgram->version  = 5;
    dgram->ihl      = 4;
    dgram->len      = h2n_16(pkt->net.size);
    dgram->ttl      = 128;
    dgram->proto    = pkt->transport.proto;
    dgram->checksum = __calculate_checksum((uint16_t *)dgram);

    return eth_send_pkt(pkt);
}

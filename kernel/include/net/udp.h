#ifndef __UDP_H__
#define __UDP_H__

#include <fs/fs.h>
#include <kernel/compiler.h>
#include <kernel/common.h>
#include <net/util.h>
#include <net/types.h>

#define SKB_MAX_SIZE 10

typedef struct socket socket_t;

typedef struct udp_skb {
    spinlock_t lock;
    size_t rptr;
    size_t wptr;
    size_t npkts;
    packet_t *packets[SKB_MAX_SIZE];
} udp_skb_t;

/* TODO:  */
int udp_handle_pkt(packet_t *pkt);

int udp_send_pkt(packet_t *pkt);

/* TODO:  */
void udp_init_socket(file_t *fd);

/* Read a datagram from the UDP socket buffer if there are any available
 *
 * If "size" is smaller than the oldest available datagram, the
 * packet is truncated to fit into "buf".
 *
 * Return the number of bytes copied on success
 * Return 0 if the socket buffer is empty
 * Return -EINVAL if one of the parameters is invalid */
int udp_read_skb(socket_t *sock, void *buf, size_t size);

/* Write a packet to the UDP socket buffer
 *
 * If the buffer is full, the oldest packet is
 * overwritten with the new packet
 *
 * Return 0 on success
 * Return -EINVAL if one of the parameters is invalid */
int udp_write_skb(socket_t *sock, packet_t *pkt);

#endif /* __UDP_H__ */

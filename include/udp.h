#ifndef SHADOWBOX_UDP_H
#define SHADOWBOX_UDP_H

#include "types.h"
#include "net.h"

typedef struct udp_socket {
    uint32_t local_ip;
    uint16_t local_port;
    uint32_t remote_ip;
    uint16_t remote_port;
    uint8_t *recv_buffer;
    uint32_t recv_size;
    uint32_t recv_capacity;
    struct udp_socket *next;
} udp_socket_t;

void udp_init(void);
udp_socket_t *udp_socket_create(void);
int udp_socket_bind(udp_socket_t *sock, uint32_t ip, uint16_t port);
int udp_socket_sendto(udp_socket_t *sock, net_device_t *dev, uint32_t dest_ip, uint16_t dest_port, const void *data, uint32_t len);
int udp_socket_recvfrom(udp_socket_t *sock, void *buf, uint32_t len);
void udp_socket_destroy(udp_socket_t *sock);
void udp_handle_packet(net_device_t *dev, uint8_t *packet, uint32_t len, uint32_t src_ip);

#endif

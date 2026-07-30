#ifndef SHADOWBOX_TCP_H
#define SHADOWBOX_TCP_H

#include "types.h"
#include "net.h"

int tcp_init(void);
tcp_socket_t* tcp_socket_create(void);
int tcp_socket_bind(tcp_socket_t *sock, uint32_t ip, uint16_t port);
int tcp_socket_connect(tcp_socket_t *sock, uint32_t ip, uint16_t port);
int tcp_socket_listen(tcp_socket_t *sock, int backlog);
tcp_socket_t* tcp_socket_accept(tcp_socket_t *listen_sock);
int tcp_socket_send(tcp_socket_t *sock, const void *data, uint32_t len);
int tcp_socket_recv(tcp_socket_t *sock, void *buf, uint32_t len);
void tcp_socket_close(tcp_socket_t *sock);
void tcp_handle_packet(net_device_t *dev, uint8_t *packet, uint32_t len);
void tcp_congestion_control(tcp_socket_t *sock);
void tcp_update_rtt(tcp_socket_t *sock, uint32_t rtt_sample);

int tcp_send_syn(tcp_socket_t *sock, net_device_t *dev);
int tcp_send_ack(tcp_socket_t *sock, net_device_t *dev, uint32_t seq, uint32_t ack);
int tcp_send_fin(tcp_socket_t *sock, net_device_t *dev);
int tcp_send_data(tcp_socket_t *sock, net_device_t *dev, const void *data, uint32_t len);

#endif

#include "udp.h"
#include "kernel.h"
#include "malloc.h"
#include "kstring.h"
#include "errno.h"
#include "net.h"

static udp_socket_t *udp_sockets = NULL;
static uint16_t next_udp_port = 40000;

void udp_init(void) {
    printk(KERN_INFO "UDP: Initializing UDP subsystem\n");
    udp_sockets = NULL;
    next_udp_port = 40000;
}

udp_socket_t *udp_socket_create(void) {
    udp_socket_t *sock = kmalloc(sizeof(udp_socket_t));
    if (!sock) return NULL;
    memset(sock, 0, sizeof(udp_socket_t));
    sock->recv_buffer = kmalloc(65536);
    sock->recv_capacity = 65536;
    sock->recv_size = 0;
    sock->next = udp_sockets;
    udp_sockets = sock;
    return sock;
}

int udp_socket_bind(udp_socket_t *sock, uint32_t ip, uint16_t port) {
    if (!sock) return -EINVAL;
    sock->local_ip = ip;
    sock->local_port = port;
    return 0;
}

int udp_socket_sendto(udp_socket_t *sock, net_device_t *dev,
                      uint32_t dest_ip, uint16_t dest_port,
                      const void *data, uint32_t len) {
    if (!sock || !dev) return -EINVAL;

    uint8_t *packet = kmalloc(sizeof(struct eth_header) + sizeof(struct ip_header) +
                               sizeof(struct udp_header) + len);
    if (!packet) return -ENOMEM;

    struct eth_header *eth = (struct eth_header *)packet;
    for (int i = 0; i < 6; i++) {
        eth->dest[i] = 0xFF;
        eth->src[i] = dev->mac[i];
    }
    eth->ethertype = (ETH_IP >> 8) | (ETH_IP << 8);

    struct ip_header *ip = (struct ip_header *)(packet + sizeof(struct eth_header));
    ip->version_ihl = 0x45;
    ip->tos = 0;
    ip->total_len = sizeof(struct ip_header) + sizeof(struct udp_header) + len;
    ip->id = 0;
    ip->flags_frag = 0;
    ip->ttl = 64;
    ip->protocol = IP_UDP;
    ip->checksum = 0;
    ip->src_ip = sock->local_ip ? sock->local_ip : dev->ip;
    ip->dest_ip = dest_ip;
    ip->checksum = net_checksum((uint16_t *)ip, sizeof(struct ip_header));

    struct udp_header *udp = (struct udp_header *)(packet + sizeof(struct eth_header) + sizeof(struct ip_header));
    udp->src_port = sock->local_port ? sock->local_port : next_udp_port++;
    udp->dest_port = dest_port;
    udp->length = sizeof(struct udp_header) + len;
    udp->checksum = 0;

    memcpy((uint8_t *)(udp + 1), data, len);

    if (sock->local_port == 0) sock->local_port = udp->src_port;

    dev->send_packet(dev, packet,
                     sizeof(struct eth_header) + sizeof(struct ip_header) +
                     sizeof(struct udp_header) + len);
    kfree(packet);
    return len;
}

int udp_socket_recvfrom(udp_socket_t *sock, void *buf, uint32_t len) {
    if (!sock) return -EINVAL;
    if (sock->recv_size == 0) return 0;
    uint32_t to_copy = (len < sock->recv_size) ? len : sock->recv_size;
    memcpy(buf, sock->recv_buffer, to_copy);
    sock->recv_size -= to_copy;
    if (sock->recv_size > 0) {
        memmove(sock->recv_buffer, sock->recv_buffer + to_copy, sock->recv_size);
    }
    return to_copy;
}

void udp_socket_destroy(udp_socket_t *sock) {
    if (!sock) return;
    udp_socket_t **prev = &udp_sockets;
    while (*prev && *prev != sock) prev = &(*prev)->next;
    if (*prev == sock) *prev = sock->next;
    if (sock->recv_buffer) kfree(sock->recv_buffer);
    kfree(sock);
}

void udp_handle_packet(net_device_t *dev, uint8_t *packet, uint32_t len, uint32_t src_ip) {
    (void)dev;
    if (len < sizeof(struct udp_header)) return;
    struct udp_header *udp = (struct udp_header *)packet;

    udp_socket_t *sock = udp_sockets;
    while (sock) {
        if (sock->local_port == udp->dest_port &&
            (!sock->remote_port || sock->remote_port == udp->src_port)) {
            uint32_t data_len = udp->length - sizeof(struct udp_header);
            uint8_t *data = packet + sizeof(struct udp_header);
            if (data_len > 0 && sock->recv_size + data_len <= sock->recv_capacity) {
                memcpy(sock->recv_buffer + sock->recv_size, data, data_len);
                sock->recv_size += data_len;
            }
            return;
        }
        sock = sock->next;
    }
}

#include "tcp.h"
#include "kernel.h"
#include "malloc.h"
#include "kstring.h"
#include "errno.h"
#include "net.h"
#include "time.h"

static tcp_socket_t *tcp_sockets = NULL;
static uint16_t next_tcp_port = 49152;
static net_device_t *default_dev = NULL;

int tcp_init(void) {
    printk(KERN_INFO "TCP: Initializing TCP subsystem with packet send/recv\n");
    tcp_sockets = NULL;
    next_tcp_port = 49152;
    return 0;
}

static net_device_t *tcp_get_dev(tcp_socket_t *sock) {
    (void)sock;
    extern net_device_t *net_devices;
    if (default_dev) return default_dev;
    default_dev = net_devices;
    return default_dev;
}

static uint32_t tcp_checksum(uint32_t src_ip, uint32_t dst_ip,
                              uint8_t *tcp_seg, uint32_t tcp_len) {
    uint32_t sum = 0;
    uint16_t *pseudo = (uint16_t *)kmalloc(12 + tcp_len + ((tcp_len & 1) ? 1 : 0));
    if (!pseudo) return 0;

    memset(pseudo, 0, 12 + tcp_len + ((tcp_len & 1) ? 1 : 0));
    pseudo[0] = src_ip >> 16;
    pseudo[1] = src_ip & 0xFFFF;
    pseudo[2] = dst_ip >> 16;
    pseudo[3] = dst_ip & 0xFFFF;
    pseudo[4] = 0;
    pseudo[5] = IP_TCP;
    pseudo[6] = (tcp_len >> 16) & 0xFFFF;
    pseudo[7] = tcp_len & 0xFFFF;

    memcpy(pseudo + 8, tcp_seg, tcp_len);

    uint32_t pseudo_len = 12 + tcp_len + ((tcp_len & 1) ? 1 : 0);
    for (uint32_t i = 0; i < pseudo_len / 2; i++) {
        sum += pseudo[i];
    }

    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    kfree(pseudo);
    return ~sum & 0xFFFF;
}

static int tcp_send_ip_packet(net_device_t *dev, uint32_t dest_ip,
                               uint8_t protocol, uint8_t *data, uint32_t len) {
    uint8_t *packet = kmalloc(sizeof(struct eth_header) + sizeof(struct ip_header) + len);
    if (!packet) return -ENOMEM;

    struct eth_header *eth = (struct eth_header *)packet;
    for (int i = 0; i < 6; i++) {
        eth->dest[i] = 0xFF;
        eth->src[i] = dev->mac[i];
    }
    eth->dest[5] = 0xFF;
    eth->ethertype = (ETH_IP >> 8) | (ETH_IP << 8);

    struct ip_header *ip = (struct ip_header *)(packet + sizeof(struct eth_header));
    ip->version_ihl = 0x45;
    ip->tos = 0;
    ip->total_len = sizeof(struct ip_header) + len;
    ip->id = 0;
    ip->flags_frag = 0;
    ip->ttl = 64;
    ip->protocol = protocol;
    ip->checksum = 0;
    ip->src_ip = dev->ip;
    ip->dest_ip = dest_ip;
    ip->checksum = net_checksum((uint16_t *)ip, sizeof(struct ip_header));

    memcpy(packet + sizeof(struct eth_header) + sizeof(struct ip_header), data, len);
    dev->send_packet(dev, packet, sizeof(struct eth_header) + sizeof(struct ip_header) + len);
    kfree(packet);
    return len;
}

int tcp_send_syn(tcp_socket_t *sock, net_device_t *dev) {
    struct tcp_header tcp;
    memset(&tcp, 0, sizeof(tcp));
    tcp.src_port = sock->local_port;
    tcp.dest_port = sock->remote_port;
    tcp.seq_num = sock->send_seq;
    tcp.ack_num = 0;
    tcp.data_offset = (sizeof(struct tcp_header) / 4) << 4;
    tcp.flags = TCP_SYN;
    tcp.window_size = sock->recv_window;
    tcp.checksum = 0;
    tcp.checksum = tcp_checksum(sock->local_ip, sock->remote_ip,
                                 (uint8_t *)&tcp, sizeof(tcp));
    return tcp_send_ip_packet(dev, sock->remote_ip, IP_TCP, (uint8_t *)&tcp, sizeof(tcp));
}

int tcp_send_ack(tcp_socket_t *sock, net_device_t *dev, uint32_t seq, uint32_t ack) {
    struct tcp_header tcp;
    memset(&tcp, 0, sizeof(tcp));
    tcp.src_port = sock->local_port;
    tcp.dest_port = sock->remote_port;
    tcp.seq_num = seq;
    tcp.ack_num = ack;
    tcp.data_offset = (sizeof(struct tcp_header) / 4) << 4;
    tcp.flags = TCP_ACK;
    tcp.window_size = sock->recv_window;
    tcp.checksum = tcp_checksum(sock->local_ip, sock->remote_ip,
                                 (uint8_t *)&tcp, sizeof(tcp));
    return tcp_send_ip_packet(dev, sock->remote_ip, IP_TCP, (uint8_t *)&tcp, sizeof(tcp));
}

int tcp_send_fin(tcp_socket_t *sock, net_device_t *dev) {
    struct tcp_header tcp;
    memset(&tcp, 0, sizeof(tcp));
    tcp.src_port = sock->local_port;
    tcp.dest_port = sock->remote_port;
    tcp.seq_num = sock->send_seq;
    tcp.ack_num = sock->recv_seq;
    tcp.data_offset = (sizeof(struct tcp_header) / 4) << 4;
    tcp.flags = TCP_FIN | TCP_ACK;
    tcp.window_size = sock->recv_window;
    tcp.checksum = tcp_checksum(sock->local_ip, sock->remote_ip,
                                 (uint8_t *)&tcp, sizeof(tcp));
    return tcp_send_ip_packet(dev, sock->remote_ip, IP_TCP, (uint8_t *)&tcp, sizeof(tcp));
}

int tcp_send_data(tcp_socket_t *sock, net_device_t *dev, const void *data, uint32_t len) {
    uint32_t tcp_len = sizeof(struct tcp_header) + len;
    uint8_t *tcp = kmalloc(tcp_len);
    if (!tcp) return -ENOMEM;

    struct tcp_header *hdr = (struct tcp_header *)tcp;
    memset(hdr, 0, sizeof(struct tcp_header));
    hdr->src_port = sock->local_port;
    hdr->dest_port = sock->remote_port;
    hdr->seq_num = sock->send_seq;
    hdr->ack_num = sock->recv_seq;
    hdr->data_offset = (sizeof(struct tcp_header) / 4) << 4;
    hdr->flags = TCP_PSH | TCP_ACK;
    hdr->window_size = sock->recv_window;

    memcpy(tcp + sizeof(struct tcp_header), data, len);
    hdr->checksum = tcp_checksum(sock->local_ip, sock->remote_ip, tcp, tcp_len);

    int ret = tcp_send_ip_packet(dev, sock->remote_ip, IP_TCP, tcp, tcp_len);
    sock->send_seq += len;
    kfree(tcp);
    return ret;
}

tcp_socket_t* tcp_socket_create(void) {
    tcp_socket_t *sock = kmalloc(sizeof(tcp_socket_t));
    if (!sock) return NULL;
    memset(sock, 0, sizeof(tcp_socket_t));
    sock->send_window = TCP_DEFAULT_WINDOW;
    sock->recv_window = TCP_DEFAULT_WINDOW;
    sock->state = TCP_CLOSED;
    sock->cwnd = TCP_INITIAL_CWND;
    sock->ssthresh = TCP_INITIAL_SSTHRESH;
    sock->rto = TCP_MIN_RTO;
    sock->send_buffer = kmalloc(65536);
    sock->recv_buffer = kmalloc(65536);
    sock->next = tcp_sockets;
    tcp_sockets = sock;
    return sock;
}

int tcp_socket_bind(tcp_socket_t *sock, uint32_t ip, uint16_t port) {
    if (!sock) return -EINVAL;
    sock->local_ip = ip;
    sock->local_port = port;
    return 0;
}

int tcp_socket_connect(tcp_socket_t *sock, uint32_t ip, uint16_t port) {
    if (!sock) return -EINVAL;
    sock->remote_ip = ip;
    sock->remote_port = port;
    if (sock->local_port == 0) sock->local_port = next_tcp_port++;
    sock->send_seq = get_ms_time() * 1000;
    sock->state = TCP_SYN_SENT;
    net_device_t *dev = tcp_get_dev(sock);
    if (dev) tcp_send_syn(sock, dev);
    return 0;
}

int tcp_socket_listen(tcp_socket_t *sock, int backlog) {
    if (!sock) return -EINVAL;
    if (sock->local_port == 0) sock->local_port = next_tcp_port++;
    sock->state = TCP_LISTEN;
    (void)backlog;
    return 0;
}

tcp_socket_t* tcp_socket_accept(tcp_socket_t *listen_sock) {
    if (!listen_sock || listen_sock->state != TCP_LISTEN) return NULL;
    return NULL;
}

int tcp_socket_send(tcp_socket_t *sock, const void *data, uint32_t len) {
    if (!sock || sock->state != TCP_ESTABLISHED) return -EINVAL;
    net_device_t *dev = tcp_get_dev(sock);
    if (!dev) return -ENODEV;
    return tcp_send_data(sock, dev, data, len);
}

int tcp_socket_recv(tcp_socket_t *sock, void *buf, uint32_t len) {
    if (!sock || sock->state != TCP_ESTABLISHED) return -EINVAL;
    if (sock->recv_buffer_size == 0) return 0;
    uint32_t to_copy = (len < sock->recv_buffer_size) ? len : sock->recv_buffer_size;
    memcpy(buf, sock->recv_buffer, to_copy);
    sock->recv_buffer_size -= to_copy;
    if (sock->recv_buffer_size > 0) {
        memmove(sock->recv_buffer, sock->recv_buffer + to_copy, sock->recv_buffer_size);
    }
    return to_copy;
}

void tcp_socket_close(tcp_socket_t *sock) {
    if (!sock) return;
    net_device_t *dev = tcp_get_dev(sock);
    if (sock->state == TCP_ESTABLISHED && dev) {
        tcp_send_fin(sock, dev);
    }
    sock->state = TCP_CLOSED;
    if (sock->send_buffer) kfree(sock->send_buffer);
    if (sock->recv_buffer) kfree(sock->recv_buffer);

    tcp_socket_t **prev = &tcp_sockets;
    while (*prev && *prev != sock) prev = &(*prev)->next;
    if (*prev == sock) *prev = sock->next;
    kfree(sock);
}

void tcp_handle_packet(net_device_t *dev, uint8_t *packet, uint32_t len) {
    if (len < sizeof(struct tcp_header)) return;
    struct tcp_header *tcp = (struct tcp_header *)packet;

    tcp_socket_t *sock = tcp_sockets;
    while (sock) {
        if (sock->local_port == ntohs(tcp->dest_port) &&
            (sock->remote_port == 0 || sock->remote_port == ntohs(tcp->src_port))) {
            break;
        }
        sock = sock->next;
    }
    if (!sock) return;

    uint32_t seq = ntohl(tcp->seq_num);
    uint32_t ack = ntohl(tcp->ack_num);
    uint8_t flags = tcp->flags;

    switch (sock->state) {
    case TCP_SYN_SENT:
        if ((flags & (TCP_SYN | TCP_ACK)) == (TCP_SYN | TCP_ACK)) {
            sock->state = TCP_ESTABLISHED;
            sock->recv_seq = seq + 1;
            sock->send_unack = ack;
            tcp_send_ack(sock, dev, sock->send_seq, sock->recv_seq);
        } else if (flags & TCP_SYN) {
            sock->state = TCP_SYN_RECEIVED;
            sock->recv_seq = seq + 1;
            tcp_send_ack(sock, dev, sock->send_seq, sock->recv_seq);
            tcp_send_syn(sock, dev);
        }
        break;
    case TCP_SYN_RECEIVED:
        if (flags & TCP_ACK) {
            sock->state = TCP_ESTABLISHED;
            sock->send_unack = ack;
        }
        break;
    case TCP_ESTABLISHED:
        if (flags & TCP_ACK) {
            sock->send_unack = ack;
        }
        if (flags & TCP_FIN) {
            sock->state = TCP_CLOSE_WAIT;
            tcp_send_ack(sock, dev, sock->send_seq, seq + 1);
        }
        {
            uint32_t data_len = len - ((tcp->data_offset >> 4) * 4);
            if (data_len > 0) {
                uint8_t *data = packet + (tcp->data_offset >> 4) * 4;
                if (sock->recv_buffer_size + data_len <= 65536) {
                    memcpy(sock->recv_buffer + sock->recv_buffer_size, data, data_len);
                    sock->recv_buffer_size += data_len;
                }
                sock->recv_seq = seq + data_len;
                tcp_send_ack(sock, dev, sock->send_seq, sock->recv_seq);
            }
        }
        break;
    case TCP_FIN_WAIT_1:
    case TCP_FIN_WAIT_2:
        if (flags & TCP_FIN) {
            tcp_send_ack(sock, dev, sock->send_seq, seq + 1);
            sock->state = TCP_TIME_WAIT;
        }
        break;
    default:
        break;
    }
}

void tcp_congestion_control(tcp_socket_t *sock) {
    if (!sock) return;
    if (sock->cwnd < sock->ssthresh) {
        sock->cwnd *= 2;
    } else {
        sock->cwnd += TCP_MSS;
    }
    if (sock->cwnd > 65535) sock->cwnd = 65535;
}

void tcp_update_rtt(tcp_socket_t *sock, uint32_t rtt_sample) {
    if (!sock) return;
    if (sock->rtt == 0) {
        sock->rtt = rtt_sample;
        sock->rtt_var = rtt_sample / 2;
    } else {
        sock->rtt_var = (3 * sock->rtt_var + abs((int)sock->rtt - (int)rtt_sample)) / 4;
        sock->rtt = (7 * sock->rtt + rtt_sample) / 8;
    }
    sock->rto = sock->rtt + 4 * sock->rtt_var;
    if (sock->rto < TCP_MIN_RTO) sock->rto = TCP_MIN_RTO;
    if (sock->rto > TCP_MAX_RTO) sock->rto = TCP_MAX_RTO;
}

#ifndef SHADOWBOX_NET_H
#define SHADOWBOX_NET_H

#include "types.h"

struct udp_socket;
typedef struct udp_socket udp_socket_t;

struct tcp_socket;
typedef struct tcp_socket tcp_socket_t;

struct bt_device { char name[64]; };
typedef struct bt_device bt_device_t;

/*
 * net_device_t - Network device descriptor
 * @name:        Device name
 * @mac:         MAC address
 * @ip:          IP address
 * @netmask:     Network mask
 * @gateway:     Gateway address
 * @send_packet: Packet transmission callback
 * @next:        Next device in list
 */
typedef struct net_device {
    const char *name;
    uint8_t mac[6];
    uint32_t ip;
    uint32_t netmask;
    uint32_t gateway;
    int (*send_packet)(struct net_device *dev, void *data, uint32_t len);
    struct net_device *next;
} net_device_t;

/*
 * eth_header - Ethernet frame header
 */
struct eth_header {
    uint8_t dest[6];
    uint8_t src[6];
    uint16_t ethertype;
} __attribute__((packed));

/*
 * arp_packet - ARP packet structure
 */
struct arp_packet {
    uint16_t hw_type;
    uint16_t proto_type;
    uint8_t hw_addr_len;
    uint8_t proto_addr_len;
    uint16_t opcode;
    uint8_t src_mac[6];
    uint32_t src_ip;
    uint8_t dest_mac[6];
    uint32_t dest_ip;
} __attribute__((packed));

/*
 * ip_header - IPv4 packet header
 */
struct ip_header {
    uint8_t version_ihl;
    uint8_t tos;
    uint16_t total_len;
    uint16_t id;
    uint16_t flags_frag;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dest_ip;
} __attribute__((packed));

/*
 * icmp_header - ICMP packet header
 */
struct icmp_header {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t id;
    uint16_t sequence;
} __attribute__((packed));

/*
 * udp_header - UDP packet header
 */
struct udp_header {
    uint16_t src_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed));

/*
 * tcp_header - TCP packet header
 */
struct tcp_header {
    uint16_t src_port;
    uint16_t dest_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint8_t data_offset;
    uint8_t flags;
    uint16_t window_size;
    uint16_t checksum;
    uint16_t urgent_ptr;
} __attribute__((packed));

#define TCP_FIN  0x01
#define TCP_SYN  0x02
#define TCP_RST  0x04
#define TCP_PSH  0x08
#define TCP_ACK  0x10
#define TCP_URG  0x20
#define TCP_ECE  0x40
#define TCP_CWR  0x80

#define TCP_CLOSED       0
#define TCP_LISTEN       1
#define TCP_SYN_SENT     2
#define TCP_SYN_RECEIVED 3
#define TCP_ESTABLISHED  4
#define TCP_FIN_WAIT_1   5
#define TCP_FIN_WAIT_2   6
#define TCP_CLOSE_WAIT   7
#define TCP_CLOSING      8
#define TCP_LAST_ACK     9
#define TCP_TIME_WAIT    10

/*
 * tcp_socket_t - TCP socket control block
 */
typedef struct tcp_socket {
    uint32_t local_ip;
    uint16_t local_port;
    uint32_t remote_ip;
    uint16_t remote_port;

    uint32_t send_seq;
    uint32_t recv_seq;
    uint32_t send_unack;
    uint32_t send_window;
    uint32_t recv_window;

    int state;
    uint8_t *send_buffer;
    uint8_t *recv_buffer;
    uint32_t send_buffer_size;
    uint32_t recv_buffer_size;

    uint32_t cwnd;
    uint32_t ssthresh;
    uint32_t rtt;
    uint32_t rtt_var;
    uint32_t rto;

    struct tcp_socket *next;
} tcp_socket_t;

#define ETH_ARP 0x0806
#define ETH_IP  0x0800

#define IP_ICMP 1
#define IP_TCP  6
#define IP_UDP  17

#define ARP_REQUEST 1
#define ARP_REPLY   2

#define ICMP_ECHO_REQUEST 8
#define ICMP_ECHO_REPLY   0

static inline uint16_t ntohs(uint16_t x) { return (x >> 8) | (x << 8); }
static inline uint32_t ntohl(uint32_t x) { return ((x >> 24) & 0xFF) | ((x >> 8) & 0xFF00) | ((x << 8) & 0xFF0000) | ((x << 24) & 0xFF000000); }

extern net_device_t *net_devices;

#define TCP_MSS 1460
#define TCP_DEFAULT_WINDOW 8192
#define TCP_INITIAL_CWND 10
#define TCP_INITIAL_SSTHRESH 65535
#define TCP_MIN_RTO 200
#define TCP_MAX_RTO 120000

/*
 * net_init - Initialize network subsystem
 */
void net_init(void);

/*
 * net_register_device - Register a network device
 * @dev: Network device to register
 */
void net_register_device(net_device_t *dev);

/*
 * net_handle_packet - Handle an incoming network packet
 * @dev:    Receiving network device
 * @packet: Packet data
 * @len:    Packet length
 */
void net_handle_packet(net_device_t *dev, uint8_t *packet, uint32_t len);

/*
 * net_checksum - Compute Internet checksum
 * @data: Data buffer
 * @len:  Data length
 * Returns: 16-bit checksum
 */
uint16_t net_checksum(uint16_t *data, uint32_t len);

/*
 * tcp_init - Initialize TCP subsystem
 * Returns: 0 on success, -1 on error
 */
int tcp_init(void);

/*
 * tcp_socket_create - Create a new TCP socket
 * Returns: New TCP socket, or NULL
 */
tcp_socket_t* tcp_socket_create(void);

/*
 * tcp_socket_bind - Bind TCP socket to local address
 * @sock: Socket to bind
 * @ip:   Local IP address
 * @port: Local port
 * Returns: 0 on success, -1 on error
 */
int tcp_socket_bind(tcp_socket_t *sock, uint32_t ip, uint16_t port);

/*
 * tcp_socket_connect - Connect to remote host
 * @sock: Socket to connect
 * @ip:   Remote IP address
 * @port: Remote port
 * Returns: 0 on success, -1 on error
 */
int tcp_socket_connect(tcp_socket_t *sock, uint32_t ip, uint16_t port);

/*
 * tcp_socket_listen - Listen for connections
 * @sock:    Socket to listen on
 * @backlog: Max pending connections
 * Returns: 0 on success, -1 on error
 */
int tcp_socket_listen(tcp_socket_t *sock, int backlog);

/*
 * tcp_socket_accept - Accept a connection
 * @listen_sock: Listening socket
 * Returns: New connected socket, or NULL
 */
tcp_socket_t* tcp_socket_accept(tcp_socket_t *listen_sock);

/*
 * tcp_socket_send - Send data on TCP socket
 * @sock: Socket to send on
 * @data: Data to send
 * @len:  Data length
 * Returns: Bytes sent, or -1 on error
 */
int tcp_socket_send(tcp_socket_t *sock, const void *data, uint32_t len);

/*
 * tcp_socket_recv - Receive data from TCP socket
 * @sock: Socket to receive from
 * @data: Receive buffer
 * @len:  Buffer length
 * Returns: Bytes received, or -1 on error
 */
int tcp_socket_recv(tcp_socket_t *sock, void *data, uint32_t len);

/*
 * tcp_socket_close - Close a TCP socket
 * @sock: Socket to close
 */
void tcp_socket_close(tcp_socket_t *sock);

/*
 * tcp_handle_packet - Process an incoming TCP packet
 * @dev:    Network device
 * @packet: Packet data
 * @len:    Packet length
 */
void tcp_handle_packet(net_device_t *dev, uint8_t *packet, uint32_t len);

/*
 * tcp_congestion_control - Apply congestion control algorithm
 * @sock: Socket to control
 */
void tcp_congestion_control(tcp_socket_t *sock);

/*
 * tcp_update_rtt - Update RTT estimation
 * @sock:      Socket to update
 * @rtt_sample: New RTT sample
 */
void tcp_update_rtt(tcp_socket_t *sock, uint32_t rtt_sample);

void udp_init(void);
int udp_socket_sendto(udp_socket_t *sock, net_device_t *dev,
                      uint32_t dest_ip, uint16_t dest_port,
                      const void *data, uint32_t len);

#endif

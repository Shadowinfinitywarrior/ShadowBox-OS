#include "net.h"
#include "kernel.h"
#include "malloc.h"
#include "kstring.h"
#include "time.h"
#include "errno.h"
#include "tcp.h"
#include "udp.h"

net_device_t *net_devices = NULL;

#define ARP_CACHE_SIZE 32
static struct {
    uint32_t ip;
    uint8_t mac[6];
    uint64_t timestamp;
} arp_cache[ARP_CACHE_SIZE];
static int arp_cache_count = 0;

void net_init(void) {
    printk(KERN_INFO "NET: Initializing TCP/IP Networking Stack...\n");
    net_devices = NULL;
    arp_cache_count = 0;
    for (int i = 0; i < ARP_CACHE_SIZE; i++) arp_cache[i].ip = 0;
    tcp_init();
    udp_init();
}

void net_register_device(net_device_t *dev) {
    if (!dev) return;
    dev->next = net_devices;
    net_devices = dev;
    printk(KERN_INFO "NET: Registered device '%s' (MAC %x:%x:%x:%x:%x:%x)\n",
           dev->name, dev->mac[0], dev->mac[1], dev->mac[2],
           dev->mac[3], dev->mac[4], dev->mac[5]);
}

uint16_t net_checksum(uint16_t *data, uint32_t len) {
    uint32_t sum = 0;
    while (len > 1) { sum += *data++; len -= 2; }
    if (len > 0) sum += *(uint8_t *)data;
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return ~sum;
}

static void arp_handle_packet(net_device_t *dev, uint8_t *packet, uint32_t len) {
    if (len < sizeof(struct arp_packet)) return;
    struct arp_packet *arp = (struct arp_packet *)packet;
    if (arp->hw_type != 1 || arp->proto_type != 0x0800) return;
    if (arp->hw_addr_len != 6 || arp->proto_addr_len != 4) return;

    if (arp->opcode == ARP_REQUEST) {
        if (arp->dest_ip == dev->ip) {
            struct arp_packet reply;
            reply.hw_type = 1;
            reply.proto_type = 0x0800;
            reply.hw_addr_len = 6;
            reply.proto_addr_len = 4;
            reply.opcode = ARP_REPLY;
            for (int i = 0; i < 6; i++) reply.src_mac[i] = dev->mac[i];
            reply.src_ip = dev->ip;
            for (int i = 0; i < 6; i++) reply.dest_mac[i] = arp->src_mac[i];
            reply.dest_ip = arp->src_ip;

            uint8_t reply_packet[512];
            struct eth_header *eth = (struct eth_header *)reply_packet;
            for (int i = 0; i < 6; i++) eth->dest[i] = arp->src_mac[i];
            for (int i = 0; i < 6; i++) eth->src[i] = dev->mac[i];
            eth->ethertype = ETH_ARP;
            uint8_t *arp_data = reply_packet + sizeof(struct eth_header);
            for (uint32_t i = 0; i < sizeof(struct arp_packet); i++)
                arp_data[i] = ((uint8_t *)&reply)[i];
            dev->send_packet(dev, reply_packet,
                            sizeof(struct eth_header) + sizeof(struct arp_packet));
        }
    } else if (arp->opcode == ARP_REPLY) {
        for (int i = 0; i < ARP_CACHE_SIZE; i++) {
            if (arp_cache[i].ip == 0 || arp_cache[i].ip == arp->src_ip) {
                arp_cache[i].ip = arp->src_ip;
                for (int j = 0; j < 6; j++) arp_cache[i].mac[j] = arp->src_mac[j];
                arp_cache[i].timestamp = 0;
                break;
            }
        }
    }
}

static void arp_handle_packet(net_device_t *dev, uint8_t *packet, uint32_t len);
static void icmp_dispatch(net_device_t *dev, uint8_t *packet, uint32_t len, uint32_t src_ip);

static void icmp_handle_packet(net_device_t *dev, uint8_t *packet, uint32_t len, uint32_t src_ip) {
    if (len < sizeof(struct icmp_header)) return;
    struct icmp_header *icmp = (struct icmp_header *)packet;
    if (icmp->type != ICMP_ECHO_REQUEST) return;

    uint32_t data_len = len - sizeof(struct icmp_header);
    uint8_t ip_packet[512];
    struct ip_header *ip = (struct ip_header *)ip_packet;
    ip->version_ihl = 0x45;
    ip->tos = 0;
    ip->total_len = sizeof(struct ip_header) + sizeof(struct icmp_header) + data_len;
    ip->id = 0;
    ip->flags_frag = 0;
    ip->ttl = 64;
    ip->protocol = IP_ICMP;
    ip->checksum = 0;
    ip->src_ip = dev->ip;
    ip->dest_ip = src_ip;
    ip->checksum = net_checksum((uint16_t *)ip, sizeof(struct ip_header));

    uint8_t *icmp_data = ip_packet + sizeof(struct ip_header);
    struct icmp_header reply;
    reply.type = ICMP_ECHO_REPLY;
    reply.code = 0;
    reply.id = icmp->id;
    reply.sequence = icmp->sequence;
    reply.checksum = 0;
    memcpy(icmp_data, &reply, sizeof(struct icmp_header));
    memcpy(icmp_data + sizeof(struct icmp_header), packet + sizeof(struct icmp_header), data_len);
    uint32_t icmp_len = sizeof(struct icmp_header) + data_len;
    uint16_t cksum = net_checksum((uint16_t *)icmp_data, icmp_len);
    icmp_data[2] = (cksum >> 8) & 0xFF;
    icmp_data[3] = cksum & 0xFF;

    uint8_t eth_frame[512];
    struct eth_header *eth = (struct eth_header *)eth_frame;
    uint8_t dest_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].ip == src_ip) {
            for (int j = 0; j < 6; j++) dest_mac[j] = arp_cache[i].mac[j];
            break;
        }
    }
    for (int i = 0; i < 6; i++) eth->dest[i] = dest_mac[i];
    for (int i = 0; i < 6; i++) eth->src[i] = dev->mac[i];
    eth->ethertype = ETH_IP;
    memcpy(eth_frame + sizeof(struct eth_header), ip_packet, ip->total_len);
    dev->send_packet(dev, eth_frame, sizeof(struct eth_header) + ip->total_len);
}

static void ip_handle_packet(net_device_t *dev, uint8_t *packet, uint32_t len) {
    if (len < sizeof(struct ip_header)) return;
    struct ip_header *ip = (struct ip_header *)packet;
    uint16_t checksum = ip->checksum;
    ip->checksum = 0;
    if (net_checksum((uint16_t *)ip, sizeof(struct ip_header)) != checksum) return;
    ip->checksum = checksum;
    if (ip->dest_ip != dev->ip && ip->dest_ip != 0xFFFFFFFF) return;

    uint8_t *payload = packet + (ip->version_ihl & 0x0F) * 4;
    uint32_t payload_len = ip->total_len - (ip->version_ihl & 0x0F) * 4;

    switch (ip->protocol) {
    case IP_ICMP: icmp_dispatch(dev, payload, payload_len, ip->src_ip); break;
    case IP_UDP:  udp_handle_packet(dev, payload, payload_len, ip->src_ip); break;
    case IP_TCP:  tcp_handle_packet(dev, payload, payload_len); break;
    }
}

void net_handle_packet(net_device_t *dev, uint8_t *packet, uint32_t len) {
    if (len < sizeof(struct eth_header)) return;
    struct eth_header *eth = (struct eth_header *)packet;
    uint16_t ethertype = (eth->ethertype >> 8) | (eth->ethertype << 8);
    uint8_t *payload = packet + sizeof(struct eth_header);
    uint32_t payload_len = len - sizeof(struct eth_header);
    switch (ethertype) {
    case ETH_ARP: arp_handle_packet(dev, payload, payload_len); break;
    case ETH_IP:  ip_handle_packet(dev, payload, payload_len); break;
    }
}

/*
 * net_arp_lookup - Look up the MAC for an IP in the ARP cache.
 * Returns 1 and fills mac[] if found.
 */
int net_arp_lookup(uint32_t ip, uint8_t *mac) {
    if (!mac) return 0;
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].ip == ip) {
            for (int j = 0; j < 6; j++) mac[j] = arp_cache[i].mac[j];
            return 1;
        }
    }
    return 0;
}

/* --- ICMP ping support --- */
static uint32_t ping_id_counter;
static int ping_received;

static void icmp_handle_echo_reply(net_device_t *dev, uint8_t *packet, uint32_t len, uint32_t src_ip) {
    (void)dev; (void)len; (void)src_ip;
    if (len < sizeof(struct icmp_header)) return;
    struct icmp_header *icmp = (struct icmp_header *)packet;
    if (ping_received == 0 && icmp->id == ping_id_counter) {
        ping_received = 1;
    }
}

/* ICMP echo request/checksum are handled by the existing path; add reply
 * recognition to the incoming ICMP dispatch. */
static void icmp_dispatch(net_device_t *dev, uint8_t *packet, uint32_t len, uint32_t src_ip) {
    if (len < sizeof(struct icmp_header)) return;
    struct icmp_header *icmp = (struct icmp_header *)packet;
    if (icmp->type == ICMP_ECHO_REPLY) {
        icmp_handle_echo_reply(dev, packet, len, src_ip);
    } else if (icmp->type == ICMP_ECHO_REQUEST) {
        icmp_handle_packet(dev, packet, len, src_ip);
    }
}

/*
 * net_ping - Send an ICMP echo request and wait for a reply.
 * Returns round-trip time in ms, or -1 on timeout.
 */
int net_ping(uint32_t ip, uint32_t timeout_ms) {
    extern net_device_t *net_devices;
    if (!net_devices) return -ENODEV;
    net_device_t *dev = net_devices;

    uint8_t frame[128];
    struct eth_header *eth = (struct eth_header *)frame;
    uint8_t dst[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    if (net_arp_lookup(ip, dst)) {
        /* use resolved MAC */
    }
    for (int i = 0; i < 6; i++) { eth->dest[i] = dst[i]; eth->src[i] = dev->mac[i]; }
    eth->ethertype = (ETH_IP >> 8) | (ETH_IP << 8);

    struct ip_header *ip_hdr = (struct ip_header *)(frame + sizeof(struct eth_header));
    ip_hdr->version_ihl = 0x45;
    ip_hdr->tos = 0;
    ip_hdr->total_len = sizeof(struct ip_header) + sizeof(struct icmp_header);
    ip_hdr->id = 0;
    ip_hdr->flags_frag = 0;
    ip_hdr->ttl = 64;
    ip_hdr->protocol = IP_ICMP;
    ip_hdr->checksum = 0;
    ip_hdr->src_ip = dev->ip;
    ip_hdr->dest_ip = ip;
    ip_hdr->checksum = net_checksum((uint16_t *)ip_hdr, sizeof(struct ip_header));

    struct icmp_header *icmp = (struct icmp_header *)(frame + sizeof(struct eth_header) + sizeof(struct ip_header));
    icmp->type = ICMP_ECHO_REQUEST;
    icmp->code = 0;
    icmp->id = ++ping_id_counter;
    icmp->sequence = 0;
    icmp->checksum = 0;
    uint16_t cksum = net_checksum((uint16_t *)icmp, sizeof(struct icmp_header));
    icmp->checksum = (uint16_t)((cksum >> 8) | (cksum << 8));

    ping_received = 0;
    uint64_t start = get_ms_time();
    dev->send_packet(dev, frame, sizeof(struct eth_header) + sizeof(struct ip_header) + sizeof(struct icmp_header));

    while (get_ms_time() - start < timeout_ms) {
        if (ping_received) {
            return (int)(get_ms_time() - start);
        }
        extern void yield(void);
        yield();
    }
    return -1;
}

/*
 * DNS Client for ShadowBox OS
 *
 * Simple UDP-based A-record resolver with a tiny cache. Sends a standard
 * query to the configured DNS server and waits (with timeout) for the reply.
 */

#include "net.h"
#include "udp.h"
#include "kernel.h"
#include "kstring.h"
#include "malloc.h"
#include "time.h"
#include "errno.h"

#define DNS_PORT 53
#define DNS_TIMEOUT_MS 2500

/* QEMU user-net DNS is 10.0.2.3; fallback 8.8.8.8 */
static uint32_t dns_server = 0x0A000203;

#define DNS_CACHE_ENTRIES 8
static struct {
    char name[64];
    uint32_t ip;
} dns_cache[DNS_CACHE_ENTRIES];
static int dns_cache_count = 0;

struct dns_header {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
} __attribute__((packed));

void dns_set_server(uint32_t ip) {
    dns_server = ip;
}

uint32_t dns_get_server(void) {
    return dns_server;
}

/* Encode a dot-separated name into DNS label format. Returns bytes or -1. */
static int dns_encode_name(uint8_t *out, int maxlen, const char *name) {
    int len = 0;
    const char *label = name;
    while (*label) {
        const char *dot = label;
        int l = 0;
        while (*dot && *dot != '.') { dot++; l++; }
        if (l <= 0 || l > 63) return -1;
        if (len + l + 1 >= maxlen) return -1;
        out[len++] = (uint8_t)l;
        for (int i = 0; i < l; i++) out[len++] = (uint8_t)label[i];
        if (!*dot) break;
        label = dot + 1;
    }
    if (len == 0) return -1;
    out[len++] = 0;
    return len;
}

int dns_resolve(const char *name, uint32_t *ip_out) {
    if (!name || !ip_out) return -EINVAL;

    /* Already an IPv4 literal? */
    {
        int numeric = (*name != 0);
        const char *p = name;
        for (; *p; p++) {
            if (!((*p >= '0' && *p <= '9') || *p == '.')) { numeric = 0; break; }
        }
        if (numeric) {
            uint32_t ip = 0;
            const char *s = name;
            for (int i = 0; i < 4; i++) {
                uint32_t part = 0;
                while (*s && *s != '.') { part = part * 10 + (*s - '0'); s++; }
                ip = (ip << 8) | (part & 0xFF);
                if (*s == '.') s++;
            }
            *ip_out = ip;
            return 0;
        }
    }

    /* Check cache */
    for (int i = 0; i < dns_cache_count; i++) {
        if (strcmp(dns_cache[i].name, name) == 0) {
            *ip_out = dns_cache[i].ip;
            return 0;
        }
    }

    extern net_device_t *net_devices;
    if (!net_devices) return -ENODEV;

    udp_socket_t *sock = udp_socket_create();
    if (!sock) return -ENOMEM;
    udp_socket_bind(sock, 0, 0);

    uint8_t query[512];
    int qlen = 0;
    struct dns_header *hdr = (struct dns_header *)query;
    hdr->id = (uint16_t)(get_ms_time() & 0xFFFF);
    hdr->flags = 0x0100; /* RD */
    hdr->qdcount = 1;
    hdr->ancount = 0;
    hdr->nscount = 0;
    hdr->arcount = 0;
    qlen = sizeof(struct dns_header);
    int nlen = dns_encode_name(query + qlen, (int)sizeof(query) - qlen, name);
    if (nlen < 0) { udp_socket_destroy(sock); return -EINVAL; }
    qlen += nlen;
    query[qlen++] = 0x00; /* QTYPE A */
    query[qlen++] = 0x01;
    query[qlen++] = 0x00; /* QCLASS IN */
    query[qlen++] = 0x01;

    net_device_t *dev = net_devices;
    udp_socket_sendto(sock, dev, dns_server, DNS_PORT, query, qlen);

    uint8_t resp[512];
    uint64_t start = get_ms_time();
    int result = -1;
    while (get_ms_time() - start < DNS_TIMEOUT_MS) {
        int r = udp_socket_recvfrom(sock, resp, sizeof(resp));
        if (r > (int)sizeof(struct dns_header)) {
            struct dns_header *rh = (struct dns_header *)resp;
            uint16_t ancount = (rh->ancount >> 8) | (rh->ancount << 8);
            /* Skip the question section */
            int pos = sizeof(struct dns_header);
            while (pos < r) {
                uint8_t l = resp[pos++];
                if (l == 0) break;
                if (l >= 192) { pos++; break; } /* compression pointer */
                pos += l;
            }
            pos += 4; /* qtype + qclass */
            /* Parse answers */
            for (uint16_t i = 0; i < ancount && pos < r; i++) {
                while (pos < r) {
                    uint8_t l = resp[pos++];
                    if (l == 0) break;
                    if (l >= 192) { pos++; break; }
                    pos += l;
                }
                if (pos + 10 > r) break;
                uint16_t type = ((uint16_t)resp[pos] << 8) | resp[pos + 1];
                uint16_t rdlen = ((uint16_t)resp[pos + 8] << 8) | resp[pos + 9];
                uint8_t *rdata = resp + pos + 10;
                if (type == 1 && rdlen == 4 && rdata + 4 <= resp + r) {
                    uint32_t ip = ((uint32_t)rdata[0] << 24) | ((uint32_t)rdata[1] << 16) |
                                  ((uint32_t)rdata[2] << 8) | (uint32_t)rdata[3];
                    if (dns_cache_count < DNS_CACHE_ENTRIES) {
                        int slot = dns_cache_count++;
                        int nl = 0;
                        while (name[nl] && nl < 63) { dns_cache[slot].name[nl] = name[nl]; nl++; }
                        dns_cache[slot].name[nl] = 0;
                        dns_cache[slot].ip = ip;
                    }
                    *ip_out = ip;
                    result = 0;
                }
                pos += 10 + rdlen;
            }
            break;
        }
        extern void yield(void);
        yield();
    }

    udp_socket_destroy(sock);
    return result;
}
/*
 * NTP Client for ShadowBox OS
 *
 * Sends an NTP v3 request over UDP/123, parses the reply, and stores a
 * clock correction via time_set_adjust() so gettimeofday() returns the
 * authoritative time (plus any configured time zone offset).
 */

#include "net.h"
#include "udp.h"
#include "kernel.h"
#include "kstring.h"
#include "malloc.h"
#include "time.h"
#include "errno.h"

#define NTP_PORT 123
#define NTP_TIMEOUT_MS 3000
#define NTP_TIMESTAMP_DELTA 2208988800ULL /* 1900 -> 1970 (seconds) */

struct ntp_packet {
    uint8_t li_vn_mode;
    uint8_t stratum;
    uint8_t poll;
    uint8_t precision;
    uint32_t root_delay;
    uint32_t root_dispersion;
    uint32_t ref_id;
    uint64_t ref_ts;
    uint64_t orig_ts;
    uint64_t rx_ts;
    uint64_t tx_ts;
} __attribute__((packed));

int ntp_sync(uint32_t server_ip, int64_t *offset_out) {
    extern net_device_t *net_devices;
    if (!net_devices) return -ENODEV;

    udp_socket_t *sock = udp_socket_create();
    if (!sock) return -ENOMEM;
    udp_socket_bind(sock, 0, 0);

    struct ntp_packet pkt;
    memset(&pkt, 0, sizeof(pkt));
    pkt.li_vn_mode = 0x1B; /* LI=0, VN=3, mode=client */

    net_device_t *dev = net_devices;
    udp_socket_sendto(sock, dev, server_ip, NTP_PORT, &pkt, sizeof(pkt));

    uint8_t resp[512];
    uint64_t start = get_ms_time();
    int result = -1;
    while (get_ms_time() - start < NTP_TIMEOUT_MS) {
        int r = udp_socket_recvfrom(sock, resp, sizeof(resp));
        if (r >= (int)sizeof(struct ntp_packet)) {
            struct ntp_packet *reply = (struct ntp_packet *)resp;
            uint64_t server_sec = (reply->tx_ts >> 32) - NTP_TIMESTAMP_DELTA;
            if (server_sec > NTP_TIMESTAMP_DELTA) { /* sanity: must be > 1970 */
                int64_t apparent = (int64_t)rtc_unix_time_now() + time_get_adjust();
                int64_t offset = (int64_t)server_sec - apparent;
                time_set_adjust(offset);
                if (offset_out) *offset_out = offset;
                result = 0;
            }
            break;
        }
        extern void yield(void);
        yield();
    }

    udp_socket_destroy(sock);
    return result;
}
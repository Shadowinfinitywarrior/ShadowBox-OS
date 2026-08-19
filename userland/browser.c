// Privacy Browser for ShadowBox OS
// A privacy-focused text-based web browser (like lynx/links)
// 
// Privacy Features:
// - No cookies stored by default
// - Do Not Track header sent with every request
// - Private browsing mode (no history, no cache)
// - Ad/tracker blocking via URL filtering
// - HTTPS warning (no TLS support yet)
// - Minimal UI with address bar and content view
//
// Build: $(CC) $(USER_CFLAGS) -no-pie -nostdlib userland/browser.c -o browser.elf

#include "sys.h"
#include "fcntl.h"
#include <stdarg.h>

// ─── Constants ──────────────────────────────────────────────────────────
#define MAX_URL_LEN       2048
#define MAX_RESPONSE_SIZE 65536
#define MAX_HISTORY       100
#define SCREEN_W          80
#define SCREEN_H          25

// Privacy settings
static int g_private_mode = 1;       // Private browsing by default
static int g_block_trackers = 1;     // Block known trackers
static int g_do_not_track = 1;       // Send DNT header
static int g_no_cookies = 1;         // Don't accept cookies
static int g_no_history = 1;         // Don't save history

// Color codes for ANSI terminal
#define ANSI_RESET    "\033[0m"
#define ANSI_BOLD     "\033[1m"
#define ANSI_DIM      "\033[2m"
#define ANSI_RED      "\033[31m"
#define ANSI_GREEN    "\033[32m"
#define ANSI_YELLOW   "\033[33m"
#define ANSI_BLUE     "\033[34m"
#define ANSI_MAGENTA  "\033[35m"
#define ANSI_CYAN     "\033[36m"
#define ANSI_WHITE    "\033[37m"
#define ANSI_BG_BLACK "\033[40m"
#define ANSI_BG_BLUE  "\033[44m"
#define ANSI_CLEAR    "\033[2J\033[H"
#define ANSI_HOME     "\033[H"

// ─── Simple string helpers ──────────────────────────────────────────────
static inline void strcpy(char *dst, const char *src) {
    while ((*dst++ = *src++));
}

static inline void strcat(char *dst, const char *src) {
    while (*dst) dst++; while ((*dst++ = *src++));
}

static inline int strcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return *(const unsigned char*)a - *(const unsigned char*)b;
}

static int sprintf(char *buf, const char *fmt, ...) {
    int pos = 0;
    const char *p = fmt;
    va_list ap;
    va_start(ap, fmt);
    while (*p) {
        if (*p == '%') {
            p++;
            int width = 0;
            if (*p == '*') {
                width = va_arg(ap, int);
                p++;
            }
            if (*p == 's') {
                const char *s = va_arg(ap, const char*);
                int slen = 0; while (s[slen]) slen++;
                if (width > slen) {
                    for (int i = 0; i < width - slen; i++) buf[pos++] = ' ';
                }
                for (int i = 0; i < slen; i++) buf[pos++] = s[i];
                p++;
            } else {
                buf[pos++] = *p++;
            }
        } else {
            buf[pos++] = *p++;
        }
    }
    va_end(ap);
    buf[pos] = '\0';
    return pos;
}

static inline void *memset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

static inline int strncmp(const char *a, const char *b, size_t n) {
    while (n-- && *a && *a == *b) { a++; b++; }
    return n == (size_t)-1 ? 0 : *(const unsigned char*)a - *(const unsigned char*)b;
}

static inline char* strstr(const char *haystack, const char *needle) {
    if (!*needle) return (char*)haystack;
    for (; *haystack; haystack++) {
        const char *h = haystack, *n = needle;
        while (*h && *n && *h == *n) { h++; n++; }
        if (!*n) return (char*)haystack;
    }
    return NULL;
}

static void uint_to_str(uint64_t val, char *buf) {
    int idx = 0;
    if (val == 0) { buf[idx++] = '0'; }
    else {
        char tmp[32]; int t = 0;
        while (val > 0) { tmp[t++] = '0' + (val % 10); val /= 10; }
        while (t--) buf[idx++] = tmp[t];
    }
    buf[idx] = '\0';
}

// Case-insensitive string search
static int strcasestr(const char *haystack, const char *needle) {
    if (!*needle) return 1;
    for (; *haystack; haystack++) {
        const char *h = haystack, *n = needle;
        while (*h && *n) {
            char ch = (*h >= 'A' && *h <= 'Z') ? *h + 32 : *h;
            char cn = (*n >= 'A' && *n <= 'Z') ? *n + 32 : *n;
            if (ch != cn) break;
            h++; n++;
        }
        if (!*n) return 1;
    }
    return 0;
}

static int atoi(const char *s) {
    int val = 0;
    while (*s >= '0' && *s <= '9') { val = val * 10 + (*s - '0'); s++; }
    return val;
}

// ─── Network helpers ────────────────────────────────────────────────────
#define AF_INET  1
#define SOCK_STREAM 1
#define SB_SOCKET_CREATE  41
#define SB_SOCKET_CONNECT 42
#define SB_SOCKET_SENDTO  44
#define SB_SOCKET_RECVFROM 45
#define SB_SOCKET_CLOSE   46

static inline int socket_create(int domain, int type, int protocol) {
    return (int)syscall3(SB_SOCKET_CREATE, domain, type, protocol);
}

static inline int socket_connect(int fd, const void *addr, size_t addrlen) {
    return (int)syscall3(SB_SOCKET_CONNECT, fd, (uint64_t)addr, addrlen);
}

static inline int socket_send(int fd, const void *buf, size_t len, int flags) {
    return (int)syscall3(SB_SOCKET_SENDTO, fd, (uint64_t)buf, len);
}

static inline int socket_recv(int fd, void *buf, size_t len, int flags) {
    return (int)syscall3(SB_SOCKET_RECVFROM, fd, (uint64_t)buf, len);
}

static inline int socket_close(int fd) {
    return (int)syscall1(SB_SOCKET_CLOSE, fd);
}

static uint16_t htons(uint16_t x) { return (x >> 8) | (x << 8); }
static uint32_t htonl(uint32_t x) {
    return ((x >> 24) & 0xFF) | ((x >> 8) & 0xFF00) |
           ((x << 8) & 0xFF0000) | ((x << 24) & 0xFF000000);
}

static uint32_t parse_ip(const char *s) {
    uint32_t ip = 0;
    for (int i = 0; i < 4; i++) {
        uint32_t part = 0;
        while (*s && *s != '.') { part = part * 10 + (*s - '0'); s++; }
        ip = (ip << 8) | (part & 0xFF);
        if (*s == '.') s++;
    }
    return ip;
}

// Known tracker domains (simplified list for privacy protection)
static const char *g_tracker_domains[] = {
    "google-analytics.com",
    "googletagmanager.com",
    "facebook.net",
    "connect.facebook.net",
    "doubleclick.net",
    "ad.doubleclick.net",
    "adservice.google.com",
    "googlesyndication.com",
    "googleadservices.com",
    "ads.youtube.com",
    "pixel.quantserve.com",
    "scorecardresearch.com",
    "quantserve.com",
    "taboola.com",
    "outbrain.com",
    "criteo.com",
    "adnxs.com",
    "rubiconproject.com",
    "openx.net",
    "pubmatic.com",
    "adform.net",
    "moatads.com",
    "advertising.com",
    "yieldmo.com",
    "indexexchange.com",
    NULL
};

static int is_tracker_url(const char *url) {
    if (!g_block_trackers) return 0;
    for (int i = 0; g_tracker_domains[i]; i++) {
        if (strcasestr(url, g_tracker_domains[i])) return 1;
    }
    return 0;
}

// ─── HTTP Request ───────────────────────────────────────────────────────
typedef struct {
    char url[MAX_URL_LEN];
    char host[256];
    char path[1024];
    uint16_t port;
    uint32_t ip;
    int use_ssl;
    int dns_failed;
} http_request_t;

static int parse_url(const char *url, http_request_t *req) {
    strcpy(req->url, url);
    
    const char *p = url;
    if (strncmp(p, "https://", 8) == 0) {
        req->use_ssl = 1;
        p += 8;
    } else if (strncmp(p, "http://", 7) == 0) {
        req->use_ssl = 0;
        p += 7;
    } else {
        req->use_ssl = 0;
    }
    
    const char *path_start = strstr(p, "/");
    if (path_start) {
        size_t host_len = path_start - p;
        if (host_len >= sizeof(req->host)) host_len = sizeof(req->host) - 1;
        for (size_t i = 0; i < host_len; i++) req->host[i] = p[i];
        req->host[host_len] = '\0';
        strcpy(req->path, path_start);
    } else {
        strcpy(req->host, p);
        strcpy(req->path, "/");
    }
    
    char *colon = strstr(req->host, ":");
    if (colon) {
        *colon = '\0';
        req->port = htons((uint16_t)atoi(colon + 1));
    } else {
        req->port = htons(req->use_ssl ? 443 : 80);
    }
    
    req->ip = htonl(parse_ip(req->host));

    /* If the host is not a dotted-quad IP literal, resolve it via DNS */
    int is_ip = 1;
    {
        const char *q = req->host;
        int dots = 0;
        while (*q) {
            if (*q == '.') dots++;
            else if (*q < '0' || *q > '9') { is_ip = 0; break; }
            q++;
        }
        if (dots != 3) is_ip = 0;
    }
    if (!is_ip) {
        uint32_t ip = 0;
        if (sb_resolve(req->host, &ip) != 0 || ip == 0) {
            req->dns_failed = 1;
            return -1;  /* resolution failed */
        }
        req->ip = ip;
    }
    
    if (is_tracker_url(url)) {
        return -1;  // Blocked
    }
    
    return 0;
}

static int http_get(http_request_t *req, char *response, size_t max_len) {
    if (req->use_ssl) {
        const char *msg = "HTTPS not supported. This browser only supports HTTP for now.\n";
        strcpy(response, msg);
        return strlen(msg);
    }
    
    int fd = socket_create(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    
    struct sockaddr_in {
        uint16_t sin_family;
        uint16_t sin_port;
        uint32_t sin_addr;
    } addr;
    addr.sin_family = AF_INET;
    addr.sin_port = req->port;
    addr.sin_addr = req->ip;
    
    if (socket_connect(fd, &addr, sizeof(addr)) < 0) {
        socket_close(fd);
        return -1;
    }
    
    // Build request with privacy headers
    char request[2048];
    int len = 0;
    
    // GET line
    const char *get = "GET ";
    while (*get) request[len++] = *get++;
    const char *p = req->path;
    while (*p) request[len++] = *p++;
    const char *http = " HTTP/1.1\r\n";
    while (*http) request[len++] = *http++;
    
    // Host header
    const char *host_hdr = "Host: ";
    while (*host_hdr) request[len++] = *host_hdr++;
    p = req->host;
    while (*p) request[len++] = *p++;
    request[len++] = '\r'; request[len++] = '\n';
    
    // User-Agent
    const char *ua = "User-Agent: ShadowBox PrivacyBrowser/1.0\r\n";
    while (*ua) request[len++] = *ua++;
    
    // Privacy headers
    if (g_do_not_track) {
        const char *dnt = "DNT: 1\r\n";
        while (*dnt) request[len++] = *dnt++;
    }
    
    if (g_no_cookies) {
        const char *cookie = "Cookie: \r\n";
        while (*cookie) request[len++] = *cookie++;
    }
    
    // Accept
    const char *accept = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
    while (*accept) request[len++] = *accept++;
    
    // Connection close
    const char *conn = "Connection: close\r\n\r\n";
    while (*conn) request[len++] = *conn++;
    
    // Send request
    if (socket_send(fd, request, len, 0) != len) {
        socket_close(fd);
        return -1;
    }
    
    // Receive response
    size_t total = 0;
    char buf[4096];
    while (total < max_len - 1) {
        int r = socket_recv(fd, buf, sizeof(buf), 0);
        if (r <= 0) break;
        size_t to_copy = (total + r < max_len - 1) ? r : max_len - 1 - total;
        for (size_t i = 0; i < to_copy; i++) response[total++] = buf[i];
        if (r < (int)sizeof(buf)) break;
    }
    response[total] = '\0';
    
    socket_close(fd);
    return total;
}

// ─── Simple HTML Renderer ───────────────────────────────────────────────
static void render_html(const char *html, char *output, size_t max_len) {
    size_t out_len = 0;
    int in_tag = 0;
    int in_script = 0;
    int in_style = 0;
    int in_body = 0;
    int in_head = 0;
    int in_title = 0;
    char title[256] = {0};
    int title_len = 0;
    
    for (size_t i = 0; html[i] && out_len < max_len - 1; i++) {
        if (!in_tag) {
            if (html[i] == '<') {
                if (strncmp(&html[i], "<script", 7) == 0 || strncmp(&html[i], "<SCRIPT", 7) == 0) {
                    in_script = 1;
                } else if (strncmp(&html[i], "<style", 6) == 0 || strncmp(&html[i], "<STYLE", 6) == 0) {
                    in_style = 1;
                } else if (strncmp(&html[i], "<body", 5) == 0 || strncmp(&html[i], "<BODY", 5) == 0) {
                    in_body = 1;
                } else if (strncmp(&html[i], "<head", 5) == 0 || strncmp(&html[i], "<HEAD", 5) == 0) {
                    in_head = 1;
                } else if (strncmp(&html[i], "<title", 6) == 0 || strncmp(&html[i], "<TITLE", 6) == 0) {
                    in_title = 1;
                } else if (strncmp(&html[i], "</script", 8) == 0 || strncmp(&html[i], "</SCRIPT", 8) == 0) {
                    in_script = 0;
                } else if (strncmp(&html[i], "</style", 7) == 0 || strncmp(&html[i], "</STYLE", 7) == 0) {
                    in_style = 0;
                } else if (strncmp(&html[i], "</body", 6) == 0 || strncmp(&html[i], "</BODY", 6) == 0) {
                    in_body = 0;
                } else if (strncmp(&html[i], "</head", 6) == 0 || strncmp(&html[i], "</HEAD", 6) == 0) {
                    in_head = 0;
                } else if (strncmp(&html[i], "</title", 7) == 0 || strncmp(&html[i], "</TITLE", 7) == 0) {
                    in_title = 0;
                }
                in_tag = 1;
            } else if (!in_script && !in_style && in_body) {
                // Decode common HTML entities
                if (html[i] == '&') {
                    if (strncmp(&html[i], "<", 4) == 0) { output[out_len++] = '<'; i += 3; }
                    else if (strncmp(&html[i], ">", 4) == 0) { output[out_len++] = '>'; i += 3; }
                    else if (strncmp(&html[i], "&", 5) == 0) { output[out_len++] = '&'; i += 4; }
                    else if (strncmp(&html[i], "&nbsp;", 6) == 0) { output[out_len++] = ' '; i += 5; }
                    else if (strncmp(&html[i], "\"", 6) == 0) { output[out_len++] = '"'; i += 5; }
                    else { output[out_len++] = html[i]; }
                } else if (in_title && title_len < 255) {
                    title[title_len++] = html[i];
                } else {
                    output[out_len++] = html[i];
                }
            }
        } else {
            if (html[i] == '>') in_tag = 0;
        }
    }
    output[out_len] = '\0';
    title[title_len] = '\0';
    
    // Build final output with privacy header
    char header[1024];
    int hlen = 0;
    
    // Top border
    const char *top = ANSI_CYAN "╔══════════════════════════════════════════════════════════════════════════╗\n" ANSI_RESET;
    while (*top) header[hlen++] = *top++;
    
    // Title row
    if (title_len > 0) {
        hlen += sprintf(&header[hlen], ANSI_CYAN "║ " ANSI_BOLD "Title: " ANSI_RESET "%s" ANSI_CYAN "%*s║\n" ANSI_RESET, 
                       title, (int)(78 - strlen(title) - 7), "");
    }
    
    // Privacy status row
    hlen += sprintf(&header[hlen], ANSI_CYAN "║ " ANSI_DIM "Privacy: " ANSI_RESET);
    if (g_private_mode) hlen += sprintf(&header[hlen], ANSI_GREEN "● Private" ANSI_RESET "  ");
    else hlen += sprintf(&header[hlen], ANSI_RED "○ Public" ANSI_RESET "  ");
    if (g_block_trackers) hlen += sprintf(&header[hlen], ANSI_GREEN "● TrackerBlock" ANSI_RESET " ");
    else hlen += sprintf(&header[hlen], ANSI_RED "○ NoBlock" ANSI_RESET " ");
    if (g_do_not_track) hlen += sprintf(&header[hlen], ANSI_GREEN "● DNT" ANSI_RESET " ");
    else hlen += sprintf(&header[hlen], ANSI_RED "○ NoDNT" ANSI_RESET " ");
    if (g_no_cookies) hlen += sprintf(&header[hlen], ANSI_GREEN "● NoCookies" ANSI_RESET);
    else hlen += sprintf(&header[hlen], ANSI_RED "○ Cookies" ANSI_RESET);
    hlen += sprintf(&header[hlen], ANSI_CYAN "%*s║\n" ANSI_RESET, 0, "");
    
    // Bottom border
    const char *bottom = ANSI_CYAN "╚══════════════════════════════════════════════════════════════════════════╝\n\n" ANSI_RESET;
    while (*bottom) header[hlen++] = *bottom++;
    header[hlen] = '\0';
    
    // Prepend header to output
    size_t header_len = strlen(header);
    if (out_len + header_len < max_len - 1) {
        for (size_t i = out_len; i < max_len - 1; i++) output[i + header_len] = output[i];
        for (size_t i = 0; i < header_len; i++) output[i] = header[i];
    }
}

// Read a line from input
static int readline(char *buf, int max) {
    int i = 0;
    while (i < max - 1) {
        char c;
        if (sb_pull(0, &c, 1) <= 0) break;
        if (c == '\n' || c == '\r') {
            sb_push(1, "\n", 1);
            break;
        }
        if (c == '\b' || c == 127) {
            if (i > 0) {
                sb_push(1, "\b \b", 3);
                i--;
            }
        } else if (c >= 32 && c <= 126) {
            sb_push(1, &c, 1);
            buf[i++] = c;
        }
    }
    buf[i] = '\0';
    return i;
}

// Print without buffer (direct to stdout)
static void print(const char *s) {
    sb_push(1, s, strlen(s));
}

// ─── Browser State ──────────────────────────────────────────────────────
typedef struct {
    char current_url[MAX_URL_LEN];
    char history[MAX_HISTORY][MAX_URL_LEN];
    int history_pos;
    int history_count;
    char rendered_content[MAX_RESPONSE_SIZE];
    int scroll_offset;
} browser_t;

static browser_t g_browser = {0};

// ─── Navigation ─────────────────────────────────────────────────────────

static void browser_navigate(const char *url) {
    print(ANSI_CLEAR);
    print(ANSI_YELLOW "Loading: " ANSI_RESET);
    print(url);
    print("\n\n");
    print(ANSI_DIM "Connecting..." ANSI_RESET "\n");
    
    http_request_t req = {0};
    if (parse_url(url, &req) < 0) {
        print(ANSI_CLEAR);
        if (req.dns_failed) {
            const char *msg =
                ANSI_RED "\n  DNS RESOLUTION FAILED\n" ANSI_RESET
                "\n  Could not resolve the host:\n    " ANSI_YELLOW;
            print(msg);
            print(req.host);
            print(ANSI_RESET "\n\n  Check that networking is up (tray popup) and try again.\n");
            print("\n  Press any key to continue...\n");
            while (1) {
                char c;
                if (sb_pull(0, &c, 1) > 0) break;
            }
            return;
        }
        const char *msg = 
            ANSI_RED "\n  ██████████████████████████████████████████████████████████████████\n"
            "  █  ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░  █\n"
            "  █  ░  TRACKER BLOCKED                                       ░  █\n"
            "  █  ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░  █\n"
            "  █                                                             █\n"
            "  █  This URL was blocked because it matches a known           █\n"
            "  █  tracking/advertising domain.                              █\n"
            "  █                                                             █\n"
            "  █  Privacy Browser protects you from:                        █\n"
            "  █    • Google Analytics & Tag Manager                        █\n"
            "  █    • Facebook tracking pixels                              █\n"
            "  █    • DoubleClick & AdSense                                 █\n"
            "  █    • Quantcast, Taboola, Outbrain                          █\n"
            "  █    • And 25+ other tracker networks                        █\n"
            "  █                                                             █\n"
            "  █  Press 's' to open Settings and disable 'Block Trackers'  █\n"
            "  █  if you need to visit this site anyway.                    █\n"
            "  █                                                             █\n"
            "  █  Press any key to continue...                               █\n"
            "  █  ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░  █\n"
            "  ██████████████████████████████████████████████████████████████████\n" ANSI_RESET;
        print(msg);
        char any; sb_pull(0, &any, 1);
        return;
    }
    
    char response[MAX_RESPONSE_SIZE];
    int len = http_get(&req, response, sizeof(response));
    
    if (len < 0) {
        print(ANSI_CLEAR);
        const char *msg = 
            ANSI_RED "\n  ╔══════════════════════════════════════════════════════════════╗\n"
            "  ║                   CONNECTION FAILED                          ║\n"
            "  ╠══════════════════════════════════════════════════════════════╣\n"
            "  ║  Could not connect to the server.                           ║\n"
            "  ║                                                              ║\n"
            "  ║  Possible reasons:                                           ║\n"
            "  ║    • Server is unreachable                                   ║\n"
            "  ║    • Network not configured                                  ║\n"
            "  ║    • DNS resolution failed                                   ║\n"
            "  ║    • Connection timeout                                      ║\n"
            "  ║                                                              ║\n"
            "  ║  Try:                                                        ║\n"
            "  ║    • Check network connection                                ║\n"
            "  ║    • Verify the URL is correct                               ║\n"
            "  ║    • Try http:// instead of https://                         ║\n"
            "  ║                                                              ║\n"
            "  ║  Press any key to continue...                                ║\n"
            "  ╚══════════════════════════════════════════════════════════════╝\n" ANSI_RESET;
        print(msg);
        char any; sb_pull(0, &any, 1);
        return;
    }
    
    // Check for HTTPS redirect
    if (strstr(response, "301") || strstr(response, "302")) {
        if (strstr(response, "Location: https://")) {
            print(ANSI_CLEAR);
            const char *msg = 
                ANSI_YELLOW "\n  ╔══════════════════════════════════════════════════════════════╗\n"
                "  ║                    HTTPS REQUIRED                            ║\n"
                "  ╠══════════════════════════════════════════════════════════════╣\n"
                "  ║  This site requires HTTPS which is not yet supported.       ║\n"
                "  ║                                                              ║\n"
                "  ║  ShadowBox OS is working on TLS/SSL support.                ║\n"
                "  ║  For now, only HTTP sites can be loaded.                    ║\n"
                "  ║                                                              ║\n"
                "  ║  You can try:                                                ║\n"
                "  ║    • Finding an HTTP version of the site                    ║\n"
                "  ║    • Using a text-based browser like lynx                   ║\n"
                "  ║    • Waiting for HTTPS support in a future update           ║\n"
                "  ║                                                              ║\n"
                "  ║  Press any key to continue...                                ║\n"
                "  ╚══════════════════════════════════════════════════════════════╝\n" ANSI_RESET;
            print(msg);
            char any; sb_pull(0, &any, 1);
            return;
        }
    }
    
    // Render HTML
    render_html(response, g_browser.rendered_content, sizeof(g_browser.rendered_content));
    g_browser.scroll_offset = 0;
    
    // Add to history if not in private mode
    if (!g_no_history && g_browser.history_count < MAX_HISTORY) {
        strcpy(g_browser.history[g_browser.history_count], url);
        g_browser.history_pos = g_browser.history_count;
        g_browser.history_count++;
    }
}

// ─── UI Rendering ──────────────────────────────────────────────────────

static void draw_browser() {
    print(ANSI_CLEAR);
    
    // Header bar
    print(ANSI_BG_BLUE ANSI_WHITE ANSI_BOLD);
    print(" ShadowBox Privacy Browser ");
    print(ANSI_RESET " ");
    
    // URL bar
    print(ANSI_CYAN "URL: " ANSI_RESET);
    print(g_browser.current_url);
    print("\n");
    
    // Separator
    print(ANSI_DIM "────────────────────────────────────────────────────────────────────────" ANSI_RESET "\n");
    
    // Content with scrolling
    // Find start of visible content
    char *content = g_browser.rendered_content;
    for (int i = 0; i < g_browser.scroll_offset && *content; i++) {
        content = strstr(content, "\n");
        if (content) content++;
        else break;
    }
    
    // Print content (up to screen height - 6 lines for header/status)
    int lines_printed = 0;
    int max_lines = SCREEN_H - 6;
    while (*content && lines_printed < max_lines) {
        char *eol = strstr(content, "\n");
        if (!eol) eol = content + strlen(content);
        int line_len = eol - content;
        
        // Print line (truncate if too long)
        if (line_len > SCREEN_W) line_len = SCREEN_W;
        sb_push(1, content, line_len);
        print("\n");
        
        lines_printed++;
        if (*eol) content = eol + 1;
        else break;
    }
    
    // Status bar
    print(ANSI_DIM "────────────────────────────────────────────────────────────────────────" ANSI_RESET "\n");
    print(ANSI_BG_BLACK ANSI_WHITE);
    print(" [G]o  [B]ack  [F]orward  [R]efresh  [H]ome  [S]ettings  [Q]uit  [↑/↓]Scroll ");
    print(ANSI_RESET "\n");
}

// ─── Settings Menu ──────────────────────────────────────────────────────

static void show_settings() {
    while (1) {
        print(ANSI_CLEAR);
        print(ANSI_BOLD ANSI_CYAN "╔══════════════════════════════════════════════════════════════════╗\n");
        print("║                    PRIVACY SETTINGS                              ║\n");
        print("╚══════════════════════════════════════════════════════════════════╝\n" ANSI_RESET "\n");
        
        print("  Current settings:\n\n");
        
        print("  [1] Private Browsing Mode    : ");
        if (g_private_mode) print(ANSI_GREEN "ON (no history, no cookies)" ANSI_RESET "\n");
        else print(ANSI_RED "OFF" ANSI_RESET "\n");
        
        print("  [2] Block Trackers           : ");
        if (g_block_trackers) print(ANSI_GREEN "ON (blocks 25+ tracker domains)" ANSI_RESET "\n");
        else print(ANSI_RED "OFF" ANSI_RESET "\n");
        
        print("  [3] Do Not Track Header      : ");
        if (g_do_not_track) print(ANSI_GREEN "ON (sent with every request)" ANSI_RESET "\n");
        else print(ANSI_RED "OFF" ANSI_RESET "\n");
        
        print("  [4] No Cookies               : ");
        if (g_no_cookies) print(ANSI_GREEN "ON (cookies disabled)" ANSI_RESET "\n");
        else print(ANSI_RED "OFF" ANSI_RESET "\n");
        
        print("  [5] No History               : ");
        if (g_no_history) print(ANSI_GREEN "ON (no history saved)" ANSI_RESET "\n");
        else print(ANSI_RED "OFF" ANSI_RESET "\n");
        
        print("\n");
        print(ANSI_DIM "  Press 1-5 to toggle, [Q] to return to browser\n" ANSI_RESET);
        
        char c; sb_pull(0, &c, 1);
        switch (c) {
            case '1': g_private_mode = !g_private_mode; 
                      if (g_private_mode) { g_no_history = 1; g_no_cookies = 1; } break;
            case '2': g_block_trackers = !g_block_trackers; break;
            case '3': g_do_not_track = !g_do_not_track; break;
            case '4': g_no_cookies = !g_no_cookies; break;
            case '5': g_no_history = !g_no_history; break;
            case 'q': case 'Q': return;
        }
    }
}

// ─── Help Screen ────────────────────────────────────────────────────────

static void show_help() {
    print(ANSI_CLEAR);
    print(ANSI_BOLD ANSI_CYAN "╔══════════════════════════════════════════════════════════════════╗\n");
    print("║                        HELP                                      ║\n");
    print("╚══════════════════════════════════════════════════════════════════╝\n" ANSI_RESET "\n");
    
    print(ANSI_BOLD "Navigation:\n" ANSI_RESET);
    print("  G          - Go to URL (enter URL in address bar)\n");
    print("  B          - Back in history\n");
    print("  F          - Forward in history\n");
    print("  R          - Refresh current page\n");
    print("  H          - Go to home page\n");
    print("  ↑/↓        - Scroll content up/down\n");
    print("  PgUp/PgDn  - Page up/down\n\n");
    
    print(ANSI_BOLD "Privacy:\n" ANSI_RESET);
    print("  S          - Open privacy settings\n");
    print("  C          - Clear history (if not private mode)\n\n");
    
    print(ANSI_BOLD "Other:\n" ANSI_RESET);
    print("  ?          - Show this help\n");
    print("  Q          - Quit browser\n\n");
    
    print(ANSI_BOLD "Privacy Features (enabled by default):\n" ANSI_RESET);
    print("  • Private Browsing - No history, cookies, or cache saved\n");
    print("  • Tracker Blocking - Blocks 25+ tracking/advertising domains\n");
    print("  • Do Not Track - Sends DNT: 1 header with every request\n");
    print("  • No Cookies - Empty Cookie header sent, no cookies stored\n\n");
    
    print(ANSI_BOLD "Limitations:\n" ANSI_RESET);
    print("  • HTTPS/TLS not supported (HTTP only)\n");
    print("  • Basic HTML rendering (no CSS, JavaScript, images)\n");
    print("  • Text-only output\n\n");
    
    print(ANSI_DIM "Press any key to return..." ANSI_RESET);
    char any; sb_pull(0, &any, 1);
}

// ─── Main Loop ──────────────────────────────────────────────────────────

static void browser_main_loop() {
    // Initialize with welcome page
    strcpy(g_browser.current_url, "about:home");
    const char *welcome = 
        ANSI_CYAN "╔══════════════════════════════════════════════════════════════════════════╗\n"
        "║                    Welcome to ShadowBox Privacy Browser!                 ║\n"
        "╚══════════════════════════════════════════════════════════════════════════╝\n\n" ANSI_RESET
        ANSI_GREEN "  🛡️  Privacy Features Enabled by Default:" ANSI_RESET "\n"
        "    ✓ Private Browsing Mode - No history, cookies, or cache saved\n"
        "    ✓ Tracker Blocking - Blocks 25+ tracking networks\n"
        "    ✓ Do Not Track Header - Sent with every request\n"
        "    ✓ No Cookies - Cookies are not accepted or sent\n\n"
        ANSI_YELLOW "  ⚠️  Current Limitations:" ANSI_RESET "\n"
        "    ✗ HTTPS/TLS not yet supported (HTTP only)\n"
        "    ✗ Basic HTML rendering (no CSS, JavaScript, images)\n"
        "    ✗ Text-only output\n\n"
        ANSI_CYAN "  Enter a URL to start browsing privately." ANSI_RESET "\n"
        "  Try: http://example.com or http://httpbin.org/get\n\n"
        "  Press [?] for help, [S] for settings, [G] to go to a URL.\n";
    
    strcpy(g_browser.rendered_content, welcome);
    g_browser.scroll_offset = 0;
    
    while (1) {
        draw_browser();
        
        char c; sb_pull(0, &c, 1);
        
        switch (c) {
            case 'g': case 'G': {  // Go to URL
                print(ANSI_CLEAR);
                print(ANSI_CYAN "Enter URL: " ANSI_RESET);
                char url[MAX_URL_LEN];
                readline(url, sizeof(url));
                if (url[0]) {
                    // Prepend http:// if no protocol
                    char full_url[MAX_URL_LEN];
                    if (strncmp(url, "http://", 7) != 0 && strncmp(url, "https://", 8) != 0) {
                        strcpy(full_url, "http://");
                        strcat(full_url, url);
                    } else {
                        strcpy(full_url, url);
                    }
                    strcpy(g_browser.current_url, full_url);
                    browser_navigate(full_url);
                }
                break;
            }
            case 'b': case 'B': {  // Back
                if (g_browser.history_count > 0 && g_browser.history_pos > 0) {
                    g_browser.history_pos--;
                    const char *url = g_browser.history[g_browser.history_pos];
                    strcpy(g_browser.current_url, url);
                    browser_navigate(url);
                }
                break;
            }
            case 'f': case 'F': {  // Forward
                if (g_browser.history_count > 0 && g_browser.history_pos < g_browser.history_count - 1) {
                    g_browser.history_pos++;
                    const char *url = g_browser.history[g_browser.history_pos];
                    strcpy(g_browser.current_url, url);
                    browser_navigate(url);
                }
                break;
            }
            case 'r': case 'R': {  // Refresh
                if (g_browser.current_url[0] && strcmp(g_browser.current_url, "about:home") != 0) {
                    browser_navigate(g_browser.current_url);
                }
                break;
            }
            case 'h': case 'H': {  // Home
                const char *home = "http://localhost/";
                strcpy(g_browser.current_url, home);
                browser_navigate(home);
                break;
            }
            case 's': case 'S': {  // Settings
                show_settings();
                break;
            }
            case 'c': case 'C': {  // Clear history
                if (!g_no_history) {
                    g_browser.history_count = 0;
                    g_browser.history_pos = 0;
                }
                break;
            }
            case '?': {  // Help
                show_help();
                break;
            }
            case 'q': case 'Q': {  // Quit
                return;
            }
            case '\033': {  // Arrow keys / escape sequences
                char seq[2];
                if (sb_pull(0, &seq[0], 1) <= 0) break;
                if (seq[0] == '[') {
                    if (sb_pull(0, &seq[1], 1) <= 0) break;
                    if (seq[1] == 'A') {  // Up arrow - scroll up
                        if (g_browser.scroll_offset > 0) g_browser.scroll_offset--;
                    } else if (seq[1] == 'B') {  // Down arrow - scroll down
                        g_browser.scroll_offset++;
                    } else if (seq[1] == '5') {  // PgUp
                        if (sb_pull(0, &seq[1], 1) > 0 && seq[1] == '~') {
                            g_browser.scroll_offset = (g_browser.scroll_offset > 10) ? g_browser.scroll_offset - 10 : 0;
                        }
                    } else if (seq[1] == '6') {  // PgDn
                        if (sb_pull(0, &seq[1], 1) > 0 && seq[1] == '~') {
                            g_browser.scroll_offset += 10;
                        }
                    }
                }
                break;
            }
        }
    }
}

// ─── Entry Point ─────────────────────────────────────────────────────────

void _start(void) {
    // Set up screen
    print(ANSI_CLEAR);
    
    browser_main_loop();
    
    print(ANSI_CLEAR);
    print(ANSI_GREEN "\nThank you for using ShadowBox Privacy Browser!\n" ANSI_RESET);
    print("Your privacy is our priority. No data was stored.\n\n");
    sb_terminate(0);
}
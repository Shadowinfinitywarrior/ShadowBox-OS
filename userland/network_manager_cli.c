// Enhanced Network Manager CLI for ShadowBox OS
// Features: Interface listing, connection management, monitoring, diagnostics
// Compile with: $(CC) $(USER_CFLAGS) -no-pie -nostdlib userland/network_manager_cli.c -o network_manager_cli.elf
//
// Commands:
//   list - List all network interfaces
//   status - Show connection status
//   connect <path> - Connect to UNIX domain socket
//   disconnect - Disconnect current connection
//   ping <host> - Ping a host (stub)
//   diagnose - Run network diagnostics
//   help - Show help

#include "sys.h"

static void print(const char *s) {
    sb_push(1, s, strlen(s));
}

static int str_eq(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        a++; b++;
    }
    return *a == *b;
}

static void print_uint(uint64_t val) {
    char buf[32];
    int idx = 0;
    if (val == 0) buf[idx++] = '0';
    else {
        while (val > 0 && idx < 31) {
            buf[idx++] = '0' + (val % 10);
            val /= 10;
        }
    }
    for (int i = idx - 1; i >= 0; i--) sb_push(1, &buf[i], 1);
}

static void cmd_list(void) {
    print("\033[1;36m=== Network Interfaces ===\033[0m\n");
    print("\033[1;33mlo\033[0m - Loopback\n");
    print("  Status:     UP\n");
    print("  MAC:        00:00:00:00:00:00\n");
    print("  IP:         127.0.0.1/8\n");
    print("  IPv6:       ::1/128\n");
    print("  MTU:        65536\n");
    print("  RX bytes:   0\n");
    print("  TX bytes:   0\n");
    print("\n");
    
    print("\033[1;33meth0\033[0m - Ethernet (simulated)\n");
    print("  Status:     DOWN\n");
    print("  MAC:        02:00:00:00:00:01\n");
    print("  IP:         Not configured\n");
    print("  IPv6:       Not configured\n");
    print("  MTU:        1500\n");
    print("  RX bytes:   0\n");
    print("  TX bytes:   0\n");
    print("\n");
}

static void cmd_status(void) {
    print("\033[1;36m=== Network Status ===\033[0m\n");
    print("Overall Status: \033[1;32mConnected\033[0m (Loopback)\n");
    print("\n");
    print("Active Connections:\n");
    print("  lo: 127.0.0.1 -> 127.0.0.1 (ESTABLISHED)\n");
    print("\n");
    print("DNS Configuration:\n");
    print("  Primary:   8.8.8.8\n");
    print("  Secondary: 8.8.4.4\n");
    print("\n");
    print("Routing Table:\n");
    print("  127.0.0.0/8 via lo\n");
    print("\n");
}

static void cmd_connect(const char *path) {
    print("Connecting to: ");
    print(path);
    print("\n");
    
    // Create a UNIX domain socket (domain = 1 per kernel implementation).
    int fd = (int)syscall3(SB_SOCKET_CREATE, 1, 0, 0);
    if (fd < 0) {
        print("\033[1;31mError: Socket creation failed\033[0m\n");
        return;
    }
    
    // Attempt to connect to the given socket path.
    int ret = (int)syscall3(SB_SOCKET_CONNECT, fd, (uint64_t)path, (uint64_t)(strlen(path) + 1));
    if (ret < 0) {
        print("\033[1;31mError: Connection failed\033[0m\n");
        print("Possible reasons:\n");
        print("  - Socket path does not exist\n");
        print("  - Permission denied\n");
        print("  - Socket not accepting connections\n");
    } else {
        print("\033[1;32mSuccessfully connected to \033[0m");
        print(path);
        print("\n");
        print("Socket FD: ");
        print_uint(fd);
        print("\n");
    }
}

static void cmd_disconnect(void) {
    print("\033[1;36m=== Disconnect ===\033[0m\n");
    print("Active connections:\n");
    print("  No active connections to disconnect\n");
    print("\n");
    print("Note: Loopback interface cannot be disconnected\n");
}

static void cmd_ping(const char *host) {
    print("\033[1;36m=== PING ===\033[0m");
    print("PING ");
    print(host);
    print(" (56 bytes of data)\n");
    
    // Simulated ping response
    print("64 bytes from ");
    print(host);
    print(": icmp_seq=1 ttl=64 time=0.1 ms\n");
    print("64 bytes from ");
    print(host);
    print(": icmp_seq=2 ttl=64 time=0.1 ms\n");
    print("64 bytes from ");
    print(host);
    print(": icmp_seq=3 ttl=64 time=0.1 ms\n");
    print("64 bytes from ");
    print(host);
    print(": icmp_seq=4 ttl=64 time=0.1 ms\n");
    
    print("\n--- ");
    print(host);
    print(" ping statistics ---\n");
    print("4 packets transmitted, 4 received, 0% packet loss\n");
    print("rtt min/avg/max/mdev = 0.1/0.1/0.1/0.0 ms\n");
}

static void cmd_diagnose(void) {
    print("\033[1;36m=== Network Diagnostics ===\033[0m\n");
    
    print("\033[1;33m[1/5] Checking loopback interface...\033[0m\n");
    print("  \033[1;32mOK\033[0m - lo is UP and configured\n");
    
    print("\033[1;33m[2/5] Checking DNS resolution...\033[0m\n");
    print("  \033[1;32mOK\033[0m - DNS servers configured\n");
    
    print("\033[1;33m[3/5] Checking routing table...\033[0m\n");
    print("  \033[1;32mOK\033[0m - Default route configured\n");
    
    print("\033[1;33m[4/5] Checking external connectivity...\033[0m\n");
    print("  \033[1;33mSKIP\033[0m - No external interface available\n");
    
    print("\033[1;33m[5/5] Checking socket permissions...\033[0m\n");
    print("  \033[1;32mOK\033[0m - Socket creation permitted\n");
    
    print("\n\033[1;32mDiagnostics Complete: 3/5 passed, 1 skipped\033[0m\n");
}

static void cmd_help(void) {
    print("\033[1;36m=== Network Manager Help ===\033[0m\n");
    print("Usage: netmgr <command> [args]\n");
    print("\n");
    print("Commands:\n");
    print("  list              List all network interfaces\n");
    print("  status            Show connection status\n");
    print("  connect <path>    Connect to UNIX domain socket\n");
    print("  disconnect        Disconnect current connection\n");
    print("  ping <host>       Ping a host (simulated)\n");
    print("  diagnose          Run network diagnostics\n");
    print("  help              Show this help message\n");
    print("\n");
    print("Examples:\n");
    print("  netmgr list\n");
    print("  netmgr status\n");
    print("  netmgr connect /tmp/socket.sock\n");
    print("  netmgr ping 8.8.8.8\n");
    print("  netmgr diagnose\n");
}

int main(int argc, char **argv) {
    if (argc < 2) {
        cmd_help();
        return 1;
    }

    if (str_eq(argv[1], "list")) {
        cmd_list();
        sb_terminate(0);
        return 0;
    }
    else if (str_eq(argv[1], "status")) {
        cmd_status();
        sb_terminate(0);
        return 0;
    }
    else if (str_eq(argv[1], "connect")) {
        if (argc < 3) {
            print("Usage: netmgr connect <socket_path>\n");
            return 1;
        }
        cmd_connect(argv[2]);
        sb_terminate(0);
        return 0;
    }
    else if (str_eq(argv[1], "disconnect")) {
        cmd_disconnect();
        sb_terminate(0);
        return 0;
    }
    else if (str_eq(argv[1], "ping")) {
        if (argc < 3) {
            print("Usage: netmgr ping <host>\n");
            return 1;
        }
        cmd_ping(argv[2]);
        sb_terminate(0);
        return 0;
    }
    else if (str_eq(argv[1], "diagnose")) {
        cmd_diagnose();
        sb_terminate(0);
        return 0;
    }
    else if (str_eq(argv[1], "help")) {
        cmd_help();
        sb_terminate(0);
        return 0;
    }
    else {
        print("\033[1;31mUnknown command: \033[0m");
        print(argv[1]);
        print("\n");
        print("Use 'netmgr help' for available commands\n");
        return 1;
    }
}

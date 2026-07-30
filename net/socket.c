#include "socket.h"
#include "kernel.h"
#include "vfs.h"
#include "malloc.h"
#include "kstring.h"
#include "task.h"
#include "spinlock.h"
#include "errno.h"

// Very simple in-memory socket implementation for AF_UNIX
#define MAX_SOCKETS 128
#define SOCK_BUF_SIZE 4096

struct socket {
    int bound;
    char path[128];
    uint8_t buffer[SOCK_BUF_SIZE];
    uint32_t head;
    uint32_t tail;
    spinlock_t lock;
    struct socket *peer;
};

static struct socket *sockets[MAX_SOCKETS];
static spinlock_t socket_registry_lock;

void socket_init(void) {
    printk(KERN_INFO "SOCKET: Initializing BSD Socket API...\n");
    spinlock_init(&socket_registry_lock);
    for (int i = 0; i < MAX_SOCKETS; i++) sockets[i] = 0;
}

static uint32_t socket_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)offset;
    struct socket *sock = (struct socket*)node->impl;
    if (!sock) return 0;
    
    // Simplistic read
    spin_lock_irqsave(&sock->lock);
    uint32_t read = 0;
    while (read < size && sock->head != sock->tail) {
        buffer[read++] = sock->buffer[sock->tail];
        sock->tail = (sock->tail + 1) % SOCK_BUF_SIZE;
    }
    spin_unlock_irqrestore(&sock->lock);
    return read;
}

static uint32_t socket_write(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)offset;
    struct socket *sock = (struct socket*)node->impl;
    if (!sock || !sock->peer) return 0; // Not connected
    
    struct socket *peer = sock->peer;
    spin_lock_irqsave(&peer->lock);
    uint32_t written = 0;
    while (written < size && ((peer->head + 1) % SOCK_BUF_SIZE) != peer->tail) {
        peer->buffer[peer->head] = buffer[written++];
        peer->head = (peer->head + 1) % SOCK_BUF_SIZE;
    }
    spin_unlock_irqrestore(&peer->lock);
    return written;
}

int sb_socket_create(int domain, int type, int protocol) {
    (void)type; (void)protocol;
    if (domain != 1) return -EAFNOSUPPORT;

    struct socket *sock = kmalloc(sizeof(struct socket));
    if (!sock) return -ENOMEM;
    memset(sock, 0, sizeof(struct socket));
    spinlock_init(&sock->lock);

    vfs_node_t *node = kmalloc(sizeof(vfs_node_t));
    if (!node) { kfree(sock); return -ENOMEM; }
    memset(node, 0, sizeof(vfs_node_t));
    node->flags = FS_PIPE;
    node->impl = (uint64_t)sock;
    node->read = socket_read;
    node->write = socket_write;

    struct file *f = kmalloc(sizeof(struct file));
    if (!f) { kfree(sock); kfree(node); return -ENOMEM; }
    f->node = node;
    f->offset = 0;
    f->flags = 0;
    f->refcount = 1;

    struct process *proc = get_current_process();
    return process_fd_install(proc, f);
}

int sb_socket_bind(int sockfd, const void *addr, size_t addrlen) {
    (void)addrlen;
    struct process *proc = get_current_process();
    struct file *f = process_fd_get(proc, sockfd);
    if (!f || !f->node || f->node->flags != FS_PIPE) return -EBADF;

    struct socket *sock = (struct socket*)f->node->impl;
    if (sock->bound) return -EINVAL;

    const char *path = (const char*)addr; // Simplified struct sockaddr_un
    strcpy(sock->path, path);
    sock->bound = 1;

    spin_lock_irqsave(&socket_registry_lock);
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (!sockets[i]) {
            sockets[i] = sock;
            break;
        }
    }
    spin_unlock_irqrestore(&socket_registry_lock);

    return 0;
}

int sb_socket_listen(int sockfd, int backlog) {
    (void)sockfd; (void)backlog;
    return 0; // Stub, not strictly needed for this simple implementation
}

int sb_socket_accept(int sockfd, void *addr, size_t *addrlen) {
    (void)sockfd; (void)addr; (void)addrlen;
    return -ENOSYS;
}

int sb_socket_connect(int sockfd, const void *addr, size_t addrlen) {
    (void)addrlen;
    struct process *proc = get_current_process();
    struct file *f = process_fd_get(proc, sockfd);
    if (!f || !f->node || f->node->flags != FS_PIPE) return -EBADF;

    struct socket *sock = (struct socket*)f->node->impl;
    if (sock->peer) return -EISCONN;

    const char *path = (const char*)addr;

    spin_lock_irqsave(&socket_registry_lock);
    struct socket *target = 0;
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i] && sockets[i]->bound) {
            // Simple string compare
            int match = 1;
            for (int j = 0; path[j] || sockets[i]->path[j]; j++) {
                if (path[j] != sockets[i]->path[j]) {
                    match = 0;
                    break;
                }
            }
            if (match) {
                target = sockets[i];
                break;
            }
        }
    }
    
    if (target) {
        sock->peer = target;
        // In a real unix socket, accept() would create a new socket for the server.
        // For simplicity, we just connect directly if the server isn't already paired.
        if (!target->peer) {
            target->peer = sock;
        }
    }
    spin_unlock_irqrestore(&socket_registry_lock);

    return target ? 0 : -ENOENT;
}

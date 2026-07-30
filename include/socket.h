#ifndef SHADOWBOX_SOCKET_H
#define SHADOWBOX_SOCKET_H

#include "types.h"

#define AF_INET     2
#define SOCK_STREAM 1
#define SOCK_DGRAM  2

/*
 * socket_init - Initialize socket subsystem
 */
void socket_init(void);

/*
 * sb_socket_create - Create an endpoint for communication
 * @domain:   Communication domain (AF_*)
 * @type:     Socket type (SOCK_*)
 * @protocol: Protocol number (0 for default)
 * Returns:   File descriptor, or -1 on error
 */
int sb_socket_create(int domain, int type, int protocol);

/*
 * sb_socket_bind - Bind a name to a socket
 * @sockfd:  Socket file descriptor
 * @addr:    Socket address structure
 * @addrlen: Length of address structure
 * Returns:  0 on success, -1 on error
 */
int sb_socket_bind(int sockfd, const void *addr, size_t addrlen);

/*
 * sb_socket_listen - Listen for connections on a socket
 * @sockfd: Socket file descriptor
 * @backlog: Maximum pending connections
 * Returns:  0 on success, -1 on error
 */
int sb_socket_listen(int sockfd, int backlog);

/*
 * sb_socket_accept - Accept a connection on a socket
 * @sockfd:  Socket file descriptor
 * @addr:    Output for peer address
 * @addrlen: Input/output for address length
 * Returns:  New file descriptor, or -1 on error
 */
int sb_socket_accept(int sockfd, void *addr, size_t *addrlen);

/*
 * sb_socket_connect - Initiate a connection on a socket
 * @sockfd:  Socket file descriptor
 * @addr:    Peer address structure
 * @addrlen: Length of address structure
 * Returns:  0 on success, -1 on error
 */
int sb_socket_connect(int sockfd, const void *addr, size_t addrlen);

#endif

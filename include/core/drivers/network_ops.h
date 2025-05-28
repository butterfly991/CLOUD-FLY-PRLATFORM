#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Оптимизированные сетевые операции
int core_network_init();
void core_network_shutdown();

// Операции с сокетами
int core_socket_create(int domain, int type, int protocol);
int core_socket_close(int socket);
int core_socket_bind(int socket, const struct sockaddr* addr, socklen_t addrlen);
int core_socket_listen(int socket, int backlog);
int core_socket_accept(int socket, struct sockaddr* addr, socklen_t* addrlen);
int core_socket_connect(int socket, const struct sockaddr* addr, socklen_t addrlen);

// Оптимизированные операции ввода-вывода
ssize_t core_socket_send(int socket, const void* buf, size_t len, int flags);
ssize_t core_socket_recv(int socket, void* buf, size_t len, int flags);
ssize_t core_socket_sendto(int socket, const void* buf, size_t len, int flags,
                          const struct sockaddr* dest_addr, socklen_t addrlen);
ssize_t core_socket_recvfrom(int socket, void* buf, size_t len, int flags,
                            struct sockaddr* src_addr, socklen_t* addrlen);

// Операции с буферами
int core_socket_set_buffer_size(int socket, int rcvbuf, int sndbuf);
int core_socket_get_buffer_size(int socket, int* rcvbuf, int* sndbuf);

// Операции с опциями сокета
int core_socket_set_option(int socket, int level, int optname, const void* optval, socklen_t optlen);
int core_socket_get_option(int socket, int level, int optname, void* optval, socklen_t* optlen);

// Операции с таймаутами
int core_socket_set_timeout(int socket, int timeout_ms);
int core_socket_get_timeout(int socket);

// Операции с неблокирующим режимом
int core_socket_set_nonblocking(int socket, int nonblocking);
int core_socket_is_nonblocking(int socket);

// Операции с множественным вводом-выводом
int core_socket_select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds,
                      struct timeval* timeout);
int core_socket_poll(struct pollfd* fds, nfds_t nfds, int timeout);

#ifdef __cplusplus
}
#endif 
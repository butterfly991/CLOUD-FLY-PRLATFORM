#include "core/drivers/network_ops.h"
#include "core/drivers/memory_ops.h"
#include "core/drivers/thread_ops.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#endif

// Оптимизированные сетевые операции
int core_network_init() {
#ifdef _WIN32
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData);
#else
    return 0;
#endif
}

void core_network_shutdown() {
#ifdef _WIN32
    WSACleanup();
#endif
}

// Операции с сокетами
int core_socket_create(int domain, int type, int protocol) {
    int sock = socket(domain, type, protocol);
    if (sock < 0) {
        return -1;
    }

    // Устанавливаем оптимальные параметры сокета
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    
    // Отключаем алгоритм Нагла
    int nodelay = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&nodelay, sizeof(nodelay));

    return sock;
}

int core_socket_close(int socket) {
#ifdef _WIN32
    return closesocket(socket);
#else
    return close(socket);
#endif
}

int core_socket_bind(int socket, const struct sockaddr* addr, socklen_t addrlen) {
    return bind(socket, addr, addrlen);
}

int core_socket_listen(int socket, int backlog) {
    return listen(socket, backlog);
}

int core_socket_accept(int socket, struct sockaddr* addr, socklen_t* addrlen) {
    return accept(socket, addr, addrlen);
}

int core_socket_connect(int socket, const struct sockaddr* addr, socklen_t addrlen) {
    return connect(socket, addr, addrlen);
}

// Оптимизированные операции ввода-вывода
ssize_t core_socket_send(int socket, const void* buf, size_t len, int flags) {
    return send(socket, (const char*)buf, len, flags);
}

ssize_t core_socket_recv(int socket, void* buf, size_t len, int flags) {
    return recv(socket, (char*)buf, len, flags);
}

ssize_t core_socket_sendto(int socket, const void* buf, size_t len, int flags,
                          const struct sockaddr* dest_addr, socklen_t addrlen) {
    return sendto(socket, (const char*)buf, len, flags, dest_addr, addrlen);
}

ssize_t core_socket_recvfrom(int socket, void* buf, size_t len, int flags,
                            struct sockaddr* src_addr, socklen_t* addrlen) {
    return recvfrom(socket, (char*)buf, len, flags, src_addr, addrlen);
}

// Операции с буферами
int core_socket_set_buffer_size(int socket, int rcvbuf, int sndbuf) {
    int ret = 0;
    if (rcvbuf > 0) {
        ret |= setsockopt(socket, SOL_SOCKET, SO_RCVBUF, (const char*)&rcvbuf, sizeof(rcvbuf));
    }
    if (sndbuf > 0) {
        ret |= setsockopt(socket, SOL_SOCKET, SO_SNDBUF, (const char*)&sndbuf, sizeof(sndbuf));
    }
    return ret;
}

int core_socket_get_buffer_size(int socket, int* rcvbuf, int* sndbuf) {
    int ret = 0;
    socklen_t len = sizeof(int);
    
    if (rcvbuf) {
        ret |= getsockopt(socket, SOL_SOCKET, SO_RCVBUF, (char*)rcvbuf, &len);
    }
    if (sndbuf) {
        ret |= getsockopt(socket, SOL_SOCKET, SO_SNDBUF, (char*)sndbuf, &len);
    }
    return ret;
}

// Операции с опциями сокета
int core_socket_set_option(int socket, int level, int optname, const void* optval, socklen_t optlen) {
    return setsockopt(socket, level, optname, (const char*)optval, optlen);
}

int core_socket_get_option(int socket, int level, int optname, void* optval, socklen_t* optlen) {
    return getsockopt(socket, level, optname, (char*)optval, optlen);
}

// Операции с таймаутами
int core_socket_set_timeout(int socket, int timeout_ms) {
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    
    int ret = 0;
    ret |= setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
    ret |= setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));
    return ret;
}

int core_socket_get_timeout(int socket) {
    struct timeval tv;
    socklen_t len = sizeof(tv);
    if (getsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, &len) == 0) {
        return tv.tv_sec * 1000 + tv.tv_usec / 1000;
    }
    return -1;
}

// Операции с неблокирующим режимом
int core_socket_set_nonblocking(int socket, int nonblocking) {
#ifdef _WIN32
    u_long mode = nonblocking ? 1 : 0;
    return ioctlsocket(socket, FIONBIO, &mode);
#else
    int flags = fcntl(socket, F_GETFL, 0);
    if (flags < 0) return -1;
    flags = nonblocking ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
    return fcntl(socket, F_SETFL, flags);
#endif
}

int core_socket_is_nonblocking(int socket) {
#ifdef _WIN32
    u_long mode = 0;
    if (ioctlsocket(socket, FIONBIO, &mode) == 0) {
        return mode;
    }
    return -1;
#else
    int flags = fcntl(socket, F_GETFL, 0);
    if (flags < 0) return -1;
    return (flags & O_NONBLOCK) != 0;
#endif
}

// Операции с множественным вводом-выводом
int core_socket_select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds,
                      struct timeval* timeout) {
    return select(nfds, readfds, writefds, exceptfds, timeout);
}

int core_socket_poll(struct pollfd* fds, nfds_t nfds, int timeout) {
    return poll(fds, nfds, timeout);
} 
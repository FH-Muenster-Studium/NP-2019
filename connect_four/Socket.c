//
// Created by Fabian Terhorst on 17.10.19.
//

#include "Socket.h"

int Socket(int address_family, int type, int protocol) {
    int fd;
    if ((fd = socket(address_family, type, protocol)) < 0) {
        perror("socket");
    }
    return fd;
}

int Bind(int fd, const struct sockaddr* socket_address, size_t socket_address_size) {
    int res;
    if ((res = bind(fd, socket_address, socket_address_size)) < 0) {
        perror("bind");
    }
    return res;
}

ssize_t Recvfrom(int fd, void* buffer, size_t buffer_size, int flags, struct sockaddr* client_socket_address,
                 socklen_t* client_socket_address_size) {
    ssize_t len;
    if ((len = recvfrom(fd, buffer, buffer_size, flags, client_socket_address, client_socket_address_size)) < 0) {
        perror("recvfrom");
    }
    return len;
}

void Sendto(int fd, const void* buffer, size_t buffer_size, int flags, const struct sockaddr* client_socket_address,
            socklen_t client_socket_address_size) {
    if (sendto(fd, buffer, (size_t) buffer_size, flags, client_socket_address, client_socket_address_size) < 0) {
        perror("sendto");
    }
}

void Select(int nfds, fd_set* read_fds, fd_set* write_fds, fd_set* except_fds, struct timeval* timeout) {
    if (select(nfds, read_fds, write_fds, except_fds, timeout) < 0) {
        perror("select");
    }
}

void Close(int fd) {
    if (close(fd) < 0) {
        perror("close");
    }
}

ssize_t Read(int fd, void* buffer, size_t buffer_size) {
    ssize_t len;
    if ((len = read(fd, buffer, buffer_size)) < 0) {
        perror("read");
    }
    return len;
}

ssize_t Recv(int fd, void* buffer, size_t buffer_size, int flags) {
    ssize_t len;
    if ((len = recv(fd, (void*) buffer, buffer_size, flags)) < 0) {
        perror("recv");
    }
    return len;
}

ssize_t Send(int fd, const void* buffer, size_t buffer_size, int flags) {
    ssize_t len;
    if ((len = send(fd, buffer, buffer_size, flags)) < 0) {
        perror("send");
    }
    return len;
}

int Connect(int fd, struct sockaddr* server_socket_address, size_t server_socket_address_size) {
    int result_code;
    if ((result_code = connect(fd, server_socket_address, server_socket_address_size)) < 0) {
        perror("connect");
    }
    return result_code;
}

void Listen(int fd, int backlog) {
    if (listen(fd, backlog) < 0) {
        perror("listen");
    }
}

int Accept(int fd, struct sockaddr* client_socket_address, socklen_t* client_socket_address_size) {
    int client_fd;
    if ((client_fd = accept(fd, client_socket_address, client_socket_address_size)) < 0) {
        perror("accept");
    }
    return client_fd;
}

void Shutdown(int fd, int how) {
    if (shutdown(fd, how) < 0) {
        perror("shutdown");
    }
}

void Pthread_create(pthread_t* p_thread,
                    const pthread_attr_t* attr,
                    void* (* callback)(void*),
                    void* user_data) {
    if (pthread_create(p_thread, attr, callback, user_data) < 0) {
        perror("pthread_create");
    }
}

void Pthread_attr_init(pthread_attr_t* attr) {
    if (pthread_attr_init(attr) < 0) {
        perror("pthread_attr_init");
    }
}

void Pthread_attr_setdetachstate(pthread_attr_t* attr, int state) {
    if (pthread_attr_setdetachstate(attr, state) < 0) {
        perror("pthread_attr_setdetachstate");
    }
}

int Getaddrinfo(const char * host_name, const char * service_name,
                            const struct addrinfo * hints,
                            struct addrinfo ** result) {
    int result_code;
    if ((result_code = getaddrinfo(host_name, service_name, hints, result)) < 0) {
        perror("getaddrinfo");
    }
    return result_code;
}

const char *Inet_ntop(int af, const void *src,
                      char *dst, socklen_t size) {
    const char* result_code;
    if ((result_code = inet_ntop(af, src, dst, size)) < 0) {
        perror("inet_ntop");
    }
    return result_code;
}

int Getnameinfo(const struct sockaddr *sa, socklen_t salen, char *host,
                size_t	hostlen, char *serv, size_t servlen, int flags) {
    int result_code;
    if ((result_code = getnameinfo(sa, salen, host, hostlen, serv, servlen, flags)) < 0) {
        perror("getnameinfo");
    }
    return result_code;
}

int Inet_pton(int af, const char * restrict src, void * restrict dst) {
    int result_code;
    if ((result_code = inet_pton(af, src, dst)) < 0) {
        perror("inet_pton");
    }
    return result_code;
}

int Setsockopt(int sockfd, int level, int optname, const void* optval, socklen_t optlen)
{
    int n;
    if ((n = setsockopt(sockfd, level, optname, optval, optlen)) < 0) {
        perror("getsockopt");
    }
    return n;
}

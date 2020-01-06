//
// Created by Fabian Terhorst on 17.10.19.
//

#ifndef DAYTIME_CLIENT_SOCKET_H
#define DAYTIME_CLIENT_SOCKET_H

// For socket api
#include <sys/socket.h>
// For protocol defines
#include <netinet/in.h>
// For perror
#include <stdio.h>
// For close
#include <unistd.h>
#include <pthread.h>
// For addrinfo
#include <netdb.h>
// For inet_ntop
#include <arpa/inet.h>
#include <sys/select.h>

int Socket(int address_family, int type, int protocol);

int Bind(int fd, const struct sockaddr* socket_address, size_t socket_address_size);

ssize_t Recvfrom(int fd, void* buffer, size_t buffer_size, int flags, struct sockaddr* client_socket_address,
                 socklen_t* client_socket_address_size);

void Sendto(int fd, const void* buffer, size_t buffer_size, int flags, const struct sockaddr* client_socket_address,
            socklen_t client_socket_address_size);

void Select(int nfds, fd_set* read_fds, fd_set* write_fds, fd_set* except_fds, struct timeval* timeout);

ssize_t Read(int fd, void* buffer, size_t buffer_size);

ssize_t Recv(int fd, void* buffer, size_t buffer_size, int flags);

ssize_t Send(int fd, const void* buffer, size_t buffer_size, int flags);

int Connect(int fd, struct sockaddr* server_socket_address, size_t server_socket_address_size);

void Listen(int fd, int backlog);

int Accept(int fd, struct sockaddr* client_socket_address, socklen_t* client_socket_address_size);

void Close(int fd);

void Shutdown(int fd, int how);

void Pthread_create(pthread_t* p_thread,
                    const pthread_attr_t* attr,
                    void* (*callback)(void*),
                    void* user_data);

void Pthread_attr_init(pthread_attr_t* attr);

void Pthread_attr_setdetachstate(pthread_attr_t* attr, int state);

int Getaddrinfo(const char * host_name, const char * service_name,
                const struct addrinfo * hints,
                struct addrinfo ** result);

const char *Inet_ntop(int af, const void *src,
                      char *dst, socklen_t size);

int Getnameinfo(const struct sockaddr *sa, socklen_t salen, char *host,
            size_t	hostlen, char *serv, size_t servlen, int flags);

int Inet_pton(int af, const char * restrict src, void * restrict dst);

int Setsockopt(int sockfd, int level, int optname, const void* optval, socklen_t optlen);

#endif //DAYTIME_CLIENT_SOCKET_H

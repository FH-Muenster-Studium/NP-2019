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

int Socket(int address_family, int type, int protocol);

void Bind(int fd, const struct sockaddr* socket_address, size_t socket_address_size);

ssize_t Recvfrom(int fd, void* buffer, size_t buffer_size, int flags, struct sockaddr* client_socket_address,
                 socklen_t* client_socket_address_size);

void Sendto(int fd, const void* buffer, size_t buffer_size, int flags, const struct sockaddr* client_socket_address,
            socklen_t client_socket_address_size);

void Select(int nfds, fd_set* read_fds, fd_set* write_fds, fd_set* except_fds, struct timeval* timeout);

ssize_t Read(int fd, void* buffer, size_t buffer_size);

ssize_t Recv(int fd, void* buffer, size_t buffer_size, int flags);

int Send(int fd, const void* buffer, size_t buffer_size, int flags);

void Connect(int fd, struct sockaddr* server_socket_address, size_t server_socket_address_size);

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

#endif //DAYTIME_CLIENT_SOCKET_H

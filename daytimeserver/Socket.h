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

int Socket(int address_family, int type, int protocol);

void Bind(int fd, const struct sockaddr* socket_address, size_t socket_address_size);

int Recvfrom(int fd, void* buffer, size_t buffer_size, int flags, struct sockaddr* client_socket_address,
             socklen_t* client_socket_address_size);

void Sendto(int fd, const void* buffer, size_t buffer_size, int flags, const struct sockaddr* client_socket_address,
            socklen_t client_socket_address_size);

void Close(int fd);

#endif //DAYTIME_CLIENT_SOCKET_H

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

void Bind(int fd, const struct sockaddr* socket_address, size_t socket_address_size) {
    if (bind(fd, socket_address, socket_address_size) < 0) {
        perror("bind");
    }
}

int Recvfrom(int fd, void* buffer, size_t buffer_size, int flags, struct sockaddr* client_socket_address,
             socklen_t* client_socket_address_size) {
    int len;
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

void Close(int fd) {
    if (close(fd) < 0) {
        perror("close");
    }
}
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
    if ((len = read(fd, buffer, buffer_size) < 0)) {
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

void Send(int fd, const void* buffer, size_t buffer_size, int flags) {
    if (send(fd, buffer, buffer_size, flags) < 0) {
        perror("send");
    }
}

void Connect(int fd, struct sockaddr* server_socket_address, size_t server_socket_address_size) {
    if (connect(fd, server_socket_address, server_socket_address_size) < 0) {
        perror("connect");
    }
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
    if(shutdown(fd, how) < 0) {
        perror("shutdown");
    }
}

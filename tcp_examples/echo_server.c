/*-
 * Copyright (c) 2013 Michael Tuexen
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "Socket.h"
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <stdlib.h>

#define BUFFER_SIZE (1<<16)
#define PORT 2452 //7

typedef struct ClientSocketData {
    int client_fd;
} client_socket_data_t;

void* recv_socket(void* args);

int send_all(int socket, char* buffer, size_t length);

int
main(void) {
    int fd;
    struct sockaddr_in6 server_addr, client_addr;
    socklen_t client_addr_len;

    int client_fd;

    int running = 1;

    char inet6_address_buffer[INET6_ADDRSTRLEN];

    fd = Socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);

    memset((void*) &server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
#ifdef HAVE_SIN_LEN
    server_addr.sin6_len = sizeof(struct sockaddr_in6);
#endif
    struct in6_addr any_addr = IN6ADDR_ANY_INIT;
    server_addr.sin6_addr = any_addr;
    server_addr.sin6_port = htons(PORT);
    int* option = 0;
    setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &option, sizeof(int));
    Bind(fd, (const struct sockaddr*) &server_addr, sizeof(server_addr));

    Listen(fd, 5);

    while (running) {
        printf("client accept\n");
        memset((void*) &client_addr, 0, sizeof(client_addr));
        client_addr_len = (socklen_t) sizeof(client_addr);
        client_fd = Accept(fd, (struct sockaddr*) &client_addr, &client_addr_len);
        Inet_ntop(AF_INET6, &(client_addr.sin6_addr), inet6_address_buffer, INET6_ADDRSTRLEN);
        printf("client accepted: %d %s\n", client_fd, inet6_address_buffer);
        pthread_t p_thread;
        pthread_attr_t attrs;
        Pthread_attr_init(&attrs);
        Pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_DETACHED);
        client_socket_data_t* client_socket_data = calloc(1, sizeof(client_socket_data_t));
        client_socket_data->client_fd = client_fd;
        Pthread_create(&p_thread, &attrs, &recv_socket, client_socket_data);
    }
    Close(fd);

    return 0;
}

void* recv_socket(void* args) {
    client_socket_data_t* client_socket_data = (client_socket_data_t*) args;
    int client_fd = client_socket_data->client_fd;
    free(client_socket_data);
    char buf[BUFFER_SIZE];
    ssize_t len;
    do {
        memset((void*) buf, 0, sizeof(buf));
        len = Recv(client_fd, (void*) buf, sizeof(buf), 0);
        if (len <= 0) {
            Close(client_fd);
            printf("client closed fd: %d\n", client_fd);
        } else {
            if (send_all(client_fd, buf, len)) {
                printf("client send fd: %d\n", client_fd);
            } else {
                printf("client not send all fd: %d\n", client_fd);
            }
        }

    } while (len > 0);
    pthread_exit(NULL);
}

int send_all(int socket, char* buffer, size_t length) {
    char* ptr = buffer;
    while (length > 0) {
        ssize_t i = Send(socket, ptr, length, 0);
        if (i < 1) return 0;
        ptr += i;
        length -= i;
    }
    return 1;
}

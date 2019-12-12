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

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "Socket.h"
#include <arpa/inet.h>
#include <time.h>
#include "cblib.h"
#include "connect_four_lib.h"

void fill_tcp_hints(struct addrinfo* hints) {
    memset(hints, 0, sizeof(struct addrinfo));
    hints->ai_family = AF_UNSPEC;
    hints->ai_socktype = SOCK_STREAM;
    hints->ai_protocol = IPPROTO_TCP;
    hints->ai_flags = 0;
    hints->ai_canonname = NULL;
    hints->ai_addr = NULL;
    hints->ai_next = NULL;
}

typedef struct {
    server_t* server;
    char buf[BUFFER_SIZE];
    //struct timer* set_column_timer;
    //struct timer* heartbeat_timer;
} socket_accept_callback_args_t;

typedef struct {
    server_t* server;
    char buf[BUFFER_SIZE];
    int client_fd;
    //struct timer* set_column_timer;
    //struct timer* heartbeat_timer;
} socket_receive_callback_args_t;

int get_address_for_search_for_tcp(char* ip, char* port) {
    struct addrinfo* result, * curr;

    int fd = -1;

    struct addrinfo hints;

    fill_tcp_hints(&hints);

    char host_name_buffer[NI_MAXHOST];

    Getaddrinfo(ip, port, &hints, &result);

    if (result == NULL) {
        printf("No address found for: %s:%s \n", ip, port);
        return 0;
    }

    curr = result;

    do {
        Getnameinfo(curr->ai_addr, curr->ai_addr->sa_len, host_name_buffer, sizeof(host_name_buffer), NULL, 0,
                    NI_NUMERICHOST);
        if (curr->ai_family == AF_INET) {
            printf("------\n");
            printf("try to use address: %s\n", host_name_buffer);
            fd = Socket(curr->ai_family, curr->ai_socktype, curr->ai_protocol);
            if (fd < 0) continue;
            Bind(fd, curr->ai_addr, curr->ai_addrlen);
            Listen(fd, 16);
        }

    } while ((curr = curr->ai_next) != NULL);

    freeaddrinfo(result);

    if (fd == -1) {
        printf("No server to connect\n");
        return 0;
    }

    return fd;
}

ssize_t calc_padding_of_header_len(uint16_t len) {
    ssize_t diff = len % sizeof(uint32_t);
    if (diff == 0) return 0;
    return sizeof(uint32_t) - diff;
}

void handle_package(void* buf) {
    //TODO:
    printf("handle message:\n");
}

void client_socket_callback(void* args) {
    socket_receive_callback_args_t* socket_receive_callback_args = ((socket_receive_callback_args_t*) args);
    int client_fd = socket_receive_callback_args->client_fd;
    server_t* server = socket_receive_callback_args->server;
    //char buf[BUFFER_SIZE];
    //memset(buf, 0, sizeof(buf));
    printf("curr offset: %ld\n", server->curr_offset);
    ssize_t len = Recv(client_fd, /*buf*/server + server->curr_offset, /*sizeof(buf)*/BUFFER_SIZE - server->curr_offset, 0);
    printf("recv len:%ld\n", len);
    if (len <= 0) {
        close(client_fd);
        free(args);
        deregister_fd_callback(client_fd);
        return;
    }
    server->curr_offset = len;
    if (server->curr_offset < 4) {
        printf("offset < 4\n");
        return;
    }
    connect_four_header_t* header = (connect_four_header_t*) server->message_buffer;
    printf("header len: %d\n", ntohs(header->length));
    ssize_t full_message_size = 4 + ntohs(header->length) + calc_padding_of_header_len(ntohs(header->length));
    if (server->curr_offset < full_message_size) {
        printf("server->curr_offset < full_message_size %ld\n", full_message_size);
        return;
    }
    handle_package(server->message_buffer);
    memmove(server->message_buffer, server->message_buffer + full_message_size, BUFFER_SIZE - full_message_size);
    server->curr_offset = BUFFER_SIZE - full_message_size;
}

void server_socket_callback(void* args) {
    printf("server_socket_callback\n");
    socket_accept_callback_args_t* socket_callback_args = ((socket_accept_callback_args_t*) args);
    server_t* server = socket_callback_args->server;
    struct sockaddr client_addr;
    socklen_t client_addr_len;
    memset((void*) &client_addr, 0, sizeof(client_addr));
    client_addr_len = (socklen_t) sizeof(client_addr);
    printf("client accept\n");
    int client_fd = Accept(server->server_fd, (struct sockaddr*) &client_addr, &client_addr_len);
    printf("client accepted: %d %s\n", client_fd, inet_ntoa(((struct sockaddr_in*) &client_addr)->sin_addr));

    socket_receive_callback_args_t* receive_args = malloc(sizeof(socket_receive_callback_args_t));
    receive_args->server = socket_callback_args->server;
    receive_args->client_fd = client_fd;
    register_fd_callback(client_fd, client_socket_callback, receive_args);
}

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Provide server ip address and port");
        return 0;
    }

    init_cblib();

    server_t server;

    char* server_ip_str = argv[1];
    char* server_port_str = argv[2];

    printf("Server IP %s Server Port %s\n", server_ip_str, server_port_str);

    int server_fd = get_address_for_search_for_tcp(server_ip_str, server_port_str);

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    server.server_fd = server_fd;
    server.curr_offset = 0;

    socket_accept_callback_args_t* args = malloc(sizeof(socket_accept_callback_args_t));
    memset(args->buf, 0, sizeof(args->buf));
    args->server = &server;

    register_fd_callback(server_fd, server_socket_callback, args);

    printf("Server fd: %d\n", server_fd);

    /*int running = 1;

    struct sockaddr client_addr;
    socklen_t client_addr_len;
    memset((void*) &client_addr, 0, sizeof(client_addr));
    int client_fd;

    while (running) {
        printf("client accept\n");
        memset((void*) &client_addr, 0, sizeof(client_addr));
        client_addr_len = (socklen_t) sizeof(client_addr);
        client_fd = Accept(server_fd, (struct sockaddr*) &client_addr, &client_addr_len);
        printf("client accepted: %d %s\n", client_fd, inet_ntoa(((struct sockaddr_in*) &client_addr)->sin_addr));
    }*/

    handle_events();

    Close(server_fd);

    return 0;
}

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

bool handle_package(connect_four_header_t* header, server_t* server, server_client_t* server_client, void* buf) {
    switch (header->type) {
        case CONNECT_FOUR_HEADER_TYPE_REGISTRATION_REQUEST: {
            if (server_client->state != SERVER_CLIENT_STATE_PENDING) return true;
            connect_four_register_request_t register_request;
            char* username;
            char* password;
            server_read_register(buf, &register_request, &username, &password);
            if (has_client(server, username)) {
                printf("Client name already present: %s\n", username);
                remove_client(server, server_client->client_fd);
                return false;
            }

            server_client->name = username;
            server_client->password = password;
            server_client->state = SERVER_CLIENT_STATE_REGISTERED;

            server->registered_client_count++;

            if (server->registered_client_count == 2) {
                server_client_t* registered_clients[2];
                get_registered_clients(server, registered_clients, 2);
                //TODO: send client infos
                server->registered_client_count = 0;
                //TODO: close connection in peer info ack
            }

            /*if (server->first_client_fd == NULL) {
                server->first_client_fd = server_client;
            } else if (server->second_client_fd == NULL) {
                server->second_client_fd = server_client;
            }*/

            /*server->registered_client_count++;
            if (server->registered_client_count == 2) {
                //TODO: send peer info from other client to other
                printf("Will deliver messages\n");
            }*/
            /*if (server->first_client_fd != NULL && server->second_client_fd != NULL) {

                remove_client(server, server->first_client_fd->client_fd);
                remove_client(server, server->second_client_fd->client_fd);
                server->first_client_fd = NULL;
                server->second_client_fd = NULL;
            }*/
            //printf("handle registration %d %d %s %s:\n", register_request.name_length, register_request.password_length, username, password);
            //server_send_peer_info(server_client, NULL, 0, 0, 0, "test3");
            break;
        }
    }
    printf("handle message:\n");
    return true;
}

void client_socket_callback(void* args) {
    socket_receive_callback_args_t* socket_receive_callback_args = ((socket_receive_callback_args_t*) args);
    int client_fd = socket_receive_callback_args->client_fd;
    server_t* server = socket_receive_callback_args->server;
    printf("Try to get client for fd:%d\n", client_fd);
    server_client_t* server_client = get_client(server, client_fd);
    if (server_client == 0) {
        printf("No client found with fd: %d\n", client_fd);
        single_linked_list_print(server->server_client_node);
        close(client_fd);
        free(args);
        deregister_fd_callback(client_fd);
        return;
    }
    printf("curr offset: %ld\n", server_client->curr_offset);
    char buf[BUFFER_SIZE];
    memcpy(buf, server_client->message_buffer, BUFFER_SIZE);
    ssize_t len = Recv(client_fd, buf + server_client->curr_offset, BUFFER_SIZE - server_client->curr_offset, 0);
    printf("recv len:%ld\n", len);
    if (len <= 0) {
        remove_client(server, client_fd);
        close(client_fd);
        free(args);
        deregister_fd_callback(client_fd);
        return;
    }
    server_client->curr_offset = len;
    if (server_client->curr_offset < 4) {
        printf("offset < 4\n");
        return;
    }

    connect_four_header_t header;
    server_read_header(buf, &header);
    ssize_t full_message_size = 4 + header.length + calc_padding_of_header_len(header.length);
    if (server_client->curr_offset < full_message_size) {
        printf("server->curr_offset < full_message_size %ld\n", full_message_size);
        return;
    }
    if (!handle_package(&header, server, server_client, buf)) return;
    memmove(buf, buf + full_message_size,
            BUFFER_SIZE - full_message_size);
    server_client->curr_offset = BUFFER_SIZE - full_message_size;

    memcpy(server_client->message_buffer, buf, BUFFER_SIZE);

    printf("new offset: %ld\n", server_client->curr_offset);
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
    if (client_fd <= 0) return;
    add_client(server, client_fd);

    printf("client accepted: %d %s\n", client_fd, inet_ntoa(((struct sockaddr_in*) &client_addr)->sin_addr));

    socket_receive_callback_args_t* receive_args = malloc(sizeof(socket_receive_callback_args_t));
    receive_args->server = server;
    receive_args->client_fd = client_fd;
    register_fd_callback(client_fd, client_socket_callback, receive_args);
}

/*int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Provide server ip address and port");
        return 0;
    }

    init_cblib();

    server_t server;

    init_server(&server);

    char* server_ip_str = argv[1];
    char* server_port_str = argv[2];

    printf("Server IP %s Server Port %s\n", server_ip_str, server_port_str);

    int server_fd = get_address_for_search_for_tcp(server_ip_str, server_port_str);

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    server.server_fd = server_fd;

    socket_accept_callback_args_t* args = malloc(sizeof(socket_accept_callback_args_t));
    memset(args->buf, 0, sizeof(args->buf));
    args->server = &server;

    register_fd_callback(server_fd, server_socket_callback, args);

    printf("Server fd: %d\n", server_fd);

    //int running = 1;

    //struct sockaddr client_addr;
    //socklen_t client_addr_len;
    //memset((void*) &client_addr, 0, sizeof(client_addr));
    //int client_fd;

    //while (running) {
    //    printf("client accept\n");
    //    memset((void*) &client_addr, 0, sizeof(client_addr));
    //    client_addr_len = (socklen_t) sizeof(client_addr);
    //    client_fd = Accept(server_fd, (struct sockaddr*) &client_addr, &client_addr_len);
    //    printf("client accepted: %d %s\n", client_fd, inet_ntoa(((struct sockaddr_in*) &client_addr)->sin_addr));
    //}

    handle_events();

    Close(server_fd);

    return 0;
}
*/
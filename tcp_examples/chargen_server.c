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

#define BUFFER_SIZE 140//(1<<16)
#define BUFFER_RECV_SIZE (1<<16)
#define PORT 2453 //7

typedef struct ClientSocketData {
    int client_fd;
} client_socket_data_t;

void* recv_socket(void* args);

int send_all(int socket, char* buffer, size_t length);

int
main(void) {
    int fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;

    fd = Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    memset((void*) &server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
    server_addr.sin_len = sizeof(struct sockaddr_in);
#endif
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);
    Bind(fd, (const struct sockaddr*) &server_addr, sizeof(server_addr));

    Listen(fd, 5);

    int client_fd;

    int running = 1;

    while (running) {
        printf("client accept\n");
        memset((void*) &client_addr, 0, sizeof(client_addr));
        client_addr_len = (socklen_t) sizeof(client_addr);
        client_fd = Accept(fd, (struct sockaddr*) &client_addr, &client_addr_len);
        printf("client accepted: %d %s\n", client_fd, inet_ntoa(client_addr.sin_addr));
        pthread_t p_thread;
        pthread_attr_t attrs;
        Pthread_attr_init(&attrs);
        //Pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_DETACHED);
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
    char bufRecv[BUFFER_RECV_SIZE];

    int running = 1;

    fd_set read_fd_set;

    fd_set write_fd_set;

    fd_set except_fd_set;

    while (running) {
        FD_ZERO(&read_fd_set);
        FD_ZERO(&write_fd_set);
        FD_ZERO(&except_fd_set);

        FD_SET(client_fd, &read_fd_set);
        FD_SET(client_fd, &write_fd_set);
        FD_SET(client_fd, &except_fd_set);

        Select(client_fd + 1, &read_fd_set, &write_fd_set, &except_fd_set, NULL);

        if (FD_ISSET(client_fd, &except_fd_set)) {
            running = 0;
            break;
        }

        if (FD_ISSET(client_fd, &read_fd_set)) {
            memset((void*) bufRecv, 0, sizeof(bufRecv));
            if (Recv(client_fd, (void*) bufRecv, sizeof(bufRecv), 0) <= 0) {
                Close(client_fd);
                running = 0;
                break;
            }
        }

        if (FD_ISSET(client_fd, &write_fd_set)) {
            memset((void*) buf, 0, sizeof(buf));
            int curr = 32;
            for (int i = 0, length = BUFFER_SIZE - 2; i < length; i++) {
                buf[i] = (char) curr;
                curr++;
                if (curr == 128) {
                    curr = 32;
                }
            }
            buf[BUFFER_SIZE - 2] = '\n';
            buf[BUFFER_SIZE - 1] = '\0';

            if (Send(client_fd, buf, sizeof(buf), 0) < 0) {
                running = 0;
                break;
            }
        }
    }

    pthread_exit(NULL);
}


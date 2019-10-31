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
#include <errno.h>

#define MAX(a, b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define BUFFER_SIZE (1<<16)
#define PORT 2451 //9
#define MAX_CLIENTS 20

int
main(void) {
    int fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;
    ssize_t len;
    char buf[BUFFER_SIZE];

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

    fd_set read_fd_set;

    int maxFd;

    int client_sockets[MAX_CLIENTS];

    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = 0;
    }

    while (running) {
        FD_ZERO(&read_fd_set);

        FD_SET(fd, &read_fd_set);

        maxFd = fd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] != 0) {
                FD_SET(client_sockets[i], &read_fd_set);
                maxFd = MAX(client_sockets[i], maxFd);
            }
        }

        Select(maxFd + 1, &read_fd_set, NULL, NULL, NULL);

        if (FD_ISSET(fd, &read_fd_set)) {
            printf("client accept\n");
            memset((void*) &client_addr, 0, sizeof(client_addr));
            client_addr_len = (socklen_t) sizeof(client_addr);
            client_fd = Accept(fd, (struct sockaddr*) &client_addr, &client_addr_len);
            printf("client accepted: %d %s\n", client_fd, inet_ntoa(client_addr.sin_addr));

            int acceptedClient = 0;

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = client_fd;
                    acceptedClient = 1;
                    break;
                }
            }

            if (!acceptedClient) {
                printf("client not accepted, max clients is reached\n");
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            client_fd = client_sockets[i];
            if (FD_ISSET(client_fd, &read_fd_set)) {
                //printf("client data: %d\n", client_fd);
                len = Recv(client_fd, (void*) buf, sizeof(buf), 0);
                if (len <= 0) {
                    Close(client_fd);
                    printf("client closed: %d\n", client_fd);
                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (client_sockets[i] == client_fd) {
                            client_sockets[i] = 0;
                        }
                    }
                } else {
                    //printf("client read: %d\n", client_fd);
                }
            }
        }
    }
    Close(fd);

    return 0;
}

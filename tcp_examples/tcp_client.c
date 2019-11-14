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
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "Socket.h"

#define BUFFER_SIZE (1<<16)
#define MESSAGE_SIZE (9216)

void fill_hints(struct addrinfo* hints) {
    memset(hints, 0, sizeof(struct addrinfo));
    hints->ai_family = AF_UNSPEC;
    hints->ai_socktype = SOCK_STREAM;
    hints->ai_protocol = IPPROTO_TCP;
    hints->ai_flags = 0;
    hints->ai_canonname = NULL;
    hints->ai_addr = NULL;
    hints->ai_next = NULL;
}

in_port_t get_in_port(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return (((struct sockaddr_in*)sa)->sin_port);
    }

    return (((struct sockaddr_in6*)sa)->sin6_port);
}

int
main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Provide server address\n");
        return 0;
    }
    if (argc < 3) {
        fprintf(stderr, "Provide server address and port\n");
        return 0;
    }

    struct addrinfo hints;

    struct addrinfo* result, * curr;

    int fd;

    fd_set read_fd_set;

    int running = 1;

    char buf[BUFFER_SIZE];

    char host_name_buffer[NI_MAXHOST];

    int len;

    int shutdown = 0;

    fill_hints(&hints);

    Getaddrinfo(argv[1], argv[2], &hints, &result);

    if (result == NULL) {
        printf("No address found for: %s:%s \n", argv[1], argv[2]);
        return 0;
    }

    curr = result;

    do {
        Getnameinfo(curr->ai_addr, curr->ai_addr->sa_len, host_name_buffer, sizeof(host_name_buffer), NULL, 0,
                    NI_NUMERICHOST);
        printf("try connect to: %s %d\n",host_name_buffer, ntohs(get_in_port(curr->ai_addr)));
        fd = Socket(curr->ai_family, curr->ai_socktype, curr->ai_protocol);
        if (Connect(fd, curr->ai_addr, curr->ai_addrlen) != 0) {
            Close(fd);
            fd = -1;
        } else {
            printf("connected\n");
            break;
        }
        printf("------\n");

    } while ((curr = curr->ai_next) != NULL);

    freeaddrinfo(result);

    if (fd == -1) {
        printf("No server to connect\n");
        return 0;
    }

    while (running) {
        FD_ZERO(&read_fd_set);
        // Add stdin to fd_set
        if (!shutdown) {
            FD_SET(STDIN_FILENO, &read_fd_set);
        }
        // Add tcp socket to fd_set
        FD_SET(fd, &read_fd_set);

        Select(fd + 1, &read_fd_set, NULL, NULL, NULL);

        // Data from stdin
        if (FD_ISSET(STDIN_FILENO, &read_fd_set)) {
            memset((void*) buf, 0, sizeof(buf));
            len = Read(STDIN_FILENO, (void*) buf, sizeof(buf));
            if (len == 0) {
                Shutdown(fd, SHUT_WR);
                shutdown = 1;
            } else {
                Send(fd, (const void*) buf, len, 0);
            }
        }

        // Data from server
        if (FD_ISSET(fd, &read_fd_set)) {
            memset((void*) buf, 0, sizeof(buf));
            len = Recv(fd, (void*) buf, sizeof(buf), 0);
            if (len == 0) {
                running = 0;
            } else {
                write(STDOUT_FILENO, buf, len);
            }
        }
    }

    Close(fd);

    return 0;
}

//bsduser498 8518457
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
#include <stdio.h>
#include "Socket.h"

#define BUFFER_SIZE (1<<16)
#define MESSAGE_SIZE (9216)
#define PORT 7788 //7

int
main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Provide server address\n");
        return 0;
    }
    int fd;

    struct sockaddr_in server_addr;

    fd = Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
    server_addr.sin_len = sizeof(struct sockaddr_in);
#endif
    server_addr.sin_port = htons(PORT);
    if ((server_addr.sin_addr.s_addr = (in_addr_t) inet_addr(argv[1])) == INADDR_NONE) {
        fprintf(stderr, "Invalid address\n");
    }

    Connect(fd, (struct sockaddr*) &server_addr, sizeof(server_addr));

    fd_set read_fd_set;

    FD_ZERO(&read_fd_set);

    int running = 1;

    char buf[BUFFER_SIZE];

    int len;

    //int input = 1;

    while (running) {
        // Add stdin to fd_set
        FD_SET(STDIN_FILENO, &read_fd_set);
        // Add tcp socket to fd_set
        FD_SET(fd, &read_fd_set);

        Select(fd + 1, &read_fd_set, NULL, NULL, NULL);

        // Data from stdin
        if (FD_ISSET(STDIN_FILENO, &read_fd_set)) {
            memset((void*) buf, 0, sizeof(buf));
            len = Read(STDIN_FILENO, (void*) buf, sizeof(buf));
            Send(fd, (const void*) buf, sizeof(buf), 0);
            if (/*len == 0*/strlen(buf) == 0) {
                //running = 0;
                //input = 0;
                Shutdown(fd, SHUT_WR);
            } else {
                Send(fd, (const void*) buf, (size_t) len, 0);
            }
        }

        // Data from server
        if (FD_ISSET(fd, &read_fd_set)) {
            len = Recv(fd, (void*) buf, sizeof(buf), 0);
            printf("recv %d\n", len);
            if (len == 0) {
                running = 0;
            } else {
                printf("%.*s\n", (int) len, buf);
            }
            /*if (input == 0) {
                running = 0;
            }*/
        }
    }

    close(fd);

    return 0;
}

//bsduser498 8518457
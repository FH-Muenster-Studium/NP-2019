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
#include <time.h>

#define BUFFER_SIZE (1<<16)
#define PORT 27015 //7

//bsduser222 4222365

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

    char timeBuffer[BUFFER_SIZE];
    time_t raw_time;
    struct tm* time_info;

    Listen(fd, 1);

    int client_fd;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
    while (1) {
        printf("client accept\n");
        memset((void*) &client_addr, 0, sizeof(client_addr));
        client_addr_len = (socklen_t) sizeof(client_addr);
        client_fd = Accept(fd, (struct sockaddr*) &client_addr, &client_addr_len);
        printf("client accepted: %d %s\n", client_fd, inet_ntoa(client_addr.sin_addr));

        do {
            len = Recv(client_fd, (void*) buf, sizeof(buf), 0);
            printf("recv:%ld %s\n", len, buf);
        } while (len > 0);

        if (len < 0) {
            break;
        }

        memset((void*) timeBuffer, 0, sizeof(timeBuffer));

        time(&raw_time);
        time_info = localtime(&raw_time);

        sprintf(timeBuffer, "%d.%d.%d %d:%d:%d", time_info->tm_mday, time_info->tm_mon + 1,
                time_info->tm_year + 1900,
                time_info->tm_hour, time_info->tm_min, time_info->tm_sec);

        printf("Send %zd bytes to %s. %s\n", sizeof(timeBuffer), inet_ntoa(client_addr.sin_addr), timeBuffer);

        Send(client_fd, (const void*) timeBuffer, sizeof(timeBuffer), 0);
    }
#pragma clang diagnostic pop
    Close(fd);

    return 0;
}

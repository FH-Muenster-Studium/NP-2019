#include <stdio.h>
#include "Socket.h"
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>

void fill_hints(struct addrinfo* hints) {
    memset(hints, 0, sizeof(struct addrinfo));
    hints->ai_family = AF_UNSPEC;
    hints->ai_socktype = 0;
    hints->ai_protocol = 0;
    hints->ai_flags = 0;
    hints->ai_canonname = NULL;
    hints->ai_addr = NULL;
    hints->ai_next = NULL;
}

int main(int argc, char** argv) {

    struct addrinfo hints;

    struct addrinfo* result, * curr;

    char host_name_buffer[NI_MAXHOST];

    const void* curr_address;

    if (argc < 2) {
        fprintf(stderr, "Provide address\n");
        return 0;
    }

    printf("IP addresses for: %s\n\n", argv[1]);

    fill_hints(&hints);

    Getaddrinfo(argv[1], NULL, &hints, &result);

    if (result == NULL) {
        printf("No address found \n");
    } else {

        curr = result;

        do {
            if (curr->ai_addr->sa_family == AF_INET6) {
                curr_address = &(((struct sockaddr_in6*) curr->ai_addr)->sin6_addr);
            } else {
                curr_address = &(((struct sockaddr_in*) curr->ai_addr)->sin_addr);
            }

            Inet_ntop(curr->ai_addr->sa_family, curr_address,
                      host_name_buffer, sizeof(host_name_buffer));

            printf("Inet_ntop: %s\n", host_name_buffer);

            getnameinfo(curr->ai_addr, curr->ai_addr->sa_len, host_name_buffer, sizeof(host_name_buffer), NULL, 0,
                        NI_NUMERICHOST);
            printf("getnameinfo: %s\n", host_name_buffer);

            printf("------\n");

        } while ((curr = curr->ai_next) != NULL);
    }

    freeaddrinfo(result);

    return 0;
}
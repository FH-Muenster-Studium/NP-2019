#include <stdio.h>
#include "Socket.h"
#include "cblib.h"
#include "4clib.h"
#include <string.h>
#include <stdlib.h>
#include "connect_four_lib.h"

#define BUFFER_SIZE (1<<16)
#define PORT 2450 //7

typedef struct socket_callback_args {
    client_t* client;
    int fd;
    char buf[BUFFER_SIZE];
} socket_callback_args_t;

int init_socket(int port) {
    int fd;
    struct sockaddr_in6 server_addr;

    fd = Socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);

    memset((void*) &server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
#ifdef HAVE_SIN_LEN
    server_addr.sin6_len = sizeof(struct sockaddr_in6);
#endif
    struct in6_addr any_addr = IN6ADDR_ANY_INIT;
    server_addr.sin6_addr = any_addr;
    server_addr.sin6_port = htons(port);
    int* option = 0;
    setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &option, sizeof(int));
    Bind(fd, (const struct sockaddr*) &server_addr, sizeof(server_addr));
    return fd;
}

int init_socket_other_client(struct sockaddr* other_client_addr, socklen_t other_client_addr_len) {
    int fd = Socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    int* option = 0;
    setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &option, sizeof(int));
    Connect(fd, other_client_addr, other_client_addr_len);
    return fd;
}

void fill_hints(struct addrinfo* hints) {
    memset(hints, 0, sizeof(struct addrinfo));
    hints->ai_family = AF_UNSPEC;
    hints->ai_socktype = SOCK_DGRAM;
    hints->ai_protocol = IPPROTO_UDP;
    hints->ai_flags = 0;
    hints->ai_canonname = NULL;
    hints->ai_addr = NULL;
    hints->ai_next = NULL;
}

void socket_callback(void* args) {
    socket_callback_args_t* socket_callback_args = ((socket_callback_args_t*) args);
    int fd = socket_callback_args->fd;
    char* buf = socket_callback_args->buf;
    client_t* client = socket_callback_args->client;
    ssize_t len = Recv(fd, (void*) (buf), sizeof(buf), 0);
    if (len < sizeof(connect_four_header_t)) {
        printf("len: %ld smaller then header\n", sizeof(connect_four_header_t));
    }
    connect_four_header_t* header = (connect_four_header_t*) buf;
    switch (header->type) {
        case CONNECT_FOUR_HEADER_TYPE_SET_COLUMN:
            if (client->state == CONNECT_FOUR_CLIENT_STATE_WAITING_FOR_A_CLIENT_WITH_A_FIRST_TURN) {
                //TODO:
                //TODO: client_valid_ack is different here, it needs to be 0 not 1
            } else if (client->state == CONNECT_FOUR_CLIENT_STATE_WAITING_FOR_TURN) {
                if (header->length < sizeof(connect_four_set_column_t)) return;
                connect_four_set_column_t* set_column = (connect_four_set_column_t*) buf;

                if (!client_valid_ack(client, set_column->seq)) return;
                client->seq = set_column->seq + 1;

                connect_four_set_column_ack_t set_column_ack;
                set_column_ack.seq = set_column->seq;
                int size = sizeof(set_column_ack);
                memcpy(buf, &set_column_ack, size);
                client->state = CONNECT_FOUR_CLIENT_STATE_WAITING_FOR_A_USER_INPUT;
                client_send_message(client, buf, size);
            }
            break;
        case CONNECT_FOUR_HEADER_TYPE_SET_COLUMN_ACK:
            if (client->state != CONNECT_FOUR_CLIENT_STATE_WAITING_FOR_TURN_ACK) return;
            if (header->length < sizeof(uint32_t)) return;
            connect_four_set_column_ack_t* set_column_ack = (connect_four_set_column_ack_t*) buf;
            if (set_column_ack->seq != client->seq) return;
            client->state = CONNECT_FOUR_CLIENT_STATE_WAITING_FOR_TURN;
            break;
    }
}

in_port_t get_in_port(struct sockaddr* sa) {
    if (sa->sa_family == AF_INET) {
        return (((struct sockaddr_in*) sa)->sin_port);
    }

    return (((struct sockaddr_in6*) sa)->sin6_port);
}

void stdin_callback(void* args) {
    socket_callback_args_t* socket_callback_args = ((socket_callback_args_t*) args);
    int fd = socket_callback_args->fd;
    char* buf = socket_callback_args->buf;
    client_t* client = socket_callback_args->client;
    ssize_t len = Read(STDIN_FILENO, (void*) buf, sizeof(buf));
    if (len > BUFFER_SIZE) return;
    if (client->state != CONNECT_FOUR_CLIENT_STATE_WAITING_FOR_A_USER_INPUT) return;
    buf[len] = '\0';
    int column = atoi(buf);
    printf("column to move: %d\n", column);
    fflush(stdin);
    connect_four_set_column_header_t message;
    message.header.type = CONNECT_FOUR_HEADER_TYPE_SET_COLUMN;
    message.header.length = sizeof(connect_four_set_column_t);
    message.set_column.column = column;
    message.set_column.seq = client->seq;
    int size = sizeof(message);
    memcpy(buf, &message, size);
    client->state = CONNECT_FOUR_CLIENT_STATE_WAITING_FOR_TURN_ACK;
    client_send_message(client, buf, size);
}

void process_state(client_t* client) {
    switch (client->state) {
        case CONNECT_FOUR_CLIENT_STATE_WAITING_FOR_A_CLIENT_WITH_A_FIRST_TURN:
            break;
        case CONNECT_FOUR_CLIENT_STATE_WAITING_FOR_A_USER_INPUT:
            break;
    }
}

int main(int argc, char** argv) {

    init_cblib();

    client_t client;

    if (argc < 2) {
        fprintf(stderr, "Provide server port\n");
        return 0;
    }

    int port = atoi(argv[1]);
    if (port < 0) {
        perror("Port needs to be positive");
        return -1;
    } else if (port == 0) {
        perror("Port needs to be valid integer");
        return -1;
    }

    int fd;
    int other_client_fd;

    if (argc < 3) {
        // Client started only with port
        printf("init own socket\n");
        fd = init_socket(port);
        other_client_fd = 0;
        init_client(&client, NULL, 0, port, other_client_fd);
    } else {
        // Client started with port and ip
        struct addrinfo hints;

        struct addrinfo* result = NULL;

        fill_hints(&hints);

        Getaddrinfo(argv[2], argv[1], &hints, &result);

        if (result == NULL) {
            printf("No address found for: %s:%s \n", argv[2], argv[1]);
            return 0;
        }

        struct addrinfo* curr = NULL;

        char host_name_buffer[NI_MAXHOST];

        curr = result;

        result = NULL;

        do {
            Getnameinfo(curr->ai_addr, curr->ai_addr->sa_len, host_name_buffer, sizeof(host_name_buffer), NULL, 0,
                        NI_NUMERICHOST);
            if (curr->ai_family == AF_INET6) {
                result = curr;
                printf("try connect to: %s %d\n", host_name_buffer, ntohs(get_in_port(curr->ai_addr)));
            }

        } while ((curr = curr->ai_next) != NULL);

        if (result == NULL) {
            printf("No ipv6 address found for: %s:%s \n", argv[2], argv[1]);
            return 0;
        }

        printf("init own socket\n");
        fd = init_socket(port);
        printf("done own socket:%d\n", fd);
        printf("init other client socket\n");
        other_client_fd = init_socket_other_client(result->ai_addr, result->ai_addrlen);
        printf("done init other client socket:%d\n", other_client_fd);
        init_client(&client, result->ai_addr, result->ai_addrlen, port, other_client_fd);
        freeaddrinfo(result);
    }

    socket_callback_args_t* args = malloc(sizeof(socket_callback_args_t));
    memset(args->buf, 0, BUFFER_SIZE);
    args->fd = fd;
    args->client = &client;

    register_fd_callback(fd, socket_callback, args);

    register_stdin_callback(stdin_callback, args);

    process_state(&client);

    handle_events();

    Close(fd);

    return 0;
}

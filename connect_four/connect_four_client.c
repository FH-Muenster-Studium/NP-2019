#include <stdio.h>
#include "Socket.h"
#include "cblib.h"
#include "4clib.h"
#include <string.h>
#include <stdlib.h>
#include "connect_four_lib.h"

#define BUFFER_SIZE (1<<16)

typedef struct socket_callback_args {
    client_t* client;
    int fd;
    char buf[BUFFER_SIZE];
    struct timer* set_column_timer;
    struct timer* heartbeat_timer;
} socket_callback_args_t;

int init_socket(int port) {
    /*int fd;
    struct sockaddr_in6 server_addr;

    fd = Socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);

    memset((void*) &server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
#ifdef HAVE_SIN_LEN
    server_addr.sin6_len = sizeof(struct sockaddr_in6);
#endif
    struct in6_addr any_addr = IN6ADDR_ANY_INIT;
    server_addr.sin6_addr = any_addr;
    if (port) {
        server_addr.sin6_port = htons(port);
    }
    int* option = 0;
    setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &option, sizeof(int));
    Bind(fd, (const struct sockaddr*) &server_addr, sizeof(server_addr));*/
    int fd;
    struct sockaddr_in server_addr;

    fd = Socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    memset((void *) &server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
    server_addr.sin_len = sizeof(struct sockaddr_in);
#endif
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);
    Bind(fd, (const struct sockaddr *) &server_addr, sizeof(server_addr));
    return fd;
}

int get_port_of_socket(int fd) {
    struct sockaddr_in my_addr;
    bzero(&my_addr, sizeof(my_addr));
    int len = sizeof(my_addr);
    getsockname(fd, (struct sockaddr *) &my_addr, &len);
    return ntohs(my_addr.sin_port);
}

in_port_t get_in_port(struct sockaddr* sa) {
    if (sa->sa_family == AF_INET) {
        return ntohs(((struct sockaddr_in*) sa)->sin_port);
    }

    return ntohs((((struct sockaddr_in6*) sa)->sin6_port));
}

void set_in_port(struct sockaddr* sa, in_port_t port) {
    if (sa->sa_family == AF_INET) {
        ((struct sockaddr_in*) sa)->sin_port = htons(port);
    }

    ((struct sockaddr_in6*) sa)->sin6_port = htons(port);
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
    client_t* client = socket_callback_args->client;
    struct sockaddr client_addr;
    socklen_t client_addr_len;
    client_addr_len = (socklen_t) sizeof(client_addr);
    memset((void*) &client_addr, 0, sizeof(client_addr));
    ssize_t len = Recvfrom(fd, socket_callback_args->buf, sizeof(socket_callback_args->buf), 0, &client_addr, &client_addr_len);
    if (len < 0) return;
    printf("recv len:%ld\n", len);
    if (len < sizeof(connect_four_header_t)) {
        printf("len: %ld smaller then header\n", sizeof(connect_four_header_t));
    }
    connect_four_header_t* header = (connect_four_header_t*) socket_callback_args->buf;
    printf("header type:%d\n", header->type);

    /*char *s = NULL;
    switch(client_addr.sa_family) {
        case AF_INET: {
            struct sockaddr_in* addr_in = (struct sockaddr_in*)&client_addr;
            s = malloc(INET_ADDRSTRLEN);
            inet_ntop(AF_INET, &(addr_in->sin_addr), s, INET_ADDRSTRLEN);
            break;
        }
        case AF_INET6: {
            struct sockaddr_in6* addr_in6 = (struct sockaddr_in6*)&client_addr;
            s = malloc(INET6_ADDRSTRLEN);
            inet_ntop(AF_INET6, &(addr_in6->sin6_addr), s, INET6_ADDRSTRLEN);
            break;
        }
        default:
            printf("unknown address family\n");
            break;
    }
    printf("IP address: %s\n", s);
    printf("port: %d\n", get_in_port(&client_addr));
    printf("client_addr_len: %d\n", client_addr_len);
    free(s);*/

    if (client->state == CONNECT_FOUR_CLIENT_STATE_WAITING_FOR_A_CLIENT_WITH_A_FIRST_TURN) {
        client->other_client_addr_len = client_addr_len; //TODO: not required
        client->other_client_addr = &client_addr; //TODO: not required

        Connect(client->socket_fd, &client_addr, client_addr_len);

        client->state = CONNECT_FOUR_CLIENT_STATE_WAITING_FOR_TURN;
    }
    switch (header->type) {
        case CONNECT_FOUR_HEADER_TYPE_SET_COLUMN:
            if (client->state == CONNECT_FOUR_CLIENT_STATE_WAITING_FOR_TURN) {
                if (header->length < sizeof(connect_four_set_column_content_t)) return;
                connect_four_set_column_message_t* set_column_message = (connect_four_set_column_message_t*) socket_callback_args->buf;
                connect_four_set_column_content_t set_column = set_column_message->set_column;

                if (!valid_move(set_column.column)) {
                    client_send_error(client, socket_callback_args->buf, "Cause 1: Invalid column");
                }

                if (!client_valid_ack(client, set_column.seq)) return;
                client->seq = set_column.seq + 1;

                client_send_set_column_ack(client, socket_callback_args->buf, set_column.seq);

                make_move(set_column.column, client_get_player_id(client));

                print_board();

                int winnerPlayerId = winner();
                if (winnerPlayerId != 0) {
                    if (winnerPlayerId == client_get_player_id(client)) {
                        printf("You won\n");
                    } else {
                        printf("You lost\n");
                    }
                    client->state = CONNECT_FOUR_CLIENT_STATE_GAME_END;
                } else {
                    client->state = CONNECT_FOUR_CLIENT_STATE_WAITING_FOR_A_USER_INPUT;
                }
            }
            break;
        case CONNECT_FOUR_HEADER_TYPE_SET_COLUMN_ACK:
            if (client->state != CONNECT_FOUR_CLIENT_STATE_WAITING_FOR_TURN_ACK) return;
            if (header->length != sizeof(connect_four_set_column_ack_content_t)) return;
            connect_four_set_column_ack_message_t* set_column_ack = (connect_four_set_column_ack_message_t*) socket_callback_args->buf;
            if (set_column_ack->set_column_ack.seq != client->seq) return;
            client->state = CONNECT_FOUR_CLIENT_STATE_WAITING_FOR_TURN;
            break;
        case CONNECT_FOUR_HEADER_TYPE_HEARTBEAT: {
            connect_four_heartbeat_message_t* heartbeat_message = (connect_four_heartbeat_message_t*) socket_callback_args->buf;
            if (header->length != (len - sizeof(connect_four_header_t))) return;
            printf("hb len h:%d\n", atoi(heartbeat_message->data));
            //client_send_heartbeat_ack(client, socket_callback_args->buf, heartbeat_message->data, header->length);
            break;
        }
        case CONNECT_FOUR_HEADER_TYPE_HEARTBEAT_ACK: {
            connect_four_heartbeat_ack_message_t* heartbeat_ack_message = (connect_four_heartbeat_ack_message_t*) socket_callback_args->buf;
            if (header->length != (len - sizeof(connect_four_header_t))) return;
            printf("data: %s\n", heartbeat_ack_message->data);
            ++client->heartbeat_count;
            /*if (client->heartbeat_count == (int64_t) heartbeat_ack_message->data) {
                time_t msec = time(NULL) * 1000;
                client->last_heartbeat_received = msec;
                ++client->heartbeat_count;
                printf("heartbeat ack count:%lld\n", client->heartbeat_count);
            }*/
            break;
        }
    }
}

int entered_column_to_data_column(int column) {
    return column - 1;
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
    int column = entered_column_to_data_column(atoi(buf));
    if (!valid_move(column)) {
        printf("Invalid move: %d. Enter a valid column\n", column);
        return;
    }
    printf("column to move: %d\n", column);
    fflush(stdin);

    client->last_column = column;
    client->state = CONNECT_FOUR_CLIENT_STATE_WAITING_FOR_TURN_ACK;

    make_move(column, client_get_player_id(client));

    client_send_set_column(client, buf, column);

    print_board();
}

void send_set_column_timer_callback(void* args) {
    socket_callback_args_t* socket_callback_args = ((socket_callback_args_t*) args);
    int fd = socket_callback_args->fd;
    char* buf = socket_callback_args->buf;
    client_t* client = socket_callback_args->client;
    if (client->state == CONNECT_FOUR_CLIENT_STATE_WAITING_FOR_TURN_ACK) {
        client_send_set_column(client, buf, client->last_column);
    }
    start_timer(socket_callback_args->set_column_timer, 1000);
}

void send_heartbeat_timer_callback(void* args) {
    socket_callback_args_t* socket_callback_args = ((socket_callback_args_t*) args);
    int fd = socket_callback_args->fd;
    char* buf = socket_callback_args->buf;
    client_t* client = socket_callback_args->client;

    if (client->state != CONNECT_FOUR_CLIENT_STATE_WAITING_FOR_A_CLIENT_WITH_A_FIRST_TURN) {
        client_send_heartbeat(client, buf);
    }

    start_timer(socket_callback_args->heartbeat_timer, 1000);
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

    printf("port to use:%d\n", port);

    int fd;

    if (argc < 3) {
        // Client started only with port
        printf("init own socket\n");
        fd = init_socket(port);
        printf("done own socket:%d %d\n", fd, get_port_of_socket(fd));
        init_client(&client, NULL, 0, port, fd);
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
            if (curr->ai_family == AF_INET) {
                result = curr;
                printf("try connect to: %s %d\n", host_name_buffer, get_in_port(curr->ai_addr));
            }

        } while ((curr = curr->ai_next) != NULL);

        if (result == NULL) {
            printf("No ipv6 address found for: %s:%s \n", argv[2], argv[1]);
            return 0;
        }

        fd = init_socket(0);
        printf("listening to port:%d\n", get_port_of_socket(fd));
        Connect(fd, result->ai_addr, result->ai_addrlen);
        printf("connect to ip and port:%d %d\n", fd, get_in_port(result->ai_addr));
        init_client(&client, result->ai_addr, result->ai_addrlen, port, fd);
        freeaddrinfo(result);
    }

    socket_callback_args_t* args = malloc(sizeof(socket_callback_args_t));
    memset(args->buf, 0, sizeof(args->buf));
    args->fd = fd;
    args->client = &client;

    struct timer* set_column_timer = create_timer(send_set_column_timer_callback, args,
                                                  "Checks for resending set column messages");

    struct timer* heartbeat_timer = create_timer(send_heartbeat_timer_callback, args, "Sends heartbeat");

    args->set_column_timer = set_column_timer;
    args->heartbeat_timer = heartbeat_timer;

    register_fd_callback(fd, socket_callback, args);

    register_stdin_callback(stdin_callback, args);

    start_timer(heartbeat_timer, 1000);

    start_timer(set_column_timer, 1000);

    handle_events();

    Close(fd);

    return 0;
}

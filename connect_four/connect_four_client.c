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
    char buf[BUFFER_SIZE];
    struct timer* set_column_timer;
    struct timer* heartbeat_timer;
} socket_callback_args_t;

typedef struct {
    client_t* client;
    int server_fd;
    char buf[BUFFER_SIZE];
} server_callback_args_t;

void end_game_error(client_t* client) {
    client->state = CONNECT_FOUR_CLIENT_STATE_ERROR;
    printf("Spiel wurde aufgrund eines Fehlers beendet.\n");
}

int get_port_of_socket(int fd) {
    struct sockaddr_in my_addr;
    bzero(&my_addr, sizeof(my_addr));
    int len = sizeof(my_addr);
    getsockname(fd, (struct sockaddr*) &my_addr, &len);
    return ntohs(my_addr.sin_port);
}

in_port_t get_in_port(struct sockaddr* sa) {
    if (sa->sa_family == AF_INET) {
        return ntohs(((struct sockaddr_in*) sa)->sin_port);
    }

    return ntohs((((struct sockaddr_in6*) sa)->sin6_port));
}

in_addr_t get_in_addr(struct sockaddr* sa) {
    if (sa->sa_family == AF_INET) {
        return ntohl(((struct sockaddr_in*) sa)->sin_addr.s_addr);
    }

    return 0;
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

bool check_win(client_t* client) {
    int winnerPlayerId = winner();
    if (winnerPlayerId != 0) {
        if (winnerPlayerId == client_get_player_id(client)) {
            printf("You won\n");
        } else {
            printf("You lost\n");
        }
        client->state = CONNECT_FOUR_CLIENT_STATE_GAME_END;
        return true;
    } else {
        return false;
    }
}

void handle_package(connect_four_header_t* header, client_t* client, void* buf) {
    switch (header->type) {
        case CONNECT_FOUR_HEADER_TYPE_PEER_INFO: {
            connect_four_peer_info peer_info;
            char* username;
            client_read_peer_info(buf, &peer_info, &username);
            printf("handle peer info %s\n", username);
            free(username);
            break;
        }
    }
    printf("handle message:\n");
}

void server_callback(void* args) {
    server_callback_args_t* server_callback_args = ((server_callback_args_t*) args);
    client_t* client = server_callback_args->client;
    int server_fd = server_callback_args->server_fd;

    printf("curr offset: %ld\n", client->curr_offset);
    char buf[BUFFER_SIZE];
    memcpy(buf, client->message_buffer, BUFFER_SIZE);
    ssize_t len = Recv(server_fd, buf + client->curr_offset, BUFFER_SIZE - client->curr_offset, 0);
    printf("recv len:%ld\n", len);
    if (len <= 0) {
        close(server_fd);
        free(args);
        deregister_fd_callback(server_fd);
        return;
    }
    client->curr_offset = len;
    if (client->curr_offset < 4) {
        printf("offset < 4\n");
        return;
    }

    connect_four_header_t header;
    server_read_header(buf, &header);
    ssize_t full_message_size = 4 + header.length + calc_padding_of_header_len(header.length);
    if (client->curr_offset < full_message_size) {
        printf("server->curr_offset < full_message_size %ld\n", full_message_size);
        return;
    }
    handle_package(&header, client, buf);
    memmove(buf, buf + full_message_size,
            BUFFER_SIZE - full_message_size);
    client->curr_offset = BUFFER_SIZE - full_message_size;

    memcpy(client->message_buffer, buf, BUFFER_SIZE);

    printf("new offset: %ld\n", client->curr_offset);
}

void socket_callback(void* args) {
    socket_callback_args_t* socket_callback_args = ((socket_callback_args_t*) args);
    client_t* client = socket_callback_args->client;
    struct sockaddr client_addr;
    socklen_t client_addr_len;
    client_addr_len = (socklen_t) sizeof(client_addr);
    memset((void*) &client_addr, 0, sizeof(client_addr));
    char buf[BUFFER_SIZE];
    memset(buf, 0, sizeof(buf));
    ssize_t len = Recvfrom(client->other_client_fd, buf, sizeof(buf), 0, &client_addr, &client_addr_len);
    if (len < 0) return;
    //printf("recv len:%ld\n", len);
    if (len < sizeof(connect_four_header_t)) {
        printf("len: %ld smaller then header\n", sizeof(connect_four_header_t));
        return;
    }
    connect_four_header_t header2;
    read_header(buf, &header2);

    int header_type = header2.type;
    int header_length = header2.length;

    //printf("header type:%d\n", header_type);
    //printf("header length:%d\n", header_length);

    if (client->state == CONNECT_FOUR_CLIENT_STATE_WAITING_FOR_A_CLIENT_WITH_A_FIRST_TURN) {
        client->other_client_addr_len = client_addr_len; //TODO: not required
        client->other_client_addr = &client_addr; //TODO: not required

        Connect(client->other_client_fd, &client_addr, client_addr_len);

        client->state = CONNECT_FOUR_CLIENT_STATE_WAITING_FOR_TURN;
    }
    switch (header_type) {
        case CONNECT_FOUR_HEADER_TYPE_SET_COLUMN:
            if (client->state == CONNECT_FOUR_CLIENT_STATE_WAITING_FOR_TURN) {
                if (header_length < (sizeof(uint32_t) + sizeof(uint16_t))) return;
                connect_four_set_column_message_t set_column;
                read_set_column(buf, &set_column);

                if (!valid_move(set_column.column)) {
                    client_send_error(client, buf, "Cause 1: Invalid column");
                    end_game_error(client);
                    return;
                }

                printf("ack c %d\n", client->seq);
                printf("ack msg %d\n", set_column.seq);

                if (!client_valid_ack(client, set_column.seq)) return;
                client->seq = set_column.seq + 1;

                client_send_set_column_ack(client, buf, set_column.seq);

                make_move(set_column.column, client_get_other_player_id(client));

                print_board();

                if (check_win(client) == false) {
                    client->state = CONNECT_FOUR_CLIENT_STATE_WAITING_FOR_A_USER_INPUT;
                }
            }
            break;
        case CONNECT_FOUR_HEADER_TYPE_SET_COLUMN_ACK:
            if (client->state != CONNECT_FOUR_CLIENT_STATE_WAITING_FOR_TURN_ACK) return;
            if (header_length != sizeof(connect_four_set_column_ack_content_t)) return;
            connect_four_set_column_ack_message_t set_column_ack;
            read_set_column_ack(buf, &set_column_ack);
            if (set_column_ack.seq != client->seq) return;
            client->seq = set_column_ack.seq + 1;

            make_move(client->last_column, client_get_player_id(client));
            print_board();
            if (check_win(client) == false) {
                client->state = CONNECT_FOUR_CLIENT_STATE_WAITING_FOR_TURN;
            }
            break;
        case CONNECT_FOUR_HEADER_TYPE_HEARTBEAT: {
            connect_four_heartbeat_message_t heartbeat_message;
            read_heartbeat(buf, header_length, &heartbeat_message);
            if (header_length == (len - sizeof(connect_four_header_t))) {
                client_send_heartbeat_ack(client, buf, heartbeat_message.data, header_length);
            }
            free(heartbeat_message.data);
            break;
        }
        case CONNECT_FOUR_HEADER_TYPE_HEARTBEAT_ACK: {
            connect_four_heartbeat_ack_message_t heartbeat_ack_message2;
            read_heartbeat_ack(buf, header_length, &heartbeat_ack_message2);
            printf("hb ack %s\n", heartbeat_ack_message2.data);
            if (header_length == (len - sizeof(connect_four_header_t)) &&
                client->heartbeat_count == atoi(heartbeat_ack_message2.data)) {
                time_t msec = time(NULL) * 1000;
                client->last_heartbeat_received = msec;
                client->heartbeat_count = client->heartbeat_count + 1;
                printf("heartbeat ack count:%lld\n", client->heartbeat_count);
            }
            free(heartbeat_ack_message2.data);
            break;
        }
        case CONNECT_FOUR_HEADER_TYPE_ERROR: {
            connect_four_error_message_t error_message;
            char* error;
            read_error(buf, &error_message, &error);
            printf("Error: %s\n", error);
            client->state = CONNECT_FOUR_CLIENT_STATE_ERROR;
            free(error);
            break;
        }
        default:
            printf("Unsupported header type: %d\n", header_type);
            break;
    }
}

int entered_column_to_data_column(int column) {
    return column - 1;
}

void stdin_callback(void* args) {
    socket_callback_args_t* socket_callback_args = ((socket_callback_args_t*) args);
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

    client_send_set_column(client, buf, column);
}

void send_set_column_timer_callback(void* args) {
    socket_callback_args_t* socket_callback_args = ((socket_callback_args_t*) args);
    char* buf = socket_callback_args->buf;
    client_t* client = socket_callback_args->client;
    if (client->state == CONNECT_FOUR_CLIENT_STATE_WAITING_FOR_TURN_ACK) {
        client_send_set_column(client, buf, client->last_column);
    }
    start_timer(socket_callback_args->set_column_timer, 1000);
}

void send_heartbeat_timer_callback(void* args) {
    socket_callback_args_t* socket_callback_args = ((socket_callback_args_t*) args);
    char* buf = socket_callback_args->buf;
    client_t* client = socket_callback_args->client;

    if (client->state != CONNECT_FOUR_CLIENT_STATE_WAITING_FOR_A_CLIENT_WITH_A_FIRST_TURN &&
        client->state != CONNECT_FOUR_CLIENT_STATE_ERROR) {
        client_send_heartbeat(client, buf);
        if (time(NULL) * 1000 - client->last_heartbeat_received > 30000) {
            printf("No signal in last 30 seconds.\n");
        }
    }

    start_timer(socket_callback_args->heartbeat_timer, 1000);
}

bool get_address_for_search(char* ip_str, char* port_str, uint32_t* ip, uint16_t* port) {

    struct addrinfo hints;

    struct addrinfo* result = NULL;

    struct addrinfo* curr = NULL;

    char host_name_buffer[NI_MAXHOST];

    fill_hints(&hints);

    result = NULL;

    Getaddrinfo(ip_str, port_str, &hints, &result);

    curr = result;

    if (result == NULL) {
        printf("No address found for: %s:%s \n", ip_str, port_str);
        return 0;
    }

    do {
#ifdef HAVE_SIN_LEN
        Getnameinfo(curr->ai_addr, ((struct sockaddr_in*) curr->ai_addr)->sin_len, host_name_buffer,
                    sizeof(host_name_buffer), NULL, 0,
                    NI_NUMERICHOST);
#else
        Getnameinfo(curr->ai_addr, sizeof(struct sockaddr_in), host_name_buffer,
                    sizeof(host_name_buffer), NULL, 0,
                    NI_NUMERICHOST);
#endif
        if (curr->ai_family == AF_INET) {
            result = curr;
            *ip = get_in_addr(curr->ai_addr);
            *port = get_in_port(curr->ai_addr);
            printf("try to use address: %s %d %d\n", host_name_buffer, get_in_port(curr->ai_addr),
                   ((struct sockaddr_in*) curr->ai_addr)->sin_port);
        }

    } while ((curr = curr->ai_next) != NULL);

    int fd = Socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    Bind(fd, result->ai_addr, result->ai_addrlen);
    freeaddrinfo(result);
    return fd;
}

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
            printf("try connect to: %s %d\n", host_name_buffer, ntohs(((struct sockaddr_in*) curr->ai_addr)->sin_port));
            fd = Socket(curr->ai_family, curr->ai_socktype, curr->ai_protocol);
            if (fd < 0) continue;
            if (Connect(fd, curr->ai_addr, curr->ai_addrlen) != 0) {
                Close(fd);
                fd = -1;
                printf("not connected\n");
            } else {
                printf("connected\n");
                break;
            }
            printf("------\n");
        }

    } while ((curr = curr->ai_next) != NULL);

    freeaddrinfo(result);

    if (fd == -1) {
        printf("No server to connect\n");
        return 0;
    }

    return fd;
}

//Server IP, Server Port, Eine IP, Eigenen Port, Name, Passwort
int main(int argc, char** argv) {

    init_cblib();

    client_t client;

    if (argc < 7) {
        fprintf(stderr, "Server IP, Server Port, Eine IP, Eigenen Port, Name, Passwort\n");
        return 0;
    }

    char* server_ip_str = argv[1];
    char* server_port_str = argv[2];

    printf("Server IP %s Server Port %s\n", server_ip_str, server_port_str);

    // Server IP, Server Port
    int server_fd = get_address_for_search_for_tcp(server_ip_str, server_port_str);
    if (server_fd < 1) {
        return 0;
    }

    server_callback_args_t* server_args = malloc(sizeof(socket_callback_args_t));
    memset(server_args->buf, 0, sizeof(server_args->buf));
    server_args->client = &client;
    server_args->server_fd = server_fd;

    register_fd_callback(server_fd, server_callback, server_args);


    printf("Server fd: %d\n", server_fd);

    char* client_ip_str = argv[3];
    char* client_port_str = argv[4];

    printf("Client IP %s Client Port %s\n", client_ip_str, client_port_str);

    uint32_t client_ip = 0;
    uint16_t client_port = 0;

    // Client IP, Client Port
    int client_fd = get_address_for_search(client_ip_str, client_port_str, &client_ip, &client_port);
    if (client_fd < 1) {
        return 0;
    }

    printf("Client fd: %d\n", client_fd);

    char* name = argv[5];
    char* password = argv[6];

    init_client(&client, 0, 0, 0, client_fd, server_fd);

    socket_callback_args_t* args = malloc(sizeof(socket_callback_args_t));
    memset(args->buf, 0, sizeof(args->buf));
    args->client = &client;


    int send_res = client_send_register(&client, args->buf, client_ip, client_port, name, password);

    printf("send result: %d\n", send_res);

    handle_events();

    /*printf("listening to port:%d\n", get_port_of_socket(own_client_fd));
    Connect(fd, result->ai_addr, result->ai_addrlen);
    printf("connect to ip and port:%d %d\n", fd, get_in_port(result->ai_addr));
    init_client(&client, result->ai_addr, result->ai_addrlen, 0, fd);
    freeaddrinfo(result);

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

    Close(fd);*/

    return 0;
}

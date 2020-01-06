#include "gameserver.h"

int parse_args(int argc, char** argv, game_server_info* server_info) {
    struct addrinfo hints;
    if (argc != 3) {
        printf("Please provide 3 parameters, got %d. Provide server_port password\n", argc);
        return -1;
    }
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    Getaddrinfo(NULL, argv[1], &hints, &server_info->server);
    /* Note: These "strings" are not zero-terminated anymore, so we handle it as char array rather than strings */
    server_info->credentials.pass_len = strlen(argv[2]);
    server_info->credentials.pass = malloc(server_info->credentials.pass_len);
    memcpy(server_info->credentials.pass, argv[2], server_info->credentials.pass_len);
    return 0;
}

void cleanup_client(client_info* a_client) {
    printf("Cleanup client, fd #%d\n", a_client->client_fd);
    deregister_fd_callback(a_client->client_fd);
    Shutdown(a_client->client_fd, SHUT_WR);
    while (Recv(a_client->client_fd, (void*) a_client->message.buf, (size_t) BUFFER_SIZE, 0) != 0) {
    }
    Close(a_client->client_fd);
    client_list_remove(&a_client->server_info->client_queue, a_client);
    free(a_client->credentials.name);
    free(a_client->credentials.pass);
    free(a_client);
}

void cleanup(game_server_info* server_info, int exit_code, char* last_message) {
    client_info* a_client, * tmp_client;
    if (last_message != NULL)
        printf("%s", last_message);
    for (a_client = server_info->client_queue.next; a_client != NULL;) {
        tmp_client = a_client;
        a_client = a_client->next;
        cleanup_client(tmp_client);
    }
    Close(server_info->server_fd);
    freeaddrinfo(server_info->server);
    exit(exit_code);
}

void send_primitive_message(client_info* a_client, uint16_t type) {
    struct msg_header_t a_header_msg;
    a_header_msg.length = htons(0);
    a_header_msg.type = htons(type);
    Send(a_client->client_fd, &a_header_msg, sizeof(a_header_msg), 0);
}

void send_peer_info(client_info* a_client, client_info* a_clients_peer, uint16_t start_flag) {
    ssize_t name_padding, s;
    char* a_msg, * a_name;
    msg_peer_info* a_peer_info;

    name_padding = (4 - a_clients_peer->credentials.name_len % 4) % 4;
    s = sizeof(msg_peer_info) + a_clients_peer->credentials.name_len + name_padding;
    a_msg = calloc(1, s);

    a_peer_info = (msg_peer_info*) a_msg;
    a_peer_info->type = htons(MSG_PEER_INFO);
    a_peer_info->length = htons(s - name_padding - sizeof(struct msg_header_t));
    // Note: net_addr and net_port are still in network byte order
    a_peer_info->net_addr = a_clients_peer->net_addr;
    a_peer_info->net_port = a_clients_peer->net_port;
    a_peer_info->start_flag = htons(start_flag);

    a_name = a_msg + sizeof(msg_peer_info);
    memcpy(a_name, a_clients_peer->credentials.name, a_clients_peer->credentials.name_len);

    Send(a_client->client_fd, a_msg, s, 0);
    free(a_msg);
    //TODO: should we wait for ack forever?
    a_client->client_state = CLIENT_STATE_WAIT_PEER_INFO_ACK;
}

void find_game_peers(game_server_info* server_info) {
    client_info* a_client, * b_client, * list;
    a_client = NULL;
    b_client = NULL;
    list = server_info->client_queue.next;
    for (; list != NULL; list = list->next) {
        if (list->client_state == CLIENT_STATE_WAIT_FOR_PEER) {
            if (a_client == NULL) {
                a_client = list;
            } else {
                b_client = list;
                send_peer_info(a_client, b_client, 0);
                send_peer_info(b_client, a_client, 1);
                return;
            }
        }
    }
}

void handle_peer_reg(client_info* a_client, struct msg_header_t* a_msg) {
    ssize_t name_padd, pass_len;
    char* a_name, * a_pass;
    msg_reg* a_reg_msg;

    //printf("Got a peer_reg message\n");
    a_reg_msg = (msg_reg*) a_msg;
    a_reg_msg->name_len = ntohs(a_reg_msg->name_len);
    name_padd = (4 - a_reg_msg->name_len % 4) % 4;

    a_name = (char*) a_msg + sizeof(msg_reg);
    a_pass = a_name + a_reg_msg->name_len + name_padd;
    pass_len = a_reg_msg->length - ((sizeof(msg_reg) - sizeof(struct msg_header_t)) + a_reg_msg->name_len + name_padd);

    a_client->credentials.name_len = a_reg_msg->name_len;
    a_client->credentials.name = malloc(a_client->credentials.name_len);
    a_client->credentials.pass_len = pass_len;
    a_client->credentials.pass = malloc(a_client->credentials.pass_len);
    memcpy(a_client->credentials.name, a_name, a_client->credentials.name_len);
    memcpy(a_client->credentials.pass, a_pass, a_client->credentials.pass_len);

    //Save it in network byte order to prevent converting again
    a_client->net_addr = a_reg_msg->net_addr;
    a_client->net_port = a_reg_msg->net_port;
    // printf("s_len: %d, c_len: %d\n", a_client->server_info->credentials.pass_len, a_client->credentials.pass_len);
    // printf("s: %.*s, p: %.*s\n", a_client->server_info->credentials.pass_len, a_client->server_info->credentials.pass, a_client->credentials.pass_len, a_client->credentials.pass);
    if (a_client->server_info->credentials.pass_len == a_client->credentials.pass_len &&
        memcmp(a_client->server_info->credentials.pass, a_client->credentials.pass, a_client->credentials.pass_len) ==
        0) {
        printf("password is valid.\n");
        send_primitive_message(a_client, MSG_REG_ACK);
        a_client->client_state = CLIENT_STATE_WAIT_FOR_PEER;
        find_game_peers(a_client->server_info);
    } else {
        printf("password is invalid.\n");
        send_primitive_message(a_client, MSG_REG_NACK);
    }
}

void handle_client_message(client_info* a_client, struct msg_header_t* a_msg) {
    switch (a_msg->type) {
        case MSG_REG:
            handle_peer_reg(a_client, a_msg);
            break;
        case MSG_PEER_INFO_ACK:
            printf("Got MSG_PEER_INFO_ACK\n");
            cleanup_client(a_client);
            break;
        case MSG_UNKNOWN_MSG_TYPE:
            printf("Sended unknown message.\n");
            cleanup_client(a_client);
            break;
        default:
            printf("Received unknown message, msg_type-ID: %d\n", a_msg->type);
            send_primitive_message(a_client, MSG_UNKNOWN_MSG_TYPE);
            break;
    }
}

ssize_t packet_sequenzer(client_info* a_client, ssize_t stream_len) {
    uint16_t msg_len;
    if (stream_len < (ssize_t) sizeof(struct msg_header_t))
        return stream_len;

    struct msg_header_t* a_msg = (struct msg_header_t*) a_client->message.buf;
    a_msg->length = ntohs(a_msg->length);
    a_msg->type = ntohs(a_msg->type);

    msg_len = sizeof(struct msg_header_t) + a_msg->length + (4 - a_msg->length % 4) % 4;
    if (stream_len < msg_len)
        return stream_len;
    if (msg_len < a_client->server_info->msg_min_size[a_msg->type]) {
        printf("Expected length %d but got %d\n", a_client->server_info->msg_min_size[a_msg->type], msg_len);
        cleanup_client(a_client);
        return -1;
    }
    handle_client_message(a_client, a_msg);
    memmove(a_client->message.buf, a_client->message.buf + msg_len, stream_len - msg_len);

    return packet_sequenzer(a_client, stream_len - msg_len);
}

void handle_client_rec(client_info* a_client) {
    ssize_t n;
    ssize_t stream_len;
    if ((n = Recv(a_client->client_fd, (void*) (a_client->message.buf + a_client->message.buf_offset),
                  BUFFER_SIZE - a_client->message.buf_offset, 0)) == 0) {
        cleanup_client(a_client);
        return;
    }
    //printf("Received %zd bytes, buf offset: %zd \n", n, a_client->message.buf_offset);
    if ((stream_len = packet_sequenzer(a_client, n + a_client->message.buf_offset)) < 0)
        return;
    a_client->message.buf_offset = stream_len;
}

client_info* initialize_client(game_server_info* server_info, int a_client_fd) {
    printf("initialize_client\n");
    client_info* a_client = NULL;
    a_client = calloc(1, sizeof(client_info));
    a_client->server_info = server_info;
    a_client->client_fd = a_client_fd;
    a_client->client_state = CLIENT_STATE_INIT;
    register_fd_callback(a_client->client_fd, (void (*)(void*)) &handle_client_rec, a_client);
    return a_client;
}

void handle_accept(game_server_info* server_info) {
    int a_client_fd;
    a_client_fd = Accept(server_info->server_fd, NULL, 0);
    printf("Accepted client, fd #%d\n", a_client_fd);
    client_list_add(&server_info->client_queue, initialize_client(server_info, a_client_fd));
}

int init_gs(game_server_info* server_info) {
    int option;
    server_info->server_fd = Socket(server_info->server->ai_family, server_info->server->ai_socktype, 0);
    option = 1;
    Setsockopt(server_info->server_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    Bind(server_info->server_fd, server_info->server->ai_addr, server_info->server->ai_addrlen);
    Listen(server_info->server_fd, BACKLOG);
    init_cblib();
    memset(&server_info->client_queue, 0, sizeof(server_info->client_queue));

    register_fd_callback(server_info->server_fd, (void (*)(void*)) &handle_accept, server_info);

    handle_events();
    return 0;
}

int main(int argc, char** argv) {
    int val;
    game_server_info server_info;
    init_min_msg_size(server_info.msg_min_size);
    if (parse_args(argc, argv, &server_info) != 0)
        return -1;
    val = init_gs(&server_info);
    cleanup(&server_info, val, NULL);
    return 0;
}

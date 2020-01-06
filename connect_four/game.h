#include "Socket.h"
#include "cblib.h"
#include <stdbool.h>
#include <sys/param.h>
#include <sys/types.h>

#ifndef GAME_H_
#define GAME_H_

#define BUFFER_SIZE (1 << 16)
#define BACKLOG 1000
/*
 * Message TYPES
 */
enum {
    MSG_REG = 0,
    MSG_REG_ACK,
    MSG_REG_NACK,
    MSG_PEER_INFO,
    MSG_MOVE,
    MSG_MOVE_ACK,
    MSG_HEARTBEAT,
    MSG_HEARTBEAT_ACK,
    MSG_UNKNOWN_MSG_TYPE,
    MSG_PEER_INFO_ACK
};

#define MSG_MAX_TYPE_ID 9

#define LOCAL_START 1
#define REMOTE_START 0

#define LOCAL_PLAYER -1
#define REMOTE_PLAYER 1

enum {
    STATE_INIT,
    STATE_WAIT_FOREIGN_MOVE,
    STATE_WAIT_ACK,
    STATE_WAIT_USER_INPUT,
    STATE_FINALIZE
};

enum {
    CLIENT_STATE_INIT,
    CLIENT_STATE_WAIT_FOR_PEER,
    CLIENT_STATE_WAIT_PEER_INFO_ACK
};

#define FIRST_SEQ_NUMBER 1
#define TIMEOUT_ACK_MIN_MS 200
#define TIMEOUT_ACK_MAX_MS 20000
#define TIMEOUT_HB_MIN_MS 8000
#define TIMEOUT_HB_MAX_MS 120000
#define DEBUG

/*
 * Message STRUCTURES
 */

struct msg_header_t {
    uint16_t type;
    uint16_t length;
} __attribute__((packed));

struct msg_reg_t {
    uint16_t type;
    uint16_t length;
    uint32_t net_addr;
    uint16_t net_port;
    uint16_t name_len;
    // char* name;
    // char* password;
} __attribute__((packed));
typedef struct msg_reg_t msg_reg;

struct msg_peer_info_t {
    uint16_t type;
    uint16_t length;
    uint32_t net_addr;
    uint16_t net_port;
    uint16_t start_flag;
    //char* name;
} __attribute__((packed));
typedef struct msg_peer_info_t msg_peer_info;

struct msg_move_t {
    uint16_t type;
    uint16_t length;
    uint32_t seg_nr;
    uint32_t column;
} __attribute__((packed));
typedef struct msg_move_t msg_move;
struct msg_move_ack_t {
    uint16_t type;
    uint16_t length;
    uint32_t seg_nr;
} __attribute__((packed));
typedef struct msg_move_ack_t msg_move_ack;

struct game_state_t {
    uint16_t current_mode;
    uint16_t current_state;
    uint32_t my_latest_move;
    uint32_t sec_my;
    uint32_t sec_rem_next_excepted;
};

typedef struct game_state_t game_state;
struct game_timer_t {
    struct timer* hb_timer;
    struct timer* conn_timeout;
    struct timer* ack_timer;
    unsigned int ack_timeout_ms;
    unsigned int hb_timeout_ms;
};
typedef struct game_timer_t game_timers;

struct message_buffer_t {
    ssize_t buf_offset;
    char buf[BUFFER_SIZE];
};
struct credentials_t {
    char* name;
    uint32_t name_len;
    char* pass;
    uint32_t pass_len;
};

struct game_peers_info_t {
    struct addrinfo* local;
    struct addrinfo* remote;
    int start_flag;
    int net_fd;
    struct game_state_t state;
    struct game_timer_t timer;
    struct message_buffer_t message;
    struct credentials_t credentials;
    uint16_t msg_min_size[MSG_MAX_TYPE_ID];
};
typedef struct game_peers_info_t game_peers_info;

struct client_info_t {
    struct game_server_info_t* server_info;
    struct client_info_t* next;
    int client_fd;
    int client_state;
    struct credentials_t credentials;
    uint32_t net_addr;
    uint16_t net_port;
    struct message_buffer_t message;
};
typedef struct client_info_t client_info;
struct game_server_info_t {
    struct addrinfo* server;
    int server_fd;
    struct client_info_t client_queue;
    struct credentials_t credentials;
    uint16_t msg_min_size[MSG_MAX_TYPE_ID];
};
typedef struct game_server_info_t game_server_info;

#endif

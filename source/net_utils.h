#ifndef NET_UTILS_H
#define NET_UTILS_H

#include "pp4m/pp4m_net.h"

#define MAX_CLIENTS     2
#define MAX_LOBBY       MAX_CLIENTS/2

typedef enum {
    LB_AVAIL = 0,
    LB_FULL = 1,
    LB_BUSY = 2,
    LB_ERROR = 3,
    LB_BLOCKED = 4
} LOBBY_STATUS;

// this struct will be used to join two incoming connections to compete the game
#define cli_t int

typedef struct {
    cli_t *cli_a, *cli_b;
} pair_cli_t;

typedef struct {
    pair_cli_t pair;
    LOBBY_STATUS status;
} net_lobby;

// client command codes which will be sent to the server/lobby
typedef enum {
    CL_STATE_START = 100,               // STATE START
        CL_STATE_IDLE,                  // post idle ...................................... || "1xx", code
        CL_STATE_CONFIRM,               // post confirm ................................... || "1xx", code
        CL_STATE_DENY,                  // post deny ...................................... || "1xx", code
    CL_STATE_END,                       // STATE END

    CL_REQ_START = 200,                 // REQ START
        CL_REQ_TOTAL_FILE,              // total files to download ........................ || "2xx 21", code
        CL_REQ_TOTAL_USER,              // total users at the moment connected ............ || "2xx 15 128", code, clients_active, max_clients
        CL_REQ_TOTAL_LOBBY,             // total lobbies (AVAIL/BUSY/etc..) ............... || "2xx 2 64", code, lobbies_active, max_lobbies
        CL_REQ_ASSIGN_LOBBY,            // requesting to be assigned to a new lobby ....... || "2xx", code
    CL_REQ_END,                         // REQ END

    CL_REQ_LOBBY_START,                 // REQ LOBBY_START
        CL_REQ_LOBBY_NICKNAME,          // client asking nickname of the other client ..... || "2xx", code
    CL_REQ_LOBBY_END,                   // REQ LOBBY_END

    CL_POST_START = 300,                // POST START
    CL_POST_END,                        // POST END

    CL_POST_LOBBY_START,                // POST LOBBY_START
        CL_POST_LOBBY_LEAVE,            // client decided to exit from lobby .............. || "3xx", code
        CL_POST_LOBBY_MOVE,             // client prompting to the lobby the new move ..... || "3xx 24 43 -1", code, old, new, prom
        CL_POST_LOBBY_MESG,             // client prompting to the lobby the incoming mesg  || "3xx {how long have you been playing chess?}", code, mesg*
    CL_POST_LOBBY_END,                  // POST LOBBY_END

} CLIENT_CMD;

// server command codes which will be sent to the client/lobby
typedef enum {
    SV_STATE_START = 100,               // STATE START
        SV_STATE_IDLE,                  // post idle ...................................... || "1xx", code
        SV_STATE_CONFIRM,               // post confirm ................................... || "1xx", code
        SV_STATE_DENY,                  // post deny ...................................... || "1xx", code
    SV_STATE_END,                       // STATE END

    SV_REQ_START = 200,                 // REQ START
        SV_REQ_NICKNAME,                // server asking for nickname of client ........... || "2xx", code
    SV_REQ_END,                         // REQ END

    SV_REQ_LOBBY_START,                 // REQ LOBBY_START
        SV_REQ_LOBBY_SYNC,              // server asking confirmation on sync ............. || "2xx", code
    SV_REQ_LOBBY_END,                   // REQ LOBBY_END

    SV_POST_START = 300,                // POST START
    SV_POST_END,                        // POST END

    SV_POST_LOBBY_START,                // POST LOBBY_START
        SV_POST_LOBBY_INIT,             // lobby initialized and ready .................... || "3xx w 8/8/3k4/8/8/3K4/8/8 - - 0 0", code, player_view, fen_board, fen_castle, fen_passant, fen_halfm, fen_fullm
        SV_POST_LOBBY_PARTNER_LEFT,     // the partner left the game ...................... || "3xx", code
        SV_POST_LOBBY_TIME,             // prompting new time for timers to sync .......... || "3xx 2189432", code, time_in_clock
    SV_POST_LOBBY_END                   // POST LOBBY_END

} SERVER_CMD;

int handle_client(cli_t *client, char *buffer);
int retrieve_code(char *mesg);
int verify_mesg_recv(char *mesg);
int generate_val(int max);
void init_client_list(cli_t *client_list, int max);
void init_lobby_list(net_lobby *lobby_list, int max);

cli_t client_accept(int master_socket, struct sockaddr_in *addr);
void client_disconnect(cli_t *client);

#endif

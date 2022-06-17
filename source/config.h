#ifndef CONFIG_H
#define CONFIG_H

#define PORT 62443
#define MAX_CLIENTS 2
#define MAX_LOCAL MAX_CLIENTS/2

// this struct will be used to join two incoming connections to compete the game
typedef struct {
    int socket;
    int status; // used for communication client <-> server (keeps track of what happends via commands)
} cli_t;

typedef struct {
    cli_t *cli_a, *cli_b;
} pair_cli_t;

typedef struct {
    pair_sockfd_t pair;
    LOBBY_STATUS status;
} net_lobby;

typedef enum {
    LB_AVAIL = 0,
    LB_FULL = 1,
    LB_BUSY = 2,
    LB_ERROR = 3,
    LB_BLOCKED = 4
} LOBBY_STATUS;

// client command codes which will be sent to the server/lobby
typedef enum {
    CL_START = 100,                     // _START
    CL_IDLE,                            // post idle                                        || "1xx", code
    CL_CONFIRM,                         // post confirm                                     || "1xx", code
    CL_DENY,                            // post deny                                        || "1xx", code
    CL_END,                             // _END

    CL_REQ_START = 200,                 // _START
    CL_REQ_TOTAL_FILES,                 // total files to download                          || "2xx 21", code
    CL_REQ_TOTAL_USERS,                 // total users at the moment connected              || "2xx 15/128", code
    CL_REQ_TOTAL_LOBBY,                 // total lobbies (AVAIL/BUSY/etc..)                 || "2xx 2/64", code
    CL_REQ_ASSIGN_LOBBY,                // requesting to be assigned to a new lobby         || "2xx", code
    CL_REQ_END,                         // _END

    CL_POST_START = 300,                // _START
    CL_POST_LOBBY_LEAVE,                // client decided to exit from lobby                || "3xx", code
    CL_POST_LOBBY_MOVE,                 // client prompting to the lobby the new move       || "3xx 24 43 -1", code, old, new, prom
    CL_POST_LOBBY_MESG,                 // client prompting to the lobby the incoming mesg  || "3xx {how long have you been playing chess?}", code, mesg*
    CL_POST_END,                        // _END

} CLIENT_CMD;

// server command codes which will be sent to the client/lobby
typedef enum {
    SV_START = 100,                     // _START
    SV_CONFIRM,                         // post confirm
    SV_DENY,                            // post deny
    SV_END,                             // _END

    SV_REQ_START = 200,                 // _START
    SV_REQ_LOBBY_SYNC,                  // server asking confirmation on sync
    SV_REQ_END,                         // _END

    SV_POST_START = 300,                // _START
    SV_POST_LOBBY_PARTNER_LEFT,         // the partner left the game
    SV_POST_LOBBY_TIME,                 // prompting new time for timers to sync
    SV_POST_END                         // _END

} SERVER_CMD;

#endif

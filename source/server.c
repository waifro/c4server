#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "server.h"
#include "net_utils.h"

int svcode_status_STATE(int status) {
    return (status > SV_STATE_START && status < SV_STATE_END ? 0 : -1);
}

int svcode_status_REQ(int status) {
    return (status > SV_REQ_START && status < SV_REQ_END ? 0 : -1);
}

int svcode_status_LOBBY_REQ(int status) {
    return (status > SV_REQ_LOBBY_START && status < SV_REQ_LOBBY_END ? 0 : -1);
}

int svcode_status_POST(int status) {
    return (status > SV_POST_START && status < SV_POST_END ? 0 : -1);
}

int svcode_status_LOBBY_POST(int status) {
    return (status > SV_POST_LOBBY_START && status < SV_POST_LOBBY_END ? 0 : -1);
}

int svcode_REQ_redirect(int code, cli_t *client, int room, char *buffer) {
    int result = -1;

    switch(status) {
        case SV_REQ_NICKNAME:
            break;

        default:
            break;
    }

    return result;
}

int svcode_POST_redirect(int code, cli_t *client, int room, char *buffer) {
    int result = -1;

    switch(status) {
        case SV_POST_LOBBY_START:
            result = lobby_random_start(room, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 0");
            break;

        default:
            break;
    }

    return result;
}

int svcode_redirect(int code, cli_t *client, int room, char *buffer) {
    int result = 0;

    if (svcode_status_STATE(code) == 0) result = 0; // im not sure what to do with this and cli_t.status
    else if (svcode_status_REQ(code) == 0) result = svcode_REQ_redirect(code, client, room, buffer);
    else if (svcode_status_POST(code) == 0) result = svcode_POST_redirect(code, client, room, buffer);

    return result;
}

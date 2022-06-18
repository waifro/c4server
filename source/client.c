#include "client.h"
#include "net_utils.h"

int clcode_status_STATE(int status) {
    return (status > CL_STATE_START && status < CL_STATE_END ? 0 : -1);
}

int clcode_status_REQ(int status) {
    return (status > CL_REQ_START && status < CL_REQ_END ? 0 : -1);
}

int clcode_status_LOBBY_REQ(int status) {
    return (status > CL_REQ_LOBBY_START && status < CL_REQ_LOBBY_END ? 0 : -1);
}

int clcode_status_POST(int status) {
    return (status > CL_POST_START && status < CL_POST_END ? 0 : -1);
}

int clcode_status_LOBBY_POST(int status) {
    return (status > CL_POST_LOBBY_START && status < CL_POST_LOBBY_END ? 0 : -1);
}

int clcode_REQ_redirect(int code, cli_t *client, int room, char *buffer) {
    int result = -1;

    switch(status) {
        case CL_REQ_ASSIGN_LOBBY:
            result = lobby_assign_cli(client);
            break;

        case CL_REQ_LOBBY_NICKNAME:
            break;

        default:
            break;
    }

    return result;
}

int clcode_POST_redirect(int code, cli_t *client, int room, char *buffer) {
    int result = 0;

    switch(code) {
        case CL_POST_LOBBY_LEAVE:
            break;

        case CL_POST_LOBBY_MOVE:
            result = lobby_redirect_buf(client, room, buffer);
            break;

        case CL_POST_LOBBY_MESG:
            result = lobby_redirect_buf(client, room, buffer);
            break;

        default:
            break;
    }

    return result;
}

int clcode_redirect(int code, cli_t *client, int room, char *buffer) {
    int result = 0;

    if (clcode_status_STATE(code) == 0) result = 0; // im not sure what to do with this and cli_t.status
    else if (clcode_status_REQ(code) == 0) result = clcode_REQ_redirect(code, client, room, buffer);
    else if (clcode_status_POST(code) == 0) result = clcode_POST_redirect(code, client, room, buffer);

    return result;
}

cli_t client_accept(int master_socket, struct sockaddr_in *addr) {

    cli_t client = {0, 0};
    int new_socket = -1;
    new_socket = accept(master_socket, (struct sockaddr*)addr, sizeof(addr));

    if (new_socket > 0) {
        client.socket = new_socket;
        client.status = CL_IDLE;
    }

    return client;
}

void client_disconnect(cli_t *client) {
    *client->socket = 0;
    *client->status = 0;
    return;
}

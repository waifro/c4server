#ifndef CLIENT_H
#define CLIENT_H

#include "net_utils.h"

int clcode_status_STATE(int code);
int clcode_status_REQ(int code);
int clcode_status_LOBBY_REQ(int code);
int clcode_status_POST(int code);
int clcode_status_LOBBY_POST(int code);
int clcode_status_LOBBY(int code);

int clcode_REQ_redirect(int code, cli_t *client, int room, char *buffer);
int clcode_POST_redirect(int code, cli_t *client, int room, char *buffer);
int clcode_LOBBY_REQ_redirect(int code, cli_t *client, int room, char *buffer);
int clcode_LOBBY_POST_redirect(int code, cli_t *client, int room, char *buffer);

int clcode_redirect(int code, cli_t *client, int room, char *buffer);

#endif

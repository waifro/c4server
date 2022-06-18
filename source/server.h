#ifndef SERVER_H
#define SERVER_H

#include "client.h"
#include "net_utils.h"

int svcode_status_STATE(int status);
int svcode_status_REQ(int status);
int svcode_status_LOBBY_REQ(int status);
int svcode_status_POST(int status);
int svcode_status_LOBBY_POST(int status);

int svcode_REQ_redirect(int code, cli_t *client, int room, char *buffer);
int svcode_POST_redirect(int code, cli_t *client, int room, char *buffer);

int svcode_redirect(int code, cli_t *client, int room, char *buffer);


#endif

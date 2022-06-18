#include "net_utils.h"

int handle_client(cli_t *client) {

    if (recv(client->socket, buffer, 255, 0) < 0) {
        //client_disconnect(client);
        memset(buffer, 0x00, 255);
        return -1;
    }

    int result = 0;
    result = verify_mesg_recv(mesg);
    if (result < 0) return -1;

    result = retrieve_code(mesg);
    if (result < 0) return -1;

    //clcode_redirect(result, client, room, buffer);

    return result;
}

int retrieve_code(char *mesg) {

    int code = 0;
    sscanf(mesg, "%d %*s", &code);

    return code;
}

int verify_mesg_recv(char *mesg) {

    if (strlen(mesg) < 3) return -1;
    else if (strlen(mesg) > 255) return -1;

    return 0;
}

int generate_val(int max) {
    //srand(time(NULL));
    return (rand() % max);
}

void init_client_list(cli_t *client_list, int max) {
    for (int i = 0; i < max; i++) {
        client_list[i].socket = 0;
        client_list[i].status = 0;
    }
}

void init_lobby_list(net_lobby *lobby_list, int max) {
    for (int i = 0; i < max; i++) {
        lobby_list[i].pair.cli_a = NULL;
        lobby_list[i].pair.cli_b = NULL;
        lobby_list[i].status = LB_AVAIL;
    }
}

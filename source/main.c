#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/time.h> // fd_set readfds;
#include <getopt.h>

#include "pp4m/pp4m_io.h"
#include "pp4m/pp4m_net.h"

#include "net_utils.h"
#include "server.h"
#include "client.h"
#include "lobby.h"
#include "global.h"

cli_t glo_client_list[MAX_CLIENTS];
net_lobby glo_lobby[MAX_LOBBY];

const char* const short_option = "th";
const struct option long_option[] = {
    { "testnet", 0, NULL, 't' },
    { "help", 1, NULL, 'h' },
    { NULL, 0, NULL, 0 }
};

int main(int argc, char *argv[]) {

    int master_socket = -1, result, port;
    struct sockaddr_in addr;

    result = getopt_long(argc, argv, short_option, long_option, NULL);
    if (result == -1) port = PORT_MAINNET;
    else if (result == 't') port = PORT_TESTNET;

    master_socket = pp4m_NET_Init(TCP);
    if (master_socket == -1) {
        printf("master_socket:  %s\n", strerror(errno));
        exit(0);
    }

    int opt = 1;
    if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (void*)&opt, sizeof(opt)) != 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    result = bind(master_socket, (struct sockaddr*)&addr, sizeof(addr));
    if (result == -1) {
        printf("bind: %s\n", strerror(errno));
        exit(0);
    }

    result = listen(master_socket, SOMAXCONN);
    if (result == -1) {
        printf("listen: %s\n", strerror(errno));
        exit(0);
    }

    printf("c4server ready\n");

    char buffer[256];

    int connected = 0;
    int max_socket = 0;

    cli_t buf_client = 0;

    init_client_list(glo_client_list, MAX_CLIENTS);

    fd_set sets_fd;
    struct timeval timeout = {0, 0};

    init_lobby_list(glo_lobby, MAX_LOBBY);

    printf("c4server starting... idle\n\n");

    while(1) {

        FD_ZERO(&sets_fd);

        FD_SET(master_socket, &sets_fd);
        max_socket = master_socket;

        for (int i = 0; i < MAX_CLIENTS; i++) {

            // if valid socket descriptor then add to read list
            if (glo_client_list[i] != 0)
                FD_SET(glo_client_list[i], &sets_fd);

            //set highest file descriptor number, need it for the select() function
            if(glo_client_list[i] > max_socket)
                max_socket = glo_client_list[i];
        }



        result = select(max_socket + 1, &sets_fd, NULL, NULL, &timeout);
        if (result == -1) {
            printf("select: %s, %d\n", strerror(errno), pp4m_NET_RecieveError());
            exit(0);
        }

        if (FD_ISSET(master_socket, &sets_fd)) {

            cli_t client = client_accept(master_socket, &addr);
            printf("client connect: %s:%d\t", inet_ntoa(addr.sin_addr), htons(addr.sin_port));

            if (connected > MAX_CLIENTS) {
                printf("[%d of %d] | server full, ignored\n", --connected, MAX_CLIENTS);
            } else {

                for (int i = 0; i < MAX_CLIENTS; i++)
                if (glo_client_list[i] == 0) {
                    glo_client_list[i] = client;
                    break;
                }

                printf("[%d of %d] ", ++connected, MAX_CLIENTS);

            }
        }

        // update state of other sockets
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (glo_client_list[i] == 0) continue;
            buf_client = glo_client_list[i];

            // if an old connection triggered
            if (FD_ISSET(buf_client, &sets_fd)) {

                result = handle_client(&buf_client, buffer);

                // lost connection (?)
                if (result == -1) {
                    getpeername(buf_client, (struct sockaddr*)&addr, &sockaddr_size);

					printf("client discnct: %s:%d\t[%d of %d] | ", inet_ntoa(addr.sin_addr), htons(addr.sin_port), --connected, MAX_CLIENTS);

                    int room = lobby_updateroom_cli_left(&buf_client);

                    client_disconnect(&glo_client_list[i]);
                    glo_client_list[i] = 0;

                    if (room >= MAX_LOBBY) printf("\n");
                    else printf("roomId %d[%p:%p]\n", room, glo_lobby[room].pair.cli_a, glo_lobby[room].pair.cli_b);

                } else if (clcode_status_LOBBY(result) == 0) {

                    int room = -1;
                    for (int n = 0; n < MAX_LOBBY; n++) {
                        if (lobby_checkroom_cli(&buf_client, n) == -1) continue;

                        room = n;
                        break;
                    }

                    if (room == -1) {
                        printf("error, couldn't find client on lobbies: %d, [%s]\n", buf_client, buffer);
                        continue;
                    }

                    if (clcode_status_LOBBY_REQ(result) == 0) {

                        clcode_LOBBY_REQ_redirect(result, &buf_client, room, buffer);

                    } else { // clcode_status_LOBBY_POST

                        clcode_LOBBY_POST_redirect(result, &buf_client, room, buffer);

                    }
                } else if (clcode_status_REQ(result) == 0) {

                    clcode_REQ_redirect(result, &buf_client, -1, buffer);

                } else if (clcode_status_POST(result) == 0) {

                    clcode_POST_redirect(result, &buf_client, -1, buffer);

                }
            }
        }
    }

    // After chatting close the socket
    close(master_socket);
}

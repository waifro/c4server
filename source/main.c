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

#include "c4network/net_utils.h"
#include "c4network/server.h"
#include "c4network/client.h"
#include "c4network/lobby.h"
#include "c4network/global.h"

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

    printf("c4server ready: port %d\n", port);

    char buffer[256];
    socklen_t addr_size = sizeof(struct sockaddr);

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

            if (connected == MAX_CLIENTS) {
                printf("[%d of %d] | server full, ignored\n", connected, MAX_CLIENTS);
            } else {

                for (int i = 0; i < MAX_CLIENTS; i++)
                if (glo_client_list[i] == 0) {
                    glo_client_list[i] = client;
                    break;
                }

                printf("[%d of %d] |\n", ++connected, MAX_CLIENTS);

            }
        }

        // update state of other sockets
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (glo_client_list[i] == 0) continue;

            // if an old connection triggered
            if (FD_ISSET(glo_client_list[i], &sets_fd)) {

                result = sv_handlePacket(&glo_client_list[i], buffer);

                // lost connection (?)
                if (result == -1) {
                    getpeername(glo_client_list[i], (struct sockaddr*)&addr, &addr_size);

					printf("client discnct: %s:%d\t[%d of %d] | ", inet_ntoa(addr.sin_addr), htons(addr.sin_port), --connected, MAX_CLIENTS);

                    int room = lobby_updateroom_cli_left(glo_lobby, &glo_client_list[i]);

                    client_disconnect(&glo_client_list[i]);
                    glo_client_list[i] = 0;

                    if (room >= MAX_LOBBY) printf("\n");
                    else printf("roomId %d[%p:%p]\n", room, glo_lobby[room].pair.cli_a, glo_lobby[room].pair.cli_b);

                } else if (cl_status_LOBBY(result) == 0) {

                    int room = -1;
                    for (int n = 0; n < MAX_LOBBY; n++) {
                        if (lobby_checkroom_cli(glo_lobby, &glo_client_list[i], n) == -1) continue;

                        room = n;
                        break;
                    }

                    if (room == -1) {
                        printf("error, couldn't find client on lobbies: %d, [%s]\n", glo_client_list[i], buffer);
                        continue;
                    }

                    sv_clcode_redirect(result, glo_lobby, &glo_client_list[i], room, buffer);

                } else sv_clcode_redirect(result, glo_lobby, &glo_client_list[i], -1, buffer);
            }

            for (int i = 0; i < MAX_LOBBY; i++) {

                // lobby is signed full, ready to play
                if (lobby_checkroom_isready(glo_lobby, i) == 1) {
                    sv_redirect_svcode_POST(SV_LOBBY_POST_START, glo_lobby, NULL, i, NULL);
                    printf("roomId %d[%p:%p] started\n", i, glo_lobby[i].pair.cli_a, glo_lobby[i].pair.cli_b);
                }
            }
        }
    }

    // After chatting close the socket
    close(master_socket);
}

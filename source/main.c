#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/time.h> // fd_set readfds;
#include <getopt.h>

#include "pp4m/pp4m.h"
#include "pp4m/pp4m_io.h"
#include "pp4m/pp4m_net.h"

#include "c4network/net_utils.h"
#include "c4network/server.h"
#include "c4network/client.h"
#include "c4network/lobby.h"

#include "global.h"

cli_t glo_client_list[MAX_CLIENTS];
net_lobby glo_lobby[MAX_LOBBY];

const char* const short_option = "th";
const struct option long_option[] = {
    { "testnet", 0, NULL, 't' },
    { "help", 1, NULL, 'h' },
    { NULL, 0, NULL, 0 }
};

int addr_init(struct sockaddr_in *addr, int port) {
	if (addr == NULL) return -1;
	
	addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = INADDR_ANY;
    addr->sin_port = htons(port);
	
	return 0;
}

int addr_start_init(int *master_socket, struct sockaddr_in *addr, int port) {
	int result = -1;
	
	*master_socket = pp4m_NET_Init(TCP);
    if (*master_socket == -1) {
        printf("master_socket:  %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(*master_socket, SOL_SOCKET, SO_REUSEADDR, (void*)&opt, sizeof(opt)) != 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    if (addr_init(addr, port) == -1) {
		printf("addr not initialized\n");
		exit(EXIT_FAILURE);
	}

    result = bind(*master_socket, (struct sockaddr*)addr, sizeof(struct sockaddr_in));
    if (result == -1) {
        printf("bind: %s\n", strerror(errno));
        return result;
    }

    result = listen(*master_socket, SOMAXCONN);
    if (result == -1) {
        printf("listen: %s\n", strerror(errno));
        return result;
    }
	
	return result;
}

int socket_checklist(fd_set *sets, int *master_socket, int **sockets_list, int max_sockets) {
	
	for (int i = 0; i < max_sockets; i++) {

		// if valid socket descriptor then add to read list
        if (sockets_list[i] != 0)
        	FD_SET(sockets_list[i], sets);

		//set highest file descriptor number, need it for the select() function
		if(sockets_list[i] > *master_socket)
			*master_socket = sockets_list[i];
    }
        
	return 0;
}

int main(int argc, char *argv[]) {
	int result = -1;
	
	int master_socket = -1, port;
    struct sockaddr_in addr;
	
	// read argv to set port
    result = getopt_long(argc, argv, short_option, long_option, NULL);
    if (result == -1) port = PORT_MAINNET;
    else if (result == 't') port = PORT_TESTNET;
	
    if (addr_start_init(&master_socket, &addr, port) == -1)
		exit(EXIT_FAILURE);
	
	// server is ready to operate
    printf("c4server ready: port %d\n", port);

    char buffer[256];
    socklen_t addr_size = sizeof(struct sockaddr);

    int connected = 0;
    int max_socket = 0;

    init_client_list(glo_client_list, MAX_CLIENTS);

    fd_set sets_fd;
    struct timeval timeout = {0, 0};

    init_lobby_list(glo_lobby, MAX_LOBBY);

	time_t t;
    time(&t);
    printf("c4server starting at %s\n\n", ctime(&t));

    while(1) {

        FD_ZERO(&sets_fd);

        FD_SET(master_socket, &sets_fd);
        max_socket = master_socket;
		
		// sort out and check list
        socket_checklist(&sets_fd, &max_socket, glo_client_list, MAX_CLIENTS);

        result = select(max_socket + 1, &sets_fd, NULL, NULL, &timeout);
        if (result == -1) {
            printf("select: %s, %d\n", strerror(errno), pp4m_NET_RecieveError());
            exit(EXIT_FAILURE);
        }
		
		// new connections
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

					printf("something went wrong\n");

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
			
			// lobby updates
            for (int i = 0; i < MAX_LOBBY; i++) {
				
				// terminate/recycle an empty room
				if (lobby_checkroom_endcycle(glo_lobby, i) == 1)
					lobby_updateroom_reset(glo_lobby, i);
				
				// check timers of clients
				if (lobby_checkroom_isbusy(glo_lobby, i) == 1) {
					
					// timer updates
					if (glo_lobby[i].utimer == *glo_lobby[i].pair.cli_a) {
						
						if (pp4m_FramerateTimer(CLOCKS_PER_SEC, (int*)&glo_lobby[i].clock_a, glo_lobby[i].timestamp + glo_lobby[i].clock_b) == true) {
							
							// post to clients the timeframe, they will do match calculations afterwards
							sv_redirect_svcode_LOBBY_POST(SV_LOBBY_POST_TIME, glo_lobby, NULL, i, NULL);
						}
						
					} else {
						
						if (pp4m_FramerateTimer(CLOCKS_PER_SEC, (int*)&glo_lobby[i].clock_b, glo_lobby[i].timestamp + glo_lobby[i].clock_a) == true) {
							
							// post to clients the timeframe, they will do match calculations afterwards
							sv_redirect_svcode_LOBBY_POST(SV_LOBBY_POST_TIME, glo_lobby, NULL, i, NULL);
						}
						
					}
					
				}
				
                // lobby is signed full, ready to play
                else if (lobby_checkroom_isready(glo_lobby, i) == 1) {
                    sv_redirect_svcode_POST(SV_LOBBY_POST_INIT, glo_lobby, NULL, i, NULL);
                    printf("roomId %d[%p:%p] started\n", i, glo_lobby[i].pair.cli_a, glo_lobby[i].pair.cli_b);
                }
            }
        }
    }

    // After chatting close the socket
    close(master_socket);
}

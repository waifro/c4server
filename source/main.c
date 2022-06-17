#ifdef _WIN32
    #include <winsock2.h> //ws2_32
    #include <windows.h>
    #define socklen_t int
#else // _UNIX

    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <arpa/inet.h>
    #include <netdb.h>
#endif

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/time.h> // fd_set readfds;

#include "pp4m/pp4m_io.h"
#include "pp4m/pp4m_net.h"

#include "config.h"


int clcode_REQ_redirect(cli_t *client, int status) {
    int result = 0;

    switch(status) {
        case CL_REQ_ASSIGN_LOBBY:
            break;

        case CL_REQ_TOTAL_USERS:
            break;

        default:
            break;
    }

    return result;
}

int clcode_POST_redirect(cli_t *client, int status) {
    int result = 0;

    switch(status) {
        case CL_POST_LOBBY_LEAVE:
            break;

        default:
            break;
    }

    return result;
}

int clcode_status_REQ(int status) {
    return (status > CL_REQ_START && status < CL_REQ_END ? 0 : -1);
}

int clcode_status_POST(int status) {
    return (status > CL_POST_START && status < CL_POST_END ? 0 : -1);
}

int clcode_redirect(cli_t *client, int status) {
    int result = 0;

    if (clcode_status_REQ(status) == 0) result = clcode_REQ_redirect(client, status);
    else if (clcode_status_POST(status) == 0) result = clcode_POST_redirect(client, status);

    return result;
}

cli_t accept_client(int master_socket, struct sockaddr_in *addr) {

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

int handle_client(cli_t *client) {

    char buffer[256];
    if (recv(client->socket, buffer, 255, 0) < 0) {
        client_disconnect(client);
        memset(buffer, 0x00, 255);
        return -1;
    }

    int result = 0;
    result = verify_mesg_recv(mesg);
    if (result < 0) return -1;

    result = parse_mesg_recv(mesg);
    if (result < 0) return -1;

    clcode_redirect(client, result);

    return 0;
}

int parse_mesg_recv(char *mesg) {

    int code;
    sscanf(mesg, "%d %*s", &code);

    return code;
}

int verify_mesg_recv(char *mesg) {

    if (strlen(mesg) < 3) return -1;
    else if (strlen(mesg) > 255) return -1;

    return 0;
}

/*

int genval(int max) {
    //srand(time(NULL));
    return (rand() % max);
}

int init_roompair_randpl(net_lobby *lobby, char *fen) {

    int val = genval(100);
    char buf[256];
    sprintf(buf, "w %s", fen);

    if (val < 50) {
        send(lobby->pair.sfd_a, buf, strlen(buf), 0); buf[0] = 'b';
        send(lobby->pair.sfd_b, buf, strlen(buf), 0);
    } else {
        send(lobby->pair.sfd_b, buf, strlen(buf), 0); buf[0] = 'b';
        send(lobby->pair.sfd_a, buf, strlen(buf), 0);
    }

    lobby->status = LB_BUSY;

    return 0;
}

int fill_roompair(net_lobby *lobby, int socket) {
    int result = -1;

    if (lobby->pair.sfd_a == 0) {
        lobby->pair.sfd_a = socket;

        if (lobby->pair.sfd_b > 0)
            lobby->status = LB_FULL;
        result += 1;
    } else if (lobby->pair.sfd_b == 0) {
        *lobby->pair.sfd_b->socket = socket;

        if (lobby->pair.sfd_a > 0)
            lobby->status = LB_FULL;
        result += 1;
    }

    return result;
}

int update_roompair(net_lobby *lobby, int socket, char *buf) {
    if (socket < 1) return -1;

    printf("update_lobbypair: %d\n", lobby->status);

    int result = -1;
    if (socket == lobby->pair.sfd_a) {
        send(lobby->pair.sfd_b, buf, strlen(buf), 0);
        result++;
    } else if (socket == lobby->pair.sfd_b) {
        send(lobby->pair.sfd_a, buf, strlen(buf), 0);
        result++;
    }

    return result;
}

int assign_roompair(net_lobby *lobby, int socket) {

    int room = 0;
    for (room = 0; room < MAX_LOCAL; room++) {
        if (lobby[room].status == LB_AVAIL) {
            fill_roompair(&lobby[room], new_socket);
            break;
        }
    }

    if (room >= MAX_LOCAL) printf("\n");
    else printf("| assigned roomId: %d[%d:%d]\n", room, lobby[room].pair.sfd_a, lobby[room].pair.sfd_b);

    for (room = 0; room < MAX_LOCAL; room++) {
        if (lobby[room].status == LB_FULL) {
            init_roompair_randpl(&lobby[room], fen_standard);
            break;
        }
    }

    return 0;
}
*/


int main(void) {

    int master_socket = -1, result;
    struct sockaddr_in addr;

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
    addr.sin_port = htons(PORT);

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

    char *fen_standard = "5k2/P4p2/4p2p/8/6p2/5P2/2K2p2/8 w - - 0 0";

    char buffer[256];

    int connected = 0;
    int new_socket = 0;
    int max_socket = 0;
    int buf_socket = 0;
    int client_socklist[MAX_CLIENTS];

    for(int i = 0; i < MAX_CLIENTS; i++)
        client_socklist[i] = 0;

    fd_set sets_fd;
    socklen_t sockaddr_size = sizeof(struct sockaddr);
    struct timeval timeout = {0, 0};

    net_lobby lobby[MAX_LOCAL];

    for (int i = 0; i < MAX_LOCAL; i++) {
        lobby[i].pair.sfd_a = 0;
        lobby[i].pair.sfd_b = 0;
        lobby[i].status = LB_AVAIL;
    }


    printf("c4server starting... idle\n\n");

    while(1) {

        FD_ZERO(&sets_fd);

        FD_SET(master_socket, &sets_fd);
        max_socket = master_socket;

        for (int i = 0; i < MAX_CLIENTS; i++) {

            // if valid socket descriptor then add to read list
            if (client_socklist[i] > 0)
                FD_SET(client_socklist[i], &sets_fd);

            //set highest file descriptor number, need it for the select() function
            if(client_socklist[i] > max_socket)
                max_socket = client_socklist[i];
        }



        result = select(max_socket + 1, &sets_fd, NULL, NULL, &timeout);
        if (result == -1) {
            printf("select: %s, %d\n", strerror(errno), pp4m_NET_RecieveError());
            exit(0);
        }

        if (FD_ISSET(master_socket, &sets_fd)) {

            new_socket = accept(master_socket, (struct sockaddr*)&addr, &sockaddr_size);
            if (new_socket == -1) {
                printf("accept: %s, %d\n", strerror(errno), pp4m_NET_RecieveError());
                exit(0);
            }

            printf("client connect: %s:%d\t", inet_ntoa(addr.sin_addr), htons(addr.sin_port));

            if (connected > MAX_CLIENTS) {
                printf("[%d of %d] | server full, ignored\n", --connected, MAX_CLIENTS);
            } else {

				for (int i = 0; i < MAX_CLIENTS; i++)
                if (client_socklist[i] == 0) {
                    client_socklist[i] = new_socket;
                    break;
                }

                printf("[%d of %d] ", ++connected, MAX_CLIENTS);

            }
        }

        // update state of other sockets
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_socklist[i] == 0) continue;
            buf_socket = client_socklist[i];

            // if an old connection triggered
            if (FD_ISSET(buf_socket, &sets_fd)) {

                // lost connection (?)
                if (recv(buf_socket, buffer, 255, 0) < 0) {
                    getpeername(buf_socket, (struct sockaddr*)&addr, &sockaddr_size);

					printf("buf: %s, errror\n", buffer);

                    printf("client discnct: %s:%d\t[%d of %d] | ", inet_ntoa(addr.sin_addr), htons(addr.sin_port), --connected, MAX_CLIENTS);

                    int room = 0;
                    for (room = 0; room < MAX_LOCAL; room++) {
                        if (buf_socket == lobby[room].pair.sfd_a || buf_socket == lobby[room].pair.sfd_b) {

                            if (buf_socket == lobby[room].pair.sfd_a) {
                                lobby[room].pair.sfd_a = 0; lobby[room].status = LB_ERROR;
                            } else if (buf_socket == lobby[room].pair.sfd_b) {
                                lobby[room].pair.sfd_b = 0; lobby[room].status = LB_ERROR;
                            }

                            // room cleaned
                            if (lobby[room].pair.sfd_a == 0 && lobby[room].pair.sfd_b == 0)
                                lobby[room].status = LB_AVAIL;

                            break;
                        }
                    }

                    close(buf_socket);
                    client_socklist[i] = 0;

                    if (room >= MAX_LOCAL) printf("\n");
                    else printf("roomId %d[%d:%d]\n", room, lobby[room].pair.sfd_a, lobby[room].pair.sfd_b);
                } else {

                    printf("recieved buf: %s\n", buffer);

                    for (int n = 0; n < MAX_LOCAL; n++)
                        if (update_roompair(&lobby[n], buf_socket, buffer) != -1) break;
                }
            }
        }
    }

    // After chatting close the socket
    close(master_socket);
}

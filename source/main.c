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
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/time.h> // fd_set readfds;

#include "pp4m/pp4m_io.h"
#include "pp4m/pp4m_net.h"

#define PORT 62443
#define MAX_CLIENTS 2
#define MAX_ROOMID MAX_CLIENTS/2

// this struct will be used to join two incoming connections to compete the game
typedef struct {
    int sfd_a, sfd_b;
} pair_sockfd_t;

int random_plsw(void) {
    srand(time(NULL));
    return (rand() % 2);
}


// Driver function
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

    char send_room_init[512];
    char fen_init_standard[] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 0";

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

    pair_sockfd_t sockfd_room[MAX_ROOMID];

    for (int i = 0; i < MAX_ROOMID; i++) {
        sockfd_room[i].sfd_a = 0;
        sockfd_room[i].sfd_b = 0;
    }


    printf("c4server starting... idle\n\n");

    while(1) {

        FD_ZERO(&sets_fd);

        FD_SET(master_socket, &sets_fd);
        max_socket = master_socket;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_socklist[i] != 0)
                buf_socket = client_socklist[i];

            // if valid socket descriptor then add to read list
            if (buf_socket > 0)
                FD_SET(buf_socket, &sets_fd);

            //set highest file descriptor number, need it for the select() function
            if(buf_socket > max_socket)
                max_socket = buf_socket;
        }



        result = select(master_socket + 1, &sets_fd, NULL, NULL, &timeout);
        if (result == -1) {
            printf("select: %s\n", strerror(errno));
            exit(0);
        }

        if (FD_ISSET(master_socket, &sets_fd)) {

            new_socket = accept(master_socket, (struct sockaddr*)&addr, &sockaddr_size);
            if (new_socket == -1) {
                printf("accept: %s, %d\n", strerror(errno), pp4m_NET_RecieveError());
                exit(0);
            }

            for (int i = 0; i < MAX_CLIENTS; i++)
                if (client_socklist[i] == 0) {
                    client_socklist[i] = new_socket;
                    break;
                }

            printf("client connect: %s:%d\t", inet_ntoa(addr.sin_addr), htons(addr.sin_port));

            if (connected > MAX_CLIENTS) {
                printf("[%d of %d] | server full, ignored\n", --connected, MAX_CLIENTS);
            } else {

                printf("[%d of %d] | ", ++connected, MAX_CLIENTS);

                int room = 0;
                for (room = 0; room < MAX_ROOMID; room++) {
                    if (sockfd_room[room].sfd_a == 0) {
                        sockfd_room[room].sfd_a = new_socket;
                        break;
                    } else if (sockfd_room[room].sfd_b == 0) {
                        sockfd_room[room].sfd_b = new_socket;
                        break;
                    }
                }


                if (room >= MAX_ROOMID) printf("\n");
                else printf("assigned roomId: %d[%d:%d]\n", room, sockfd_room[room].sfd_a, sockfd_room[room].sfd_b);

                for (room = 0; room < MAX_ROOMID; room++) {
                    if (sockfd_room[room].sfd_a != 0 && sockfd_room[room].sfd_b != 0) {

                        int result = random_plsw();

                        if (result < 1) {
                            sprintf(send_room_init, "%d %c %s%c", room, 'w', fen_init_standard, '\0');
                            send(sockfd_room[room].sfd_a, send_room_init, 511, 0);
                            sprintf(send_room_init, "%d %c %s%c", room, 'b', fen_init_standard, '\0');
                            send(sockfd_room[room].sfd_b, send_room_init, 511, 0);
                        } else if (result == 1) {
                            sprintf(send_room_init, "%d %c %s%c", room, 'w', fen_init_standard, '\0');
                            send(sockfd_room[room].sfd_b, send_room_init, 511, 0);
                            sprintf(send_room_init, "%d %c %s%c", room, 'b', fen_init_standard, '\0');
                            send(sockfd_room[room].sfd_a, send_room_init, 511, 0);
                        }

                    }
                }
            }
        }

        // update state of other sockets
        for (int i = 0; i < MAX_CLIENTS; i++) {
            buf_socket = client_socklist[i];

            // if an old connection triggered
            if (FD_ISSET(buf_socket, &sets_fd)) {
                printf("detected socket\n");

                // lost connection (?)
                if (read(buf_socket, buffer, 255) == -1) {
                    getpeername(buf_socket, (struct sockaddr*)&addr, &sockaddr_size);

                    printf("client discnct: %s:%d\t[%d of %d] | ", inet_ntoa(addr.sin_addr), htons(addr.sin_port), --connected, MAX_CLIENTS);

                    int room = 0;
                    for (room = 0; room < MAX_ROOMID; room++) {
                        if (sockfd_room[room].sfd_a == buf_socket) {
                            sockfd_room[room].sfd_a = 0;
                            break;
                        } else if (sockfd_room[room].sfd_b == buf_socket) {
                            sockfd_room[room].sfd_b = 0;
                            break;
                        }
                    }

                    close(buf_socket);
                    client_socklist[i] = 0;

                    if (room >= MAX_ROOMID) printf("\n");
                    else printf("roomId %d[%d:%d]\n", room, sockfd_room[room].sfd_a, sockfd_room[room].sfd_b);
                } else {


                    for (int n = 0; n < MAX_ROOMID; n++) {
                        if (buf_socket == sockfd_room[n].sfd_a || buf_socket == sockfd_room[n].sfd_b) {

                            if (buf_socket == sockfd_room[n].sfd_a) {

                                send(sockfd_room[n].sfd_b, buffer, strlen(buffer), 0);

                            } else if (buf_socket == sockfd_room[n].sfd_b) {

                                send(sockfd_room[n].sfd_a, buffer, strlen(buffer), 0);

                            }

                            break;
                        }
                    }

                }
            }
        }
    }

    // After chatting close the socket
    close(master_socket);
}

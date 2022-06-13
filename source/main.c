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

#define PORT 62443
#define MAX_CLIENTS 2
#define MAX_LOCAL MAX_CLIENTS/2

typedef enum {
    AVAIL = 0,
    FULL = 1,
    BUSY = 2,
    BLOCKED = 3
} ROOM_STATUS;

// this struct will be used to join two incoming connections to compete the game
typedef struct {
    int sfd_a, sfd_b;   // file descriptors sockets for sending/recieving packets
} pair_sockfd_t;

typedef struct {
    pair_sockfd_t pair;
    ROOM_STATUS status;
} room_cell;


int genval(int max) {
    srand(time(NULL));
    return (rand() % max);
}

int init_roompair_randpl(room_cell *local, const char *fen) {

    int val = genval(100);
    char buf[256];

    if (val < 50) {
        sprintf(buf, "%c %s", 'w', fen);
        send(local->pair.sfd_a, buf, strlen(buf), 0);
        sprintf(buf, "%c %s", 'b', fen);
        send(local->pair.sfd_b, buf, strlen(buf), 0);
    } else {
        printf(buf, "%c %s", 'w', fen);
        send(local->pair.sfd_b, buf, strlen(buf), 0);
        sprintf(buf, "%c %s", 'b', fen);
        send(local->pair.sfd_a, buf, strlen(buf), 0);
    }

    local->status = BUSY;

    return 0;
}

int fill_roompair(room_cell *local, int socket) {
    int result = -1;

    if (local->pair.sfd_a == 0) {
        local->pair.sfd_a = socket;

        if (local->pair.sfd_b > 0)
            local->status = FULL;
        result += 1;
    } else if (local->pair.sfd_b == 0) {
        local->pair.sfd_b = socket;

        if (local->pair.sfd_a > 0)
            local->status = FULL;
        result += 1;
    }

    return result;
}

int update_roompair(room_cell *local, int socket, char *buf) {
    if (socket < 1) return -1;

    int result = -1;
    if (socket == local->pair.sfd_a) {
        send(local->pair.sfd_a, buf, sizeof(buf), 0);
        result++;
    } else if (socket == local->pair.sfd_b) {
        send(local->pair.sfd_b, buf, sizeof(buf), 0);
        result++;
    }

    return result;
}

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

    const char *fen_standard = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 0";

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

    room_cell local[MAX_LOCAL];

    for (int i = 0; i < MAX_LOCAL; i++) {
        local[i].pair.sfd_a = 0;
        local[i].pair.sfd_b = 0;
        local[i].status = AVAIL;
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

            for (int i = 0; i < MAX_CLIENTS; i++)
                if (client_socklist[i] == 0) {
                    client_socklist[i] = new_socket;
                    break;
                }

            printf("client connect: %s:%d\t", inet_ntoa(addr.sin_addr), htons(addr.sin_port));

            if (connected > MAX_CLIENTS) {
                printf("[%d of %d] | server full, ignored\n", --connected, MAX_CLIENTS);
            } else {

                printf("[%d of %d] ", ++connected, MAX_CLIENTS);

                int room = 0;
                for (room = 0; room < MAX_LOCAL; room++) {
                    if (local[room].status == AVAIL) {
                        fill_roompair(&local[room], new_socket);
                        break;
                    }
                }


                if (room >= MAX_LOCAL) printf("\n");
                else printf("| assigned roomId: %d[%d:%d]\n", room, local[room].pair.sfd_a, local[room].pair.sfd_b);

                for (room = 0; room < MAX_LOCAL; room++) {
                    if (local[room].status == FULL) {
                        init_roompair_randpl(&local[room], fen_standard);
                        break;
                    }
                }
            }
        }

        // update state of other sockets
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_socklist[i] != 0)
                buf_socket = client_socklist[i];

            // if an old connection triggered
            if (FD_ISSET(buf_socket, &sets_fd)) {

                // lost connection (?)
                if (read(buf_socket, buffer, 255) < 0) {
                    getpeername(buf_socket, (struct sockaddr*)&addr, &sockaddr_size);

                    printf("client discnct: %s:%d\t[%d of %d] | ", inet_ntoa(addr.sin_addr), htons(addr.sin_port), --connected, MAX_CLIENTS);

                    int room = 0;
                    for (room = 0; room < MAX_LOCAL; room++) {
                        if (local[room].pair.sfd_a == buf_socket) {
                            local[room].pair.sfd_a = 0;
                            break;
                        } else if (local[room].pair.sfd_b == buf_socket) {
                            local[room].pair.sfd_b = 0;
                            break;
                        }
                    }

                    close(buf_socket);
                    client_socklist[i] = 0;
                    buf_socket = 0;

                    if (room >= MAX_LOCAL) printf("\n");
                    else printf("roomId %d[%d:%d]\n", room, local[room].pair.sfd_a, local[room].pair.sfd_b);
                } else {

                    printf("recieved buf: %s\n", buffer);

                    for (int n = 0; n < MAX_LOCAL; n++)
                        if (update_roompair(&local[n], buf_socket, buffer) != -1) break;
                }
            }
        }
    }

    // After chatting close the socket
    close(master_socket);
}

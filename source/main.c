#ifdef _WIN32
    #include <winsock2.h> //ws2_32
    #include <windows.h>
#else // _UNIX

    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <arpa/inet.h>
    #include <netdb.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

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


// Driver function
int main(void) {

    int master_socket = -1, result;
    struct sockaddr_in addr;

    master_socket = pp4m_NET_Init(TCP);
    if (master_socket == -1) {
        printf("master_socket:  %s\n", strerror(errno));
        exit(0);
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

    char buffer[256];
    int connected = 0;
    int new_socket = 0;
    int max_socket = 0;
    int buf_socket = 0;
    int client_socklist[MAX_CLIENTS];

    for(int i = 0; i < MAX_CLIENTS; i++)
        client_socklist[i] = 0;

    fd_set sets_fd;
    int sockaddr_size = sizeof(struct sockaddr);
    struct timeval timeout = {0, 0};

    int roomId = 0;
    pair_sockfd_t sockfd_room[MAX_ROOMID];

    printf("c4server starting... idle\n\n");

    while(1) {

        FD_ZERO(&sets_fd);

        FD_SET(master_socket, &sets_fd);
        max_socket = master_socket;

        for (int i = 0; i < MAX_CLIENTS; i++) {
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
                printf("accept: %s, %d\n", strerror(errno), WSAGetLastError());
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
            }
        }

        // update state of other sockets
        for (int i = 0; i < MAX_CLIENTS; i++) {
            buf_socket = client_socklist[i];

            // if an old connection triggered
            if (FD_ISSET(buf_socket, &sets_fd)) {

                // lost connection (?)
                if (read(buf_socket, buffer, 255) == -1) {
                    getpeername(buf_socket, (struct sockaddr*)&addr, &sockaddr_size);

                    printf("client disconnct: %s:%d\t[%d of %d] | ", inet_ntoa(addr.sin_addr), htons(addr.sin_port), --connected, MAX_CLIENTS);

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
                }
            }
        }
    }

    /*
    int master_socket_fd = 0;
    struct sockaddr_in addr;
    memset(&addr, 0x00, sizeof(addr));

    int client_hooked_sockfd[MAX_CLIENTS];

    // initialize "client_hooked_sockfd" to not waste time afterwards
    for (int i = 0; i < 30; i++) {
        client_hooked_sockfd[i] = 0;
    }

    if ((master_socket_fd = pp4m_NET_Init(TCP)) == -1) {
        printf("socket creation failed...\n");
        exit(0);
    } printf("socket successfully created...\n");


    //set master socket to allow multiple connections ,
    //this is just a good habit, it will work without this
    int opt = 1;
    if(setsockopt(master_socket_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&opt, sizeof(opt)) != 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // assign IP, PORT
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    // Binding newly created socket to given IP and verification
    if ((bind(master_socket_fd, (SA*)&addr, sizeof(addr))) != 0) {
        printf("socket bind failed...\n");
        exit(0);
    } printf("socket successfully binded...\n");


    // maximum of 5 pending connections
    if ((listen(master_socket_fd, 5)) != 0) {
        printf("Listen failed...\n");
        exit(0);
    } printf("server waiting...\n");


    struct sockaddr_in cliaddr;

    int addr_size = sizeof(cliaddr);

    // Accept the data packet from client and verification
    int new_socket = 0;

    // set of socket descriptors
    fd_set read_setd;

    // hook socket's client_hooked_sockfd[n] temporary
    int buf_sockfd;

    // used for highest socket number
    int max_sockfd;

    // required by select func (timeout argv on NULL: indefinitely, both 0: immidiately)
    struct timeval timeout = {0, 0};

    char buffer_msg[256];
    int result = -1;
    while(1)
    {
        // sets must be reinitialized before each call
        FD_ZERO(&read_setd);

        // add master socket to set
        FD_SET(master_socket_fd, &read_setd);
        max_sockfd = master_socket_fd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            buf_sockfd = client_hooked_sockfd[i];

            // if valid socket descriptor then add to read list
            if (buf_sockfd > 0)
                FD_SET(buf_sockfd, &read_setd);

            //set highest file descriptor number, need it for the select() function
            if(buf_sockfd > max_sockfd)
                max_sockfd = buf_sockfd;
        }

        // listen to incoming connections
        result = select(max_sockfd + 1, &read_setd, NULL, NULL, &timeout);
        if (result == -1) perror("error on a file descriptor");

        // triggers on new incoming connection
        if (FD_ISSET(master_socket_fd, &read_setd)) {
            new_socket = connect(master_socket_fd, (SA*)&addr, addr_size);

            // no errors on new socket
            if (new_socket == -1) {
                perror("error new socket");
                continue;
            }

            printf("client connected: %s:%d\t[%d]\n", inet_ntoa(addr.sin_addr), htons(addr.sin_port), new_socket);

            // join new socket into list
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_hooked_sockfd[i] == 0) {
                    client_hooked_sockfd[i] = new_socket;
                    break;
                }
            }
        }

        // update state of other sockets
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            buf_sockfd = client_hooked_sockfd[i];

            // if an old connection triggered
            if (FD_ISSET(buf_sockfd, &read_setd)) {

                // lost connection (?)
                if (read(buf_sockfd, buffer_msg, 255) == 0) {
                    getpeername(buf_sockfd, (SA*)&addr, &addr_size);
                    printf("client disconnected: ip %s:%d\t[%d]\n", inet_ntoa(addr.sin_addr), htons(addr.sin_port), buf_sockfd);

                    close(buf_sockfd);
                    client_hooked_sockfd[i] = 0;
                }
            }
        }
    }

    */
    // After chatting close the socket
    close(master_socket);
}

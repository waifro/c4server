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

#define MAX 80
#define PORT 62443
#define SA struct sockaddr

#define MAX_CLIENTS 30

// this struct will be used to join two incoming connections to compete the game
typedef struct {
    int sfd_a, sfd_b;
} pair_sockfd_t;

/*
// Function designed for chat between client and server.
void func(int connfd, struct sockaddr_in in)
{
    char buff[MAX];
    int n;

    // infinite loop for chat
    while(1) {

        memset(buff, 0x00, MAX);

        // read the message from client and copy it in buffer
        if (recv(connfd, buff, MAX, 0) < 0) {
            printf("client id disconnected: %d\tip: [%s:%d]\n", connfd, inet_ntoa(in.sin_addr), ntohs(in.sin_port));
            break;
        }

        // print buffer which contains the client contents
        printf("From client: %s\t To client : ", buff);

        memset(buff, 0x00, MAX);

        n = 0;

        // copy server message in the buffer
        while ((buff[n++] = getchar()) != '\n') ;

        // and send that buffer to client
        write(connfd, buff, sizeof(buff));

    }
}
*/

// Driver function
int main(int argc, char *argv[]) {

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
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);

    // Binding newly created socket to given IP and verification
    if ((bind(master_socket_fd, (SA*)&addr, sizeof(addr))) != 0) {
        printf("socket bind failed...\n");
        exit(0);
    } printf("socket successfully binded...\n");


    // Now server is ready to listen and verification
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
        if (result == -1) printf("error on a file descriptor, see manpage for info (errno: %d)\n", errno);

        // triggers on new incoming connection
        if (FD_ISSET(master_socket_fd, &read_setd)) {
            new_socket = connect(master_socket_fd, (SA*)&addr, addr_size);

            // no errors on new socket
            if (!new_socket || new_socket == -1) {
                printf("error new socket (errno: %d)\n", errno);
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


    // After chatting close the socket
    close(master_socket_fd);
}

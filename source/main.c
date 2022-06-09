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

#include "pp4m/pp4m_io.h"
#include "pp4m/pp4m_net.h"

#define MAX 80
#define PORT 62443
#define SA struct sockaddr

// Function designed for chat between client and server.
void func(int connfd)
{
    char buff[MAX];
    int n;

    // infinite loop for chat
    while(1) {

        memset(buff, 0x00, MAX);

        // read the message from client and copy it in buffer
        read(connfd, buff, sizeof(buff));

        // print buffer which contains the client contents
        printf("From client: %s\t To client : ", buff);

        memset(buff, 0x00, MAX);

        n = 0;

        // copy server message in the buffer
        while ((buff[n++] = getchar()) != '\n') ;

        // and send that buffer to client
        write(connfd, buff, sizeof(buff));

        // if msg contains "Exit" then server exit and chat ended.
        if (strncmp("exit", buff, 4) == 0) {
            printf("Server Exit...\n");
            break;
        }
    }
}

// Driver function
int main(int argc, char *argv[]) {

    int sockfd, connfd, len;
    struct sockaddr_in servaddr, cli;

    if ((sockfd = pp4m_NET_Init(TCP)) < 0) {
        printf("socket creation failed...\n");
        exit(0);
    }

    printf("Socket successfully created..\n");

    memset(&servaddr, 0x00, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
        printf("socket bind failed...\n");
        exit(0);
    }

    printf("Socket successfully binded..\n");

    // Now server is ready to listen and verification
    if ((listen(sockfd, 5)) != 0) {
        printf("Listen failed...\n");
        exit(0);
    }

    printf("Server listening..\n");

    len = sizeof(cli);

    // Accept the data packet from client and verification
    connfd = accept(sockfd, (SA*)&cli, &len);
    if (connfd < 0) {
        printf("server accept failed...\n");
        exit(0);
    }

    printf("server accept the client...\n");

    // Function for chatting between client and server
    func(connfd);

    // After chatting close the socket
    close(sockfd);
}

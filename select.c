#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <netinet/in.h>
#include <netdb.h>

// web server program
int maxconnection = 100;

char buf[2048];
char inbuf[2048];
int sockfd, clisockfd;
fd_set fds, read_fds;


void abrt_handler(int sig) {
    printf("catch the signal. socket closed.\n");
    close(sockfd);
    exit(1);
}

void init_server(char* port) {
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    int nready, clients[FD_SETSIZE];

    int i, maxfd, maxi;


    printf("server starting\n");

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (signal(SIGINT, abrt_handler) == SIG_ERR) {
        exit(1);
    }

    if (sockfd == -1) {
        perror("socket");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(port));
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);


    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(sockfd);
        exit(1);
    }

    if (listen(sockfd, maxconnection) == -1) {
        perror("listen");
        close(sockfd);
        exit(1);
    }
    printf("waiting server connection ... \n");

    // http response message
    memset(buf, 0, sizeof(buf));
    sprintf(buf,
        "HTTP/1.0 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "\r\n"
        "HELLO\r\n"
    );


    // wait second
    struct timeval waitval;
    waitval.tv_sec = 2;
    waitval.tv_usec = 500;

    for (int i = 0; i < FD_SETSIZE; i++) {
        clients[i] = -1; /* -1 means i'th DS  is not connected */
    }

    FD_ZERO(&fds);
    FD_ZERO(&read_fds);

    // add socket FD to monitor target
    FD_SET(sockfd, &read_fds);

    maxfd = sockfd;
    maxi = -1;

    // socket monitor
    for ( ; ; ) {
        
        memcpy(&fds, &read_fds, sizeof(fd_set));

        nready = select(maxfd+1, &fds, NULL, NULL, &waitval);

        // server listen 
        if (FD_ISSET(sockfd, &fds)) {
            int clen = sizeof(client_addr);
            clisockfd = accept(sockfd, (struct sockaddr *)&client_addr, &clen);

            if (clisockfd < 0) {
                perror("accept() failed");
            }

            // 監視対象にセット
            FD_SET(clisockfd, &read_fds);

            for (i=0; i<FD_SETSIZE; i++) {
                if (clients[i] < 0) {
                    clients[i] = clisockfd;
                    break;
                }
            }
            if (clisockfd > maxfd) {
                maxfd = clisockfd;
            }
            if (i > maxi) {
                maxi = i;
            }
            if (nready < 0) {
                continue;
            }
        }
        
        for (i = 0; i <= maxi; i++) {
            int fdesc = clients[i];

            if (fdesc < 0) continue;

            // &fds is readable sockets
            if (FD_ISSET(fdesc, &fds)) {
                memset(inbuf, 0, sizeof(inbuf));
                // receive client data
                recv(fdesc, inbuf, sizeof(inbuf), 0);
                
                // write for client 
                send(fdesc, buf, (int)strlen(buf), 0);
                close(fdesc);
                FD_CLR(fdesc, &read_fds);
                clients[i] = -1;
                if (nready < 0) {
                    continue;
                }
            }
        }

    }

    close(sockfd);
    return;
}

void respond(int fdesc) {
}

int main(int c, char** v) {
    printf("start server with port: 80\n");
    init_server("80");
}

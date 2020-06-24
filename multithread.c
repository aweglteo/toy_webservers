#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <netinet/in.h>
#include <netdb.h>

#include <pthread.h>

// web server program
int maxconnection = 100;

char buf[2048];
char inbuf[2048];
int sockfd, clisockfd;

void abrt_handler(int sig) {
    printf("catch the signal. socket closed.\n");
    close(sockfd);
    exit(1);
}

// exec multithread
void* respond_http(void* arg) {

    pthread_detach(pthread_self());
    int sockfd = (int)arg;

    memset(inbuf, 0, sizeof(inbuf));
    recv(sockfd, inbuf, sizeof(inbuf), 0);
    printf("%s", inbuf);
    send(sockfd, buf, (int)strlen(buf), 0);
    close(sockfd);
}

void init_server(char* port) {
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    printf("server starting\n");

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (signal(SIGINT, abrt_handler) == SIG_ERR) {
        exit(1);
    }

    if (sockfd == -1) {
        perror("socket");
        exit(1);
    }

    printf("port: %d\n", atoi(port));
    printf("socketd: %d\n", sockfd);

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

    for (;;) {
        pthread_t worker;
        int clen = sizeof(client_addr);
        clisockfd = accept(sockfd, (struct sockaddr *)&client_addr, &clen);

        if (clisockfd < 0) {
            perror("accept() failed");
            exit(1);
        }
        printf("connection established\n");

        if ((pthread_create(worker, NULL, respond_http, clisockfd)) != 0) {
            perror("pthread");
        };
    }
    close(sockfd);
    return;
}


int main(int c, char** v) {
    printf("start server with port: 80\n");
    init_server("80");
}

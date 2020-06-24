#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <netinet/in.h>
#include <netdb.h>

#include <signal.h>


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

void catchSigchld(int signo) {
    pid_t child_pid = 0;
    printf("finish child process\n");
    do {
        int child_ret;
        child_pid = waitpid(-1, &child_ret, WNOHANG);
    } while (child_pid > 0);
}

void init_server(char* port) {
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    printf("server starting\n");

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (signal(SIGINT, abrt_handler) == SIG_ERR) {
        exit(1);
    }

    struct sigaction act;
    memset(&act, 0, sizeof(act)); 
    act.sa_handler = catchSigchld;
    act.sa_flags = SA_NOCLDSTOP | SA_RESTART;
    sigaction(SIGCHLD, &act, NULL);

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

    for (;;) {
        int clen = sizeof(client_addr);
        clisockfd = accept(sockfd, (struct sockaddr *)&client_addr, &clen);

        if (clisockfd < 0) {
            perror("accept() failed");
            exit(1);
        }
        printf("connection established\n");
        pid_t pid = fork();

        if (pid == 0) {
            // close listen socket in child-process
            close(sockfd);

            memset(inbuf, 0, sizeof(inbuf));
            recv(clisockfd, inbuf, sizeof(inbuf), 0);
            send(clisockfd, buf, (int)strlen(buf), 0);
            close(clisockfd);

            exit(1);
        } else {
            // close client socet in parent process
            close(clisockfd);
        };
    }
    close(sockfd);
    return;
}


int main(int c, char** v) {
    printf("start server with port: 80\n");
    init_server("80");
}

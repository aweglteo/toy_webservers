#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MSGSIZE 1024
#define BUFSIZE (MSGSIZE + 1)

int main(int argc, char* argv[]) {

    int sock;
    struct sockaddr_in servSockAddr;
    unsigned short servPort = 8889;
    char recvBuffer[BUFSIZE];
    char sendBuffer[BUFSIZE];

    memset(&servSockAddr, 0, sizeof(servSockAddr));
    servSockAddr.sin_family = AF_INET;

    if (inet_aton("127.0.0.1", &servSockAddr.sin_addr) == 0) {
        fprintf(stderr, "Invalid IP Address.\n");
        exit(EXIT_FAILURE);
    }

    servSockAddr.sin_port = htons(servPort);

    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0 ){
        perror("socket() failed.");
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr*) &servSockAddr, sizeof(servSockAddr)) < 0) {
        perror("connect() failed.");
        exit(EXIT_FAILURE);
    }

    printf("connect to %s\n", inet_ntoa(servSockAddr.sin_addr));

    while(1){
        int byteRcvd  = 0;
        int byteIndex = 0;
        while (byteIndex < MSGSIZE) {
            byteRcvd = recv(sock, &recvBuffer[byteIndex], 1, 0);
            if (byteRcvd > 0) {
                if (recvBuffer[byteIndex] == '\n'){
                    recvBuffer[byteIndex] = '\0';
                    if (strcmp(recvBuffer, "quit") == 0) {
                        close(sock);
                        return EXIT_SUCCESS;
                    } else {
                        break;
                    }
                }
                byteIndex += byteRcvd;
            } else if(byteRcvd == 0){
                perror("ERR_EMPTY_RESPONSE");
                exit(EXIT_FAILURE);
            } else {
                perror("recv() failed.");
                exit(EXIT_FAILURE);
            }
        }
        printf("server return: %s\n", recvBuffer);
    }

    return EXIT_SUCCESS;
}

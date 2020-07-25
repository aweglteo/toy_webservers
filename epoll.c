#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <signal.h>

#define MAX_CLIENT 100
#define SERVER_PORT 8889
#define Buff 1024

int listen_sock;
int clients[MAX_CLIENT];

static void setnonblocking(int sock)
{
    int flag = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flag | O_NONBLOCK);
}

// write to all clients
void write_to_all_crient(char* buffer) {
    for (int i=0; i < MAX_CLIENT; i++) {
        if (clients[i] > 0) {
            int client_ds = clients[i];
            send(client_ds, buffer, (int)strlen(buffer), 0);
        }
    }
}

// TODO: refactoring, able to using hash table
void delete_client(int client) {
    for (int i = 0; i < MAX_CLIENT; i++) {
        if (clients[i] == client) {
            clients[i] = -1;
            break;
        }
    }
}

void init_epollserver(int port) {
    printf("initing epoll web server %d\n", port);

    int writer_len;
    struct sockaddr_in reader_addr; 
    struct sockaddr_in writer_addr;
    char buf[Buff];
    
    // socket open
    if ((listen_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("reader:socket");
        exit(1);
    }

    bzero((char *) &reader_addr, sizeof(reader_addr));
    reader_addr.sin_family = PF_INET;
    reader_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    reader_addr.sin_port = htons(port);

    // socket bind
    if (bind(listen_sock, (struct sockaddr *)&reader_addr, sizeof(reader_addr)) < 0) {
        perror("reader: bind");
        exit(1);
    }

    // listen socket
    if (listen(listen_sock, 10) < 0) {
        perror("reader: listen");
        close(listen_sock);
        exit(1);
    }

    int conn_sock;
    int nfds;

    int epfd = epoll_create1(0);
    if (epfd == -1) {
        perror("epoll_create1");
        exit(1);
    }

    struct epoll_event ev;
    struct epoll_event events[MAX_CLIENT];
    ev.events = EPOLLIN;
    ev.data.fd = listen_sock;

    // register epfd
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listen_sock, &ev) == -1) {
        perror("epoll_ctl: listen_sock");
        exit(1);
    }

    for (;;) {
        nfds = epoll_wait(epfd, events, MAX_CLIENT, -1);
        for (int n = 0; n < nfds; n++) {
            // the case of listen socket being ready
            if (events[n].data.fd == listen_sock) {
                conn_sock = accept(listen_sock, (struct sockaddr *) &writer_addr, &writer_len);
                setnonblocking(conn_sock);
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = conn_sock;
                // add connection socket to epoll events
                epoll_ctl(epfd, EPOLL_CTL_ADD, conn_sock, &ev);
                // cache client socket's
                for (int i = 0; i< MAX_CLIENT; i++) {
                    if (clients[i] < 0) {
                        clients[i] = conn_sock;
                        break;
                    }     
                }
            } else {
                int client = events[n].data.fd;
                int r = read(events[n].data.fd, buf, sizeof(buf));
                if (r < 0) {
                    perror("read");
                    epoll_ctl(epfd, EPOLL_CTL_DEL, client, &ev);
                    close(client);
                    delete_client(client);
                } else if (r == 0) {
                    epoll_ctl(epfd, EPOLL_CTL_DEL, client, &ev);
                    close(client);
                } else {
                    printf("%s", buf);
                    write_to_all_crient(buf);
                }
            }
        }
    }
}

void abrt_handler(int sig) {
    printf("catch the signal. socket closed.\n");
    close(listen_sock);
    exit(1);
}

int main(int argc, char** argv)  {
    // if SIGINT signal received, close listensock and finished program
    if (signal(SIGINT, abrt_handler) == SIG_ERR) {
        exit(1);
    }

    // -1 means i'th DS  is not connected
    for (int i = 0; i < MAX_CLIENT; i++) {
        clients[i] = -1;
    }

    init_epollserver(SERVER_PORT);
}

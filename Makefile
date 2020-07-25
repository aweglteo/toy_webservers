CC = gcc

epoll: epoll.c
	$(CC) -Wall -O2 -o epoll epoll.c

sender: sender.c
	$(CC) -Wall -O2 -o sender sender.c

receiver: reciever.c
	$(CC) -Wall -O2 -o receiver.c
	

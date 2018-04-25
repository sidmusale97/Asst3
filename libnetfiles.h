#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#define PORT 8080
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <netdb.h>
#include <semaphore.h>

typedef struct fdNode{
	char * path;
	int fileMode;
	int serverfd;
	int clientfd;
	int openMode;
	struct fdNode * next;
}fdNode;

int netopen(char * pathname, int flags);
int netread(int fd, void * buf, size_t bytes);
int netwrite(int fd, void * buf, size_t bytes);
int netclose(int clientfd);
int netserverinit(char * hostname, int filemode);
int FileMode = 2;
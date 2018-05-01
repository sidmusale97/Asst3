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
#include <sys/time.h>
#define UNRESTRICTED 0
#define EXCLUSIVE 1
#define TRANSACTION 2

typedef struct fdNode{
	char * path;
	int fileMode;
	int serverfd;
	int clientfd;
	int openMode;
	struct fdNode * next;

}fdNode;

typedef struct QueueNode 
{
	int valid;
	time_t secs;
	pthread_t tid;
	char * path;
	int openMode;
	int fileMode;
	int ready;
	struct QueueNode * next;
}QueueNode;

int netopen(char * pathname, int flags);
int netread(int fd, void * buf, int bytes);
int netwrite(int fd, void * buf, size_t bytes);
int netclose(int clientfd);
int netserverinit(char * hostname, int filemode);
int FileMode;
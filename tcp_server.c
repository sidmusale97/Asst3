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

void echo(int client_socket);
int createServerSocket();
void * handleRequest(void * arg);
	
int main()
{
	int server_socket = createServerSocket();
	int * client_socket;
	pthread_t tid;

	while (1)
	{
	client_socket = (int *)malloc(sizeof(int));
	*client_socket = accept(server_socket, NULL, NULL);
	pthread_create(&tid, NULL,handleRequest,client_socket);
	}

return 0;
}

int createServerSocket()
{
	//create server socket
	int server_socket;
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == -1){
	puts("socket creation error");
	exit(1);
	}
	//define server address
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(PORT);
	server_address.sin_addr.s_addr = INADDR_ANY;

	//bind socket to address
	bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address));

	//determine how many connections can wait for a return
	listen(server_socket, 5);

	return server_socket;
}

void * handleRequest(void * arg)
{
	int client_socket = *((int *)arg);
	pthread_detach(pthread_self());
	free(arg);
	echo(client_socket);
	close(client_socket);
	return NULL;
}

void echo(int client_socket)
{
	char client_message[256] = {0};
	char server_message[256] = {0};
	read(client_socket, &client_message, 256);//receive client socket and client_message

	char dels[2] = ","; //delimeters for strtok
	char * cmessage = (char *)&client_message; 
	char * tok = strtok(cmessage,dels);	//tok holds the int sent at the front of client_message
	int funcID = atoi(tok);  
	if(funcID == 1){//if netopen

	tok = strtok(NULL, dels);//move on to next parameter which is file path
	char path[256]; //array to hold file path

	strcpy(path,tok);//copy file path into path array 
	tok = strtok(NULL, dels);//move to next parameter with is open mode
	int openMode = atoi(tok);

	//Attempt to open file
	int fd = open(path,openMode);
	if (fd < 0)
	{
		char * errDes = strerror(errno);
		sprintf(server_message, "%d", fd);
		strcat(server_message,",Error: ");
		strcat(server_message,errDes);
		write(client_socket,server_message, sizeof(server_message));	
	}
	else
	{
		sprintf(server_message, "%d", fd*-1);
		write(client_socket,server_message, sizeof(server_message));
	}
	}
}






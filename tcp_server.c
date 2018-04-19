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

typedef struct fdNode{
	int serverfd;
	int clientfd;
	struct fdNode * next;
	int openMode;
}fdNode;


int createServerSocket();
void * handleRequest(void * arg);
void insertfdNode(fdNode * node);
int getFreeClientfd();
fdNode * get_Node_from_cfd(int clientfd);
void deletefdNode(int clientfd);
void handleRead(char * cmessage, int client_socket);
void handleOpen(char * cmessage, int client_socket);
void handleWrite(char * cmessage, int client_socket);
void handleClose(char * cmessage, int client_socket);

fdNode * allfds = NULL;
	
int main()
{
	int server_socket = createServerSocket();
	int * client_socket;
	pthread_t tid;

	while (1)
	{
	client_socket = (int *)malloc(sizeof(int));
	*client_socket = accept(server_socket, NULL, NULL);
	pthread_create(&tid,NULL,handleRequest,client_socket);
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
	char client_message[256] = {0};
	char server_message[256] = {0};
	read(client_socket, &client_message, 256);//receive client socket and client_message

	char dels[2] = ","; //delimeters for strtok
	char * cmessage = (char *)&client_message; 
	char funcID = cmessage[0];  
	if(funcID == '1')handleOpen(cmessage, client_socket);//if netopen
	else if(funcID == '2')handleRead(cmessage, client_socket); //if netread
	else if(funcID == '3')handleClose(cmessage,client_socket); //if netclose
	else if(funcID == '4')handleWrite(cmessage, client_socket); //if netwrite
	close(client_socket);
	return NULL;
}

void insertfdNode(fdNode * node)
{
	if (allfds == NULL)
	{
		allfds = node;
	}
	else
	{
		fdNode * ptr = allfds;
		while(ptr->next != NULL)
		{
			ptr = ptr->next;
		}
		ptr->next = node;
	}
}

int getFreeClientfd()
{
	int i = -2;//intial value of client fd;
	fdNode * ptr = allfds;
	while(ptr != NULL)
	{
		if(ptr->clientfd == i)i--;
		ptr = ptr->next;
	}
	return i;
}

fdNode * get_Node_from_cfd(int clientfd)
{
	fdNode * ptr = allfds;
	while(ptr->clientfd != clientfd)
	{
		ptr = ptr->next;
	}
	return ptr;
}

void deletefdNode(int clientfd)
{
	fdNode * temp = allfds;
	fdNode * prev = NULL;
	while(temp != NULL)
	{
		if(temp->clientfd == clientfd)
			{
				if(temp == allfds)
				{
					allfds = allfds->next;
					free(temp);
				}
				else
				{
					prev->next = temp->next;
					free(temp);
				}
			}
		prev = temp;
		temp = temp->next;
	}
}

void handleOpen(char * cmessage, int client_socket)
{
	char server_message[256] = {0};

	char dels[2] = ","; //delimeters for strtok 

	char * tok = strtok(cmessage,dels);	//tok holds the int sent at the front of client_message

	tok = strtok(NULL, dels);//move on to next parameter which is file path
	char path[256]; //array to hold file path

	strcpy(path,tok);//copy file path into path array 
	tok = strtok(NULL, dels);//move to next parameter with is open mode
	int openMode = atoi(tok);
	int o;
	if (openMode == 0) o = O_RDONLY;
	else if (openMode == 1)o = O_WRONLY;
	else if (openMode == 2)o = O_RDWR;

	//Attempt to open file
	int fd = open(path,o);
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
		fdNode * newNode = (fdNode*)malloc(sizeof(fdNode));
		newNode->serverfd = fd;
		newNode->clientfd = getFreeClientfd();
		newNode->next = NULL;
		insertfdNode(newNode);
		sprintf(server_message, "%d",newNode->clientfd);
		write(client_socket,server_message, sizeof(server_message));
	}
}

void handleRead(char * cmessage, int client_socket)
{
	char server_message[256] = {0};
	char dels[2] = ","; //delimeters for strtok 

	char * tok = strtok(cmessage,dels);	//tok holds the int sent at the front of client_message
	tok = strtok(NULL, dels);//move on to next parameter which is clientfd
	int clientfd = atoi(tok);
	tok = strtok(NULL, dels);
	int bytesRequested = atoi(tok);

	fdNode * temp = get_Node_from_cfd(clientfd);
	int serverfd = temp->serverfd;
	char buffer[bytesRequested+1];

	int readFile = read(serverfd,buffer,bytesRequested);
	if(readFile < 0)
	{
		char * errDes = strerror(errno);
		sprintf(server_message, "%d", readFile);
		strcat(server_message,",Error: ");
		strcat(server_message,errDes);
		write(client_socket,server_message, sizeof(server_message));
	}
	else
	{	
		sprintf(server_message,"%d", readFile);
		strcat(server_message,",");
		if(readFile == bytesRequested)buffer[bytesRequested] = '\0';
		else{
			buffer[readFile] = '\0';
		}
		strcat(server_message,buffer);
		write(client_socket,server_message, sizeof(server_message));
	}
}

void handleClose(char * cmessage, int client_socket)
{
	char server_message[256] = {0};
	char dels[2] = ","; //delimeters for strtok 

	char * tok = strtok(cmessage,dels);	//tok holds the int sent at the front of client_message
	tok = strtok(NULL, dels);//move to next parameter which is clientfd
	int clientfd = atoi(tok);
	fdNode * temp = get_Node_from_cfd(clientfd);
	int serverfd = temp->serverfd;
	int closeFile = close(serverfd);
	if (closeFile < 0)
		{
		char * errDes = strerror(errno);
		sprintf(server_message, "%d", closeFile);
		strcat(server_message,",Error: ");
		strcat(server_message,errDes);
		write(client_socket,server_message, sizeof(server_message));
		}
	else
		{
		deletefdNode(clientfd);
		sprintf(server_message, "%d", closeFile);
		write(client_socket,server_message,sizeof(server_message));	
		}
}

void handleWrite(char * cmessage, int client_socket)
{
	char server_message[256] = {0};
	char dels[2] = ","; //delimeters for strtok 

	char * tok = strtok(cmessage,dels);	//tok holds the int sent at the front of client_message
	char buffer[256];
	tok = strtok(NULL, dels);//move on to next parameter which is clientfd
	int clientfd = atoi(tok);
	tok = strtok(NULL, dels);
	strcpy((char *)&buffer,tok);
	tok = strtok(NULL, dels);
	int bytesToWrite = atoi(tok);

	fdNode * temp = get_Node_from_cfd(clientfd);
	int serverfd = temp->serverfd;
	
	int writeFile = write(serverfd,(void *)&buffer,bytesToWrite);
	if(writeFile < 0)
	{
		char * errDes = strerror(errno);
		sprintf(server_message, "%d", -1);
		strcat(server_message,",Error: ");
		strcat(server_message,errDes);
		write(client_socket,server_message, sizeof(server_message));
	}
	else
	{	
		sprintf(server_message,"%d", writeFile);
		write(client_socket,server_message, sizeof(server_message));
	}
}
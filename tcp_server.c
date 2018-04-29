#include "libnetfiles.h"


int createServerSocket();
void * handleRequest(void * arg);
void insertfdNode(fdNode * node);
int getFreeClientfd();
fdNode * get_Node_from_cfd(int clientfd);
fdNode * get_Nodes_from_path(char * path, fdNode * start);
void deletefdNode(int clientfd);
void handleRead(char * cmessage, int client_socket);
void handleOpen(char * cmessage, int client_socket);
void handleWrite(char * cmessage, int client_socket);
void handleClose(char * cmessage, int client_socket);
void cleanLL();

pthread_mutex_t mutex;
sem_t socketsemaphore;
fdNode * allfds = NULL;
	
int main()
{
	int server_socket = createServerSocket();
	int * client_socket;
	pthread_t tid;
	sem_init(&socketsemaphore, 0, 10);
	pthread_mutex_init(&mutex, NULL);
	while (1)
	{
	client_socket = (int *)malloc(sizeof(int));
	sem_wait(&socketsemaphore);
	*client_socket = accept(server_socket, NULL, NULL);
	pthread_create(&tid,NULL,handleRequest,client_socket);
	}
	cleanLL();
	pthread_mutex_destroy(&mutex);
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
	char client_message[4000] = {0};
	char server_message[256] = {0};
	read(client_socket, &client_message, 4000);//receive client socket and client_message
	char dels[2] = ","; //delimeters for strtok
	char * cmessage = (char *)&client_message; 
	char funcID = cmessage[0];  
	if(funcID == '1')handleOpen(cmessage, client_socket);//if netopen
	else if(funcID == '2')handleRead(cmessage, client_socket); //if netread
	else if(funcID == '3')handleClose(cmessage,client_socket); //if netclose
	else if(funcID == '4')handleWrite(cmessage, client_socket); //if netwrite	
	close(client_socket);
	sem_post(&socketsemaphore);
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
	if (allfds == NULL)return NULL; 
	while((ptr->clientfd != clientfd)  && ptr != NULL)
	{
		ptr = ptr->next;
	}
	return ptr;
}

	fdNode * get_Nodes_from_path(char * path, fdNode * start){
	fdNode * ptr = start;
	if (start == NULL)return NULL;
	while((strcmp(path,ptr->path) != 0) && ptr != NULL)
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
					free(temp->path);
					free(temp);
					return;
				}
				else
				{
					prev->next = temp->next;
					free(temp->path);
					free(temp);
					return;
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
	char * path = (char *)malloc(256); //array to hold file path

	strcpy(path,tok);//copy file path into path array 
	tok = strtok(NULL, dels);//move to next parameter with is open mode
	int openMode = atoi(tok);

	tok = strtok(NULL, dels);//move to next parameter with is file mode
	int fileMode = atoi(tok);
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
		fdNode * temp = get_Nodes_from_path(path,allfds);
		if ((fileMode == 2 &&  temp != NULL) || (temp->fileMode == 2))//if transaction mode and file is opened in another client
		{
		sprintf(server_message, "%d", -1);
		strcat(server_message,",Error: Invalid permission in transcation mode. File is opened in another client");
		write(client_socket,server_message, sizeof(server_message));
		return;	
		}
		if(fileMode == 1 && temp != NULL)
		{
			while(temp != NULL)
			{
				if(temp->openMode == O_WRONLY || temp->openMode == O_RDWR)
				{
					sprintf(server_message, "%d", -1);
					strcat(server_message,",Error: Invalid permission in exclusive mode. File is opened with write permission in another client");
					write(client_socket,server_message, sizeof(server_message));
					return;
				}
				temp = get_Nodes_from_path(path,temp->next);
			}
		}	
		fdNode * newNode = (fdNode*)malloc(sizeof(fdNode));
		newNode->path = path;
		newNode->fileMode = fileMode;
		newNode->serverfd = fd;
		newNode->clientfd = getFreeClientfd();
		newNode->openMode = openMode;
		newNode->next = NULL;
		pthread_mutex_lock(&mutex);
		insertfdNode(newNode);
		pthread_mutex_unlock(&mutex);
		sprintf(server_message, "%d",newNode->clientfd);
		write(client_socket,server_message, sizeof(server_message));
	}
}

void handleRead(char * cmessage, int client_socket){
	char server_message[256] = {0};
	char dels[2] = ","; //delimeters for strtok 

	char * tok = strtok(cmessage,dels);	//tok holds the int sent at the front of client_message
	tok = strtok(NULL, dels);//move on to next parameter which is clientfd
	int bytesRequested = atoi(tok);
	tok = strtok(NULL, dels);
	int clientfd = atoi(tok);
	fdNode * temp = get_Node_from_cfd(clientfd);
	if (temp == NULL)
	{
		sprintf(server_message, "%d", -1);
		strcat(server_message,",Error: Bad File Descriptor");
		write(client_socket,server_message, sizeof(server_message));
	}
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
		sprintf(server_message,"%d,", readFile);
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
	if (temp == NULL)
	{
		sprintf(server_message, "%d", -1);
		strcat(server_message,",Error: Bad File Descriptor");
		write(client_socket,server_message, sizeof(server_message));
	}
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
		pthread_mutex_lock(&mutex);
		deletefdNode(clientfd);
		pthread_mutex_unlock(&mutex);
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
	int bytesToWrite = atoi(tok);
	tok = strtok(NULL, dels);
	strcpy((char *)&buffer,tok);

	fdNode * temp = get_Node_from_cfd(clientfd);
	if (temp == NULL)
	{
		sprintf(server_message, "%d", -1);
		strcat(server_message,",Error: Bad File Descriptor");
		write(client_socket,server_message, sizeof(server_message));
	}
	int serverfd = temp->serverfd;
	pthread_mutex_lock(&mutex); //if unrestricted mode lock to prevent race condition
	int writeFile = write(serverfd,(void *)buffer,bytesToWrite);
	pthread_mutex_unlock(&mutex);
	if(writeFile != bytesToWrite)
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

void cleanLL()
{	
	fdNode * temp = allfds;
	fdNode * next = allfds->next;
	while(temp != NULL)
	{
		close(temp->serverfd);
		free(temp->path);
		free(temp);	
		temp = next;
	}
}

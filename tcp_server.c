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
void * monitorThread(void * arg);
void insertQueueNode(QueueNode * node); 
void removeQueueNode(QueueNode * node);
pthread_cond_t cond;
QueueNode * queue = NULL;

pthread_mutex_t insertlock;
pthread_mutex_t deletelock;
pthread_mutex_t queuelock;



fdNode * allfds = NULL;
	
int main()
{
	struct timeval lastTime,currentTime;
	int server_socket = createServerSocket();
	int * client_socket;
	pthread_t tid,monitor;
	pthread_mutex_init(&insertlock, NULL);
  	pthread_mutex_init(&deletelock, NULL);
  	pthread_mutex_init(&queuelock, NULL);
  	gettimeofday(&lastTime, NULL);
	while (1)
	{
	gettimeofday(&currentTime,NULL);
	if (currentTime.tv_sec - lastTime.tv_sec >= 3.0)
	{
		pthread_create(&monitor,NULL,monitorThread,NULL);
		lastTime.tv_sec = currentTime.tv_sec;
    	lastTime.tv_usec = currentTime.tv_usec;
	}
	client_socket = (int *)malloc(sizeof(int));
	*client_socket = accept(server_socket, NULL, NULL);
	pthread_create(&tid,NULL,handleRequest,client_socket);
	}
	pthread_mutex_destroy(&insertlock);
    pthread_mutex_destroy(&deletelock);
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
	listen(server_socket, 10);

	return server_socket;
}

void * handleRequest(void * arg)
{
	int client_socket = *((int *)arg);
	pthread_detach(pthread_self());
	free(arg);
	char client_message[4100] = {0};
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
	
	return NULL;
}

void * monitorThread(void * arg)
{
	QueueNode * ptr = queue;
	while(ptr != NULL)
		{
			struct timeval ctime;
			gettimeofday(&ctime,NULL);
			if(ctime.tv_sec - ptr->secs >= 2)
			{
				ptr->valid = 0;
			}
		}
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

void insertQueueNode(QueueNode * node)
{
	if (queue == NULL)
	{
		queue = node;
	}
	else
	{
		QueueNode * ptr = queue;
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
	fdNode * temp = get_Nodes_from_path(path,allfds);
	if (temp != NULL)//if transaction mode and file is opened in another client
	{
   if(temp->fileMode == 2 || fileMode == 2)
    { 
    	struct timeval ctime;
    	gettimeofday(&ctime, NULL);
		QueueNode * node = (QueueNode *)malloc(sizeof(QueueNode));
		node->secs = ctime.tv_sec;			
		node->valid = 1;
		node->tid = pthread_self();
		node->path = path;
		node->openMode = openMode;
		node->fileMode = fileMode;
		node->ready = 0;
		pthread_mutex_lock(&insertlock);
		insertQueueNode(node);
		pthread_mutex_unlock(&insertlock);
		while(node->ready == 0 && node->valid)
		{
			sleep(1);
		}
      	if(node->valid == 0)
      	{
      	removeQueueNode(node);
      	errno = EWOULDBLOCK;
      	char * errDes = strerror(errno);
		sprintf(server_message, "%d", ETIMEDOUT);
		strcat(server_message,",Error: ");
		strcat(server_message,errDes);
		write(client_socket,server_message, sizeof(server_message));
		return;	
      	}
      	removeQueueNode(node);
	  }
	  else if(openMode == O_WRONLY || openMode == O_RDWR)
	  {
	  	QueueNode * node = (QueueNode *)malloc(sizeof(QueueNode));
		while(temp != NULL)
		{
			if(fileMode == 1 && temp->openMode != O_RDONLY)
				{
				struct timeval ctime;
    			gettimeofday(&ctime, NULL);
				node->secs = ctime.tv_sec;
				node->valid = 1;
				node->tid = pthread_self();
				node->path = path;
				node->openMode = openMode;
				node->fileMode = fileMode;
				node->ready = 0;
				pthread_mutex_lock(&insertlock);
				insertQueueNode(node);
				pthread_mutex_unlock(&insertlock);
				break;
			}
			temp = get_Nodes_from_path(path,temp->next);
		}
		while(node->ready == 0 && node->valid)
	       {
			  sleep(1);
		   }
		 if(node->valid == 0)
      	{
      		removeQueueNode(node);
      		errno = EWOULDBLOCK;
      		char * errDes = strerror(errno);
			sprintf(server_message, "%d", ETIMEDOUT);
			strcat(server_message,",Error: ");
			strcat(server_message,errDes);
			write(client_socket,server_message, sizeof(server_message));	
      	}
       		removeQueueNode(node);
	}
 }
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
		fdNode * newNode = (fdNode*)malloc(sizeof(fdNode));
		newNode->path = path;
		newNode->fileMode = fileMode;
		newNode->serverfd = fd;
		newNode->clientfd = getFreeClientfd();
		newNode->openMode = openMode;
		newNode->next = NULL;
		pthread_mutex_lock(&insertlock);
		insertfdNode(newNode);
		pthread_mutex_unlock(&insertlock);
		sprintf(server_message, "%d",newNode->clientfd);
		write(client_socket,server_message, sizeof(server_message));
	}
}

void handleRead(char * cmessage, int client_socket){
	
	char dels[2] = ","; //delimeters for strtok 

	char * tok = strtok(cmessage,dels);	//tok holds the int sent at the front of client_message
	tok = strtok(NULL, dels);//move on to next parameter which is clientfd
	int bytesRequested = atoi(tok);
	tok = strtok(NULL, dels);
	int clientfd = atoi(tok);
  char server_message[bytesRequested + 10];
	char buffer[bytesRequested+1];
	fdNode * temp = get_Node_from_cfd(clientfd);
	if (temp == NULL)
	{
		sprintf(server_message, "%d", -1);
		strcat(server_message,",Error: Bad File Descriptor");
		write(client_socket,server_message, sizeof(server_message));
	}
	int serverfd = temp->serverfd;

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
		if (!isEmpty())
		{
			QueueNode * ptr = queue;
			while (ptr != NULL)
				{
					if(strcmp(ptr->path,temp->path))
					{
						ptr->ready = 1;
						break;
					}
					ptr = ptr->next;
				}
		}
		pthread_mutex_lock(&deletelock);
		deletefdNode(clientfd);
		pthread_mutex_unlock(&deletelock);
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
	pthread_mutex_lock(&deletelock); //if unrestricted mode lock to prevent race condition
	int writeFile = write(serverfd,(void *)buffer,bytesToWrite);
	pthread_mutex_unlock(&deletelock);
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

int isEmpty()
{
	if (queue == NULL)
		return 1;
	else
		return 0;
}

void removeQueueNode(QueueNode * node)
{
	QueueNode * ptr = queue;
	QueueNode * prev = NULL;
	while(ptr != NULL){
	if(ptr == node)
	{
		if(ptr->next == NULL && prev != NULL)
		{
		free(ptr->path);
		free(ptr);
    	ptr = NULL; 
		prev->next = NULL;
		break;
		}
    else if(ptr->next != NULL && ptr == queue)
    {
    queue = queue->next;
    free(ptr->path);
		free(ptr);
     break;
    }
    else if(ptr->next == NULL && ptr == queue)
    {
    queue = NULL;
    free(ptr->path);
		free(ptr);
     break;
    }
		else if(ptr->next != NULL && prev != NULL)
		{
		prev->next = ptr->next->next;
		free(ptr->path);
		free(ptr);
		break;
		}
	}
	else
		{
			prev = ptr;
			ptr = ptr->next;
		}
}
}

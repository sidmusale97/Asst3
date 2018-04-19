#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#define PORT 8080
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
int netopen(char * pathname, int flags);
int netread(int fd, void * buf, size_t bytes);
int createSocket();

int main(int argc, char ** argv)
{
	if (argc != 3)
	{
		puts("Error please input only 1 arguement");
		exit(0);
	}
	else{
	char a[100] = "hello my name is sid";
	int openType = atoi(argv[2]);
	int file = netopen(argv[1], openType);
	printf("File Descriptor from server:\n%d\n", file);
	int s = netread(file,(void *)&a,10);
	printf("Bytes Read: %d,%s\n",s,a);
	s = netread(file,(void *)&a,10);
	printf("Bytes Read: %d,%s\n",s,a);
	int close = netclose(file);
	file = netopen(argv[1], openType);
	printf("File Descriptor from server:\n%d\n", file);
	s = netread(file,(void *)&a,30);
	printf("Bytes Read: %d,%s\n",s,a);
		
	}
	
}

int netopen(char * pathname, int flags)
{
	//create client socket
	int netsocket = createSocket();
	if(netsocket == -1)
	{
		return -1;
	}
	char buffer[256] = {0};


	buffer[0] = '1';	//specifies netopen
	strcat(buffer,",");
	strcat(buffer,pathname);
	sprintf(buffer,"%s,%d",buffer,flags);
	write(netsocket,buffer, sizeof(buffer));
	memset(buffer,0,sizeof(buffer));
	read(netsocket, buffer, sizeof(buffer));
	char * server_response = strtok(buffer,",");
	int num = atoi(server_response);
	if(num == -1){
	server_response = strtok(NULL,",");
	puts(server_response);
	exit(0);
	}
	return num;
}

int netread(int fd, void * buf, size_t bytes)
{
	if(bytes == 0)return 0;
	int clientfd = fd;
	int netsocket = createSocket();
	char client_message[256];
	char server_response[256];


	client_message[0] = '2';	//specifies netread
	sprintf(client_message, "%s,%d", client_message, clientfd);
	sprintf(client_message, "%s,%d", client_message, (int)bytes);

	write(netsocket, client_message, sizeof(client_message));
	read(netsocket, server_response, sizeof(server_response));
	
	char * tok = strtok(server_response, ",");
    int bytesRead = atoi(tok);
    tok = strtok(NULL, ",");
    memset(buf,0,strlen((char *)buf));
	strcpy((char *)buf,tok);
	if (bytesRead < 0)
	{
		puts(tok);
		buf = NULL;
		exit(0);
	}
	return bytesRead;

}

int netwrite(int fd, void * buf, size_t bytes)
{
	if(bytes == 0)return -1;
	int clientfd = fd;
	int netsocket = createSocket();
	char client_message[256];
	char server_response[256];

	sprintf(client_message, "4,%d,",clientfd);
	strcat(client_message,(char *)buf);
	sprintf(client_message, "%s,%d", client_message, (int)bytes);
	puts(client_message);
	write(netsocket, client_message, sizeof(client_message));
	read(netsocket, server_response, sizeof(server_response));

	char * tok = strtok(server_response, ",");
    int bytesRead = atoi(tok);
     tok = strtok(NULL, ",");
    memset(buf,0,strlen((char *)buf));
	strcpy((char *)buf,tok);
	if (bytesRead < 0)
	{
		puts(tok);
		buf = NULL;
		exit(0);
	}
}

int netclose(int clientfd)
{
	int netsocket = createSocket();
	char client_message[256];
	char server_response[256];


	client_message[0] = '3';	//specifies netread
	sprintf(client_message, "%s,%d", client_message, clientfd);
	write(netsocket, client_message, sizeof(client_message));
	read(netsocket, server_response, sizeof(server_response));

	char * tok = strtok(server_response, ",");
    int closeFile = atoi(tok);
	if (closeFile < 0)
	{
		tok = strtok(NULL, ",");
		puts(tok);
		exit(0);
	}
	return closeFile;

}
int createSocket()
{
	//create socket
	int netsocket;
	
	netsocket = socket(AF_INET, SOCK_STREAM, 0);
	if (netsocket == -1)
	{
	puts("socket creation error");
	return -1;
	}

	//specify socket address
	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_port = htons(PORT);
	address.sin_addr.s_addr = INADDR_ANY;

	//attempt socket connection
	int connection_status = connect(netsocket, (struct sockaddr *) &address, sizeof(address));
	if (connection_status == -1)
	{
	puts("Error in socket connection");
	return -1;
	}
	return netsocket;
}


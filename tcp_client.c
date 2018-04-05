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
	if (argc != 2)
	{
		puts("Error too many input args");
		exit(0);
	}
	else{
	int file = netopen(argv[1], O_RDONLY);
	printf("File Descriptor from server:\n%d\n", file);
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
	}
	return num;
}

int netread(int fd, void * buf, size_t bytes)
{
	int serverfd = fd * -1;
	int netsocket = createSocket();
	char client_message[256];

	char buffer[256] = {0};
	char server_response[256];



	client_message[0] = '2';	//specifies netread
	client_message[1] = ',';
	sprintf(client_message, "%s,%d", client_message, serverfd);
	sprintf(client_message, "%s,%d", client_message, (int)bytes);

	send(netsocket, client_message, sizeof(client_message), 0);
	recv(netsocket, buffer, sizeof(buffer), 0);
	return 0;

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


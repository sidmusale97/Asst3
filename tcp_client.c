#include "libnetfiles.h"

int createSocket();

//struct in_addr * serveraddress;
struct in_addr * saddress;
int main(int argc, char ** argv)
{
	//int sn = netserverinit("cp.cs.rutgers.edu");
	//if(sn == -1)
	//{
	//	exit(0);
	//}	
	char a[21] = "hello my name is sid";
	char b[50] = {0};
	int openType = O_RDWR;
	int file = netopen("hey.txt", openType);
	printf("File Descriptor from server:\n%d\n", file);
	int written = netwrite(file,(void *)&a,20);
	printf("Bytes written: %d\n",written);
	//int closeFile = netclose(file);
	//printf("File closed: %d\n", closeFile);
	int file2 = netopen("hey.txt", openType);
	printf("File Descriptor from server:\n%d\n", file2);
	int readFile = netread(file2, (void *)&b, 50);
	puts(b);
	return 0;
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
	sprintf(buffer,"%s,%d",buffer,FileMode);
	write(netsocket,buffer, sizeof(buffer));
	memset(buffer,0,sizeof(buffer));
	read(netsocket, buffer, sizeof(buffer));
	char * server_response = strtok(buffer,",");
	int num = atoi(server_response);
	if (num == ETIMEDOUT)
	{
		errno = ETIMEDOUT;
		perror("Error");
		return -1;
	}
	else if(num == -1){
	server_response = strtok(NULL,",");
	puts(server_response);
	exit(0);
	}
	return num;
}

int netread(int fd, void * buf, int bytes)
{
	if(bytes == 0)return 0;
	else if(fd == -1)
	{
		puts("Error: Bad File Descriptor");
		exit(0);
	}
	int clientfd = fd;
	int netsocket = createSocket();
	char client_message[256];

	char server_response[256];
	
	client_message[0] = '2';	//specifies netread
	sprintf(client_message, "%s,%d", client_message, (int)bytes);
	sprintf(client_message, "%s,%d", client_message, clientfd);
	
	write(netsocket, client_message, sizeof(client_message));
	read(netsocket, server_response, sizeof(server_response));
	char * tok = strtok(server_response, ",");
    int bytesRead = atoi(tok);
    if(bytesRead == 0)
    	{
    		buf = NULL;
    		return 0;
    	}
    tok = strtok(NULL, ",");
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
	else if(fd == -1)
	{
		puts("Error: Bad File Descriptor");
		return -1;
	}
	int clientfd = fd;
	int netsocket = createSocket();
	char client_message[4100];
	char server_response[256];


	sprintf(client_message, "4,%d", clientfd);
	sprintf(client_message, "%s,%d,", client_message, (int)bytes);
	strcat(client_message,(char *)buf);
	write(netsocket, client_message, sizeof(client_message));
	read(netsocket, server_response, sizeof(server_response));

	char * tok = strtok(server_response, ",");
    int bytesWritten = atoi(tok);
 	if (bytesWritten < 0)
	{
		tok = strtok(NULL, ",");
		puts(tok);
		buf = NULL;
		exit(0);
	}
	return bytesWritten;
}

int netclose(int clientfd)
{
	if(clientfd == -1)
	{
		puts("Error: Bad File Descriptor");
		exit(0);
	}
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

 int netserverinit(char * hostname, int filemode){
	struct hostent * serverinfo;
 if(filemode > 2)
 {
 puts("Error: Invalid File Mode");
 exit(0);
 }
	FileMode = filemode;
  
	if((serverinfo = gethostbyname(hostname)) == NULL)
	{
		h_errno = HOST_NOT_FOUND;
		herror("Error");
		return -1;
	}
	char * temp = (char *)malloc(serverinfo->h_length);
	strcpy(temp, serverinfo->h_addr);
	saddress = (struct in_addr *)temp;
	return 0;

}
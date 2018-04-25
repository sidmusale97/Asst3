CC = gcc

server: tcp_server.c
	$(CC) -o server tcp_server.c -lpthread
client: tcp_client.c
	 $(CC) -o client tcp_client.c

clean:
	rm server client

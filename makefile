CC = gcc

server: netfileserver.c
	$(CC) -o server netfileserver.c -lpthread
client: libnetfiles.c
	$(CC) -o client libnetfiles.c
clean:
	rm server client

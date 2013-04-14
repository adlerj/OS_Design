all: client server

client: client.c
	gcc -g  client.c -D_FILE_OFFSET_BITS=64 -lfuse -o client

server: server.c
	gcc -g  server.c -o server -pthread -D_FILE_OFFSET_BITS=64

clean:
	rm server client

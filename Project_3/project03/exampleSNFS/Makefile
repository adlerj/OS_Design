CC = gcc

CFLAG = -Wall `pkg-config fuse --cflags --libs`

target : fuse_rpc_example_server fuse_rpc_example_client 

fuse_rpc_example_client:fuse_rpc_example_client.c
	$(CC) $(CFLAG) -o $@ $<

fuse_rpc_example_server:fuse_rpc_example_server.c
	$(CC) $(CFLAG) -o $@ $<

clean:
	rm fuse_rpc_example_server fuse_rpc_example_client

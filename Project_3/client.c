/*OPCODES
0 -> create
1 -> open
2 -> close
3 -> truncate
4 -> getattr

5-> read
6 -> write
7 -> opendir
8 -> readdir
9 -> releasedir
10 -> mkdir
*/

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static const char *hello_str = "Hello World!\n";
static const char *hello_path = "/hello";

char *hostname;
int port;

int open_clientfd() 
{
    int clientfd;
    struct hostent *hp;
    struct sockaddr_in serveraddr;

    if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    return -1; /* check errno for cause of error */

    /* Fill in the server's IP address and port */
    if ((hp = gethostbyname("localhost")) == NULL)
    return -2; /* check h_errno for cause of error */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)hp->h_addr_list[0], 
      (char *)&serveraddr.sin_addr.s_addr, hp->h_length);
    serveraddr.sin_port = htons(port);

    /* Establish a connection with the server */
    if (connect(clientfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
    return -1;
    return clientfd;
}

static int client_getattr(const char *path, struct stat *stbuf)
{
    /*RESULT CODES
    0 -> failed
    1 -> success
    */
    
    uint16_t op = 4;
    op = htons(op);
    int res = 0;
    char *send_request = calloc(1024, sizeof(char));
    char *recv_data = calloc(sizeof(struct stat) + 2, sizeof(char));
    printf("PATH: %s\n", path);
    memcpy(send_request, &op, 2);
    memcpy(&send_request[2], path, strlen(path));

    int clientfd = open_clientfd();

    send(clientfd, send_request, strlen(path)+2, 0); 

    recv(clientfd, recv_data, sizeof(struct stat)+2, 0);

    uint16_t result;
    memcpy(&result, recv_data, 2);
    result = ntohs(result);
    memset(stbuf, 0, sizeof(struct stat));
    if(result == 0)
    {
        res = -ENOENT;
    }
    else if(result == 1)
    {
        memset(stbuf, 0, sizeof(struct stat));
        memcpy(stbuf, &recv_data[2], sizeof(struct stat));
    }

    free(send_request);
    free(recv_data);
    

    /*
    int res = 0;
    if(strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    }
    else if(strcmp(path, hello_path) == 0) {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = strlen(hello_str);
    }
    else
        res = -ENOENT;
    */
    return res;
}

static int client_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi)
{
    (void) offset;
    (void) fi;

    if(strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    filler(buf, hello_path + 1, NULL, 0);

    return 0;
}

static int client_open(const char *path, struct fuse_file_info *fi)
{

    uint16_t op = 1;
    op = htons(op);
    int flags = fi->flags;
    flags = htons(flags);

    int res = 0;
    char *send_request = calloc(1024, sizeof(char));
    char *recv_data = calloc(sizeof(struct stat) + 2, sizeof(char));
    printf("PATH: %s\n", path);
    memcpy(send_request, &op, 2);
    memcpy(&send_request[2], &flags, sizeof(int));
    memcpy(&send_request[2+sizeof(int)], path, strlen(path));
    
    int clientfd = open_clientfd();

    send(clientfd, send_request, strlen(path)+2, 0); 
    recv(clientfd, recv_data, 4, 0);

    uint16_t result;
    int access;

    memcpy(&result, recv_data, 2);
    memcpy(&access, &recv_data[2], 2);

    result = ntohs(result);
    access = ntohs(access);

    if(result == 0)
    {
        res = -ENOENT;
    }
    else if(result == 1)
    {
        if((access & 3) != O_RDONLY)
        {
            res = -EACCESS;
        }
    }

    free(send_request);
    free(recv_data);

    /*
    if(strcmp(path, hello_path) != 0)
        return -ENOENT;

    if((fi->flags & 3) != O_RDONLY)
        return -EACCES;
    */
    return res;
}

static int client_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
    size_t len;
    (void) fi;
    if(strcmp(path, hello_path) != 0)
        return -ENOENT;

    len = strlen(hello_str);
    if (offset < len) {
        if (offset + size > len)
            size = len - offset;
        memcpy(buf, hello_str + offset, size);
    } else
        size = 0;

    return size;
}

static struct fuse_operations client_oper = {
    .getattr    = client_getattr,
    .readdir    = client_readdir,
    .open   = client_open,
    .read   = client_read,
};

int main(int argc, char *argv[])
{
    /* Hardcoded to localhost in clientfd
    hostname = calloc(1024, sizeof(char));
    strcpy(hostname, argv[1]);
    */
    port = 8801;
    return fuse_main(argc, argv, &client_oper, NULL);
}

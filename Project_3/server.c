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

//Mount
char mount[1024];

//Function prototypes
int open_listenfd(int port);
void *handle_request(void *arg);
void server_getatrr(char *buffer, int clientfd);
void server_open(char *buffer, int clientfd);

//Global variables
pthread_attr_t attr;

int main(int argc, char *argv[])
{
    int port = 8801;
    bzero(mount, sizeof(mount));
    sprintf(mount, "%s", argv[1]);

    pthread_t thread;
    
    //Initialize attribute to set state of threads to detached.
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    //Initialize mutex
    //pthread_mutex_init(&logmutex, NULL);
    
    signal(SIGPIPE, SIG_IGN);

    /* Create a listening descriptor */
    int listenfd = open_listenfd(port);

    /*Create initial thread*/
    pthread_create(&thread, &attr, &handle_request, (void *)(&listenfd));

    while(1)
    {
        pause();
    }
    /* Control never reaches here */
    exit(0);
}

void * handle_request(void *arg)
{
    printf("I WAS CREATED\n");
    int listenfd = (*(int *)(arg)); /* The proxy's listening descriptor */
    socklen_t clientlen;            /* Size in bytes of the client socket address */
    struct sockaddr_in clientaddr;  /* Clinet address structure*/  
    int connfd;                     /* socket desciptor for talkign wiht client*/ 
    pthread_t thread;

    //Accept Connection
    clientlen = sizeof(clientaddr);
    connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);

    //Create thread for next request
    pthread_create(&thread, &attr, &handle_request, (void *)(&listenfd));
    
    char recv_data[1024];
    bzero(recv_data, 1024);
    uint16_t op;

    recv(connfd, &recv_data, 1024, 0);

    memcpy(&op, recv_data, 2);
    op = ntohs(op);
    printf("op: %i\n", op);

    switch(op)
    {
        case 1:
            printf("OPEN\n")
            server_open(recv_data, connfd);
            break;
        case 2:
            break;
        case 3:
            break;
        case 4:
            printf("GET_ATTR\n");
            server_getatrr(recv_data, connfd);
            break;
        case 5:
            break;
        case 6:
            break;
        case 7:
            break;
        case 8:
            break;
        case 9:
            break;
        case 10:
            break;
    }

    return NULL;
}

void server_getatrr(char *buffer, int clientfd)
{
    struct stat fileStat;
    char path_temp[1024];
    char *path;
    char send_request[1024];
    uint16_t response;
    bzero(path_temp, sizeof(path_temp));
    bzero(send_request, sizeof(send_request));
    memcpy(path_temp, mount, strlen(mount));
    printf("Buffer: %s\n", &buffer[2]);
    memcpy(&path_temp[strlen(mount)], &buffer[2], strlen(&buffer[2]));
    printf("Path_Temp %s:%i\n", path_temp, strlen(path_temp));
    int n;
    path = malloc(strlen(path_temp)*sizeof(char));
    memcpy(path, path_temp, strlen(path_temp));
    printf("Path: %s\n", path);
    if((n = stat(path, &fileStat)) < 0)
    {
        printf("NOPE %i\n", n);
        perror("server");
        response = 0;
        response = htons(response);
        memcpy(send_request, &response, 2);
    }
    else
    {
        response = 1;
        response = htons(response);
        memcpy(send_request, &response, 2);
        memcpy(&send_request[2], &fileStat, sizeof(struct stat));
    }

    send(clientfd, send_request, sizeof(struct stat) + 2, 0);
}

void server_open(char *buffer, int clientfd)
{
    struct stat fileStat;
    char path_temp[1024];
    char *path;
    char send_request[1024];
    uint16_t response;
    int permissions;
    int n;

    bzero(path_temp, sizeof(path_temp));
    bzero(send_request, sizeof(send_request));
    
    memcpy(&permissions, &buffer[2], sizeof(int));

    memcpy(path_temp, mount, strlen(mount));
    memcpy(&path_temp[strlen(mount)], &buffer[2+sizeof(int)], strlen(&buffer[2+sizeof(int)]));
    printf("Path_Temp %s:%i\n", path_temp, strlen(path_temp));
    
    path = malloc(strlen(path_temp)*sizeof(char));
    memcpy(path, path_temp, strlen(path_temp));
    printf("Path: %s\n", path);
    
    if((n = stat(path, &fileStat)) < 0)
    {
        printf("NOPE %i\n", n);
        perror("server");
        response = 0;
        response = htons(response);
        memcpy(send_request, &response, 2);
    }
    else
    {
        response = 1;
        response = htons(response);
        memcpy(send_request, &response, 2);
        memcpy(&send_request[2], &fileStat, sizeof(struct stat));
    }

    send(clientfd, send_request, sizeof(struct stat) + 2, 0);
}

int open_listenfd(int port) 
{
    int listenfd, optval=1;
    struct sockaddr_in serveraddr;
  
    /* Create a socket descriptor */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  return -1;
 
    /* Eliminates "Address already in use" error from bind. */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, 
       (const void *)&optval , sizeof(int)) < 0)
  return -1;

    /* Listenfd will be an endpoint for all requests to port
       on any IP address for this host */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET; 
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    serveraddr.sin_port = htons((unsigned short)port); 
    if (bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
  return -1;

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, 100) < 0)
  return -1;
    return listenfd;
}

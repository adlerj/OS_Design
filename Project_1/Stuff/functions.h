#include <sys/types.h>
#include <sys/ipc.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h> 
#include <signal.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "bit_char.c"

unsigned char *read_file (char *, int *, int *, char *);
void print_matrix(unsigned char *, int);
void err_exit(char *);

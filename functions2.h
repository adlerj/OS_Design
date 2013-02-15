#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>

int *create_matrix(int lineCount);
int *read_file (char *filename, int * threadCount, int * lineCount);
void print_matrix(int *matrix, int lineCount);
void release_matrix(int *matrix);
int main(int argc, char *argv[]);
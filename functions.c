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

/*Reads input file
First entry -> number of threads
Second entry -> number of nodes*/
unsigned char *read_file (char *filename, int *number_of_threads, int *number_of_nodes, char *program)
{
	unsigned char *matrix; 
	int a; 
	int b;

	FILE *file = fopen(filename, "r");
	if(file == NULL)
	{
		err_exit(program);
	}
	fscanf(file, "%d", number_of_threads);	
	fscanf(file, "%d", number_of_nodes);
	
	matrix = bit_array_create(*number_of_nodes * *number_of_nodes);

	a = 0;
	b = 0;
	while (!feof(file))
	{
		fscanf (file, "%d %d", &a, &b);
		if(a > *number_of_nodes || b > *number_of_nodes)
		{
			err_exit(program);
		}
		bit_array_set(matrix, (a-1)* *number_of_nodes + (b-1));
	}
	fclose (file);

	return matrix;        
}

/*Prints bitpacked matrix*/
void print_matrix(unsigned char *matrix, int number_of_nodes)
{
	int i;
	int j;
	for(i = 0; i < number_of_nodes; i++)
	{
		for(j = 0; j < number_of_nodes; j++)
		{
			printf("%i ", bit_array_get(matrix, i*number_of_nodes + j));
		}
		printf("\n");
	}
}

/*Prints an error*/
void err_exit(char *name)
{
	perror(name);
	exit(0);
}

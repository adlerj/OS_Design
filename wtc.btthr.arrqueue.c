#include <sys/types.h>
#include <sys/ipc.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "bit_char.c"

//Global Variables
int number_of_threads;
int number_of_nodes;

int *queue = NULL;
int *queue_pos = NULL;

unsigned char *data_matrix = NULL;

pthread_t *threads = NULL;
pthread_mutex_t mutexmatrix;
pthread_mutex_t mutexqueue;

pthread_barrier_t barrier;
pthread_barrier_t kbarrier;

int k = 0;

//Reads input file
//First entry -> number of threads
//Second entry -> number of nodes
unsigned char *read_file (char *filename)
{
	FILE *file = fopen(filename, "r");
	fscanf(file, "%d", &number_of_threads)
;	fscanf(file, "%d", &number_of_nodes);
	
	unsigned char *matrix = bit_array_create(number_of_nodes*number_of_nodes);

	int a = 0;
	int b = 0;
	while (!feof(file))
	{
		fscanf (file, "%d %d", &a, &b);
		bit_array_set(matrix, (a-1)*number_of_nodes + (b-1), 1);
	}
	fclose (file);

	return matrix;        
}

//Prints input matrix
void print_matrix(unsigned char *matrix)
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

void *transitive_closure_worker(void *arg)
{
	//local variables
	int kl;
	int i;
	int j;
	int result;

	while(k < number_of_nodes)
	{
		pthread_barrier_wait(&kbarrier);

		while(*queue_pos != -1)
		{
			pthread_mutex_lock(&mutexqueue);
			if(*queue_pos != -1)
			{
				kl = *(queue_pos);
				i = *(queue_pos+1);
				queue_pos += 2;
			}
			pthread_mutex_unlock(&mutexqueue);

			unsigned ik = bit_array_get(data_matrix, i*number_of_nodes+kl);
			for(j = 0; j < number_of_nodes; ++j)
			{
				pthread_mutex_lock(&mutexmatrix);
				result = bit_array_get(data_matrix, i*number_of_nodes+j) || (ik && bit_array_get(data_matrix, kl*number_of_nodes+j));
				bit_array_set(data_matrix, i*number_of_nodes+j, result);
				pthread_mutex_unlock (&mutexmatrix);
			}
		}
		pthread_barrier_wait(&barrier);	
		pthread_barrier_wait(&kbarrier);
	}
	return NULL;
}

//main: thread processing
int main(int argc, char *argv[])
{
	//Create variable for loops
	int i;

	//Pull data from file
	data_matrix = read_file(argv[1]);
	printf("Initial matrix:\n");
	print_matrix(data_matrix);
	
	//Initialize thread vector
	threads = (pthread_t *)malloc(sizeof(pthread_t)*number_of_threads);

	//Initialize mutex
	pthread_mutex_init(&mutexmatrix, NULL);
	pthread_mutex_init(&mutexqueue, NULL);
	
	//Initialize barrier
	pthread_barrier_init(&barrier, NULL, number_of_threads+1);
	pthread_barrier_init(&kbarrier, NULL, number_of_threads+1);

	//initialize queue
	queue = (int *)malloc(sizeof(int)*(number_of_nodes*2 + 1));

	//Create threads
	for(i = 0; i < number_of_threads; ++i)
	{
		pthread_create(&threads[i], NULL, transitive_closure_worker, NULL);
	}

	//Transitive closure algorithm
	int count;
	for(k = 0; k < number_of_nodes;)
	{
		pthread_mutex_lock(&mutexqueue);
		count = 0;
		for(i = 0; i < number_of_nodes; ++i)
		{
			queue[count] = k;
			queue[count+1] = i;
			count += 2;
		}
		queue[count] = -1;
		queue_pos = queue;
		pthread_mutex_unlock(&mutexqueue);
		pthread_barrier_wait(&kbarrier);
		pthread_barrier_wait(&barrier);
		++k;
		pthread_barrier_wait(&kbarrier);
	}

	//Join threads
	for(i = 0; i < number_of_threads; ++i)
	{
		pthread_join(threads[i], NULL);
	}

	printf("\nTransitive Closure Matrix:\n");
	print_matrix(data_matrix);
	
	free(data_matrix);
	free(threads);
	free(queue);

	//pthread_attr_destroy(&attr);
	pthread_mutex_destroy(&mutexmatrix);
	pthread_mutex_destroy(&mutexqueue);
	pthread_barrier_destroy(&barrier);
	pthread_barrier_destroy(&kbarrier);
	
	return 0;
}

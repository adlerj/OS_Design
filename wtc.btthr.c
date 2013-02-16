#include <sys/types.h>
#include <sys/ipc.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "bit_char.c"

struct queue_node
{
	int i;
	int k;
	struct queue_node *next;
};


//Global Variables
int number_of_threads;
int number_of_nodes;

unsigned char *data_matrix = NULL;

pthread_t *threads = NULL;
pthread_mutex_t mutexmatrix;
pthread_mutex_t mutexqueue;

pthread_barrier_t barrier;
pthread_barrier_t kbarrier;
pthread_barrier_t qbarrier;

struct queue_node *root = NULL;
struct queue_node *iter = NULL;

int k = 0;
int length;


//Creates an n by n matrix in a one d matrix
//n = number_of_nodes
//matrix[n][m] = n*number_of_nodes + m


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
		printf("a: %i, b: %i\n", a, b);
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

void *transitive_closure(void *arg)
{
	//set parameters for operation
	//int start = ((int)arg)*length;
	//int end = start + length;
	//pthread_mutex_unlock(&input_value);
	//printf("myid: %ld; start: %i; end: %i\n", (long int)syscall(224), start, end);

	//loop variables
	int kl;
	int i;
	int j;
	int result;
	while(k < number_of_nodes)
	{
		pthread_barrier_wait(&qbarrier);
		while(root != NULL)
		{
			pthread_mutex_lock(&mutexqueue);
			printf("here\n");
			if(root != NULL)
			{
				printf("GETTING NODE\n");
				printf("This is k: %i\n", root->k);
				printf("This is i: %i\n", root->i);
				kl = root->k;
				i = root->i;
				iter = root;
				root = root->next;
				printf("END GETTING NODE\n");
				iter->next = NULL;
				free(iter);
				printf("FREED ITER\n");
			}
			printf("here2\n");
			pthread_mutex_unlock(&mutexqueue);

			for(j = 0; j < number_of_nodes; ++j)
			{
				pthread_mutex_lock(&mutexmatrix);
				printf("I AM HERE\n");
				result = bit_array_get(data_matrix, i*number_of_nodes+j) || (bit_array_get(data_matrix, i*number_of_nodes+kl) && bit_array_get(data_matrix, kl*number_of_nodes+j));
				printf("I AM HERE 2\n");
				bit_array_set(data_matrix, i*number_of_nodes+j, result);
				pthread_mutex_unlock (&mutexmatrix);
			}
			printf("After done with j\n");
		}
		pthread_barrier_wait(&barrier);	
		pthread_barrier_wait(&kbarrier);
	}
	pthread_exit(NULL);
}

//main: thread processing
int main(int argc, char *argv[])
{
	//Create status variable for threads
	void *status;

	//Create variable for loops
	int i;

	//Pull data from file
	data_matrix = read_file(argv[1]);
	printf("Initial matrix:\n");
	print_matrix(data_matrix);
	
	//set length of of operations
	length = number_of_nodes/number_of_threads;

	//Initialize thread vector
	threads = (pthread_t *)malloc(sizeof(pthread_t)*number_of_threads);

	//Initialize mutex
	pthread_mutex_init(&mutexmatrix, NULL);
	pthread_mutex_init(&mutexqueue, NULL);
	
	//Initialize barrier
	pthread_barrier_init(&barrier, NULL, number_of_threads+1);
	pthread_barrier_init(&kbarrier, NULL, number_of_threads+1);
	pthread_barrier_init(&qbarrier, NULL, number_of_threads+1);

	//Set thread attribute and status
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	//Create threads
	for(i = 0; i < number_of_threads; ++i)
	{
		pthread_create(&threads[i], &attr, transitive_closure, NULL);
	}
	printf("THREADS CREATED\n");
	int j;
	//Transitive closure algorithm
	for(k = 0; k < number_of_nodes;)
	{
		pthread_mutex_lock(&mutexqueue);
		printf("CREATING QUEUE\n");
		for(i = 0; i < number_of_nodes; ++i)
		{
			if(root == NULL)
			{
				root = (struct queue_node *) malloc(sizeof(* root));
				root->k = k;
				root->i = i;
				root->next = NULL;
				iter = root;
			}
			else
			{
				iter->next = (struct queue_node *) malloc(sizeof(* root));
				iter->next->k = k;
				iter->next->i = i;
				iter->next->next = NULL;
				iter = iter->next;
			}
		}
		printf("DONE CREATING QUEUE\n");
		pthread_barrier_wait(&qbarrier);
		pthread_mutex_unlock(&mutexqueue);
		pthread_barrier_wait(&barrier);
		++k;
		pthread_barrier_wait(&kbarrier);
	}

	/*
	for(k; k < number_of_nodes;)
	{
		pthread_barrier_wait(&barrier);
		++k;
		pthread_barrier_wait(&kbarrier);
	}
	*/

	//Join threads
	for(i = 0; i < number_of_threads; ++i)
	{
		pthread_join(threads[i], &status);
	}

	printf("Transitive Closure Matrix:\n");
	print_matrix(data_matrix);
	
	free(data_matrix);
	free(threads);	

	pthread_attr_destroy(&attr);
	pthread_mutex_destroy(&mutexmatrix);
	pthread_mutex_destroy(&mutexqueue);
	pthread_barrier_destroy(&barrier);
	pthread_barrier_destroy(&kbarrier);

	pthread_exit(NULL);

	return 0;
}

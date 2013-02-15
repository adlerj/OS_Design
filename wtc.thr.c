#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <semaphore.h>

//Global Variables
int number_of_threads;
int number_of_nodes;

int *input_data = NULL;

pthread_t *threads = NULL;
pthread_mutex_t mutexmatrix;
pthread_mutex_t input_value;

pthread_barrier_t barrier;
pthread_barrier_t kbarrier;


int k = 0;
int length;


//Creates an n by n matrix in a one d matrix
//n = number_of_nodes
//matrix[n][m] = n*number_of_nodes + m
int *create_matrix()
{
	int *matrix = (int *)malloc(sizeof(int)*number_of_nodes*number_of_nodes);
	memset(matrix, 0, number_of_nodes*number_of_nodes);
	return matrix;
}

//Reads input file
//First entry -> number of threads
//Second entry -> number of nodes
int *read_file (char *filename)
{
	FILE *file = fopen(filename, "r");
	fscanf(file, "%d", &number_of_threads);
	fscanf(file, "%d", &number_of_nodes);
	
	int *matrix = create_matrix();

	int a = 0;
	int b = 0;
	while (!feof(file))
	{
		fscanf (file, "%d %d", &a, &b);
		matrix[(a-1)*number_of_nodes + (b-1)] = 1;    
    	}
	fclose (file);
	return matrix;        
}

//Prints input matrix
void print_matrix(int *matrix)
{
	int i;
	int j;
	for(i = 0; i < number_of_nodes; i++)
	{
		for(j = 0; j < number_of_nodes; j++)
		{
			printf("%i ", matrix[i*number_of_nodes + j]);
		}
		printf("\n");
	}
}

void *transitive_closure(void *arg)
{
	//set parameters for operation
	int start = ((int)arg)*length;
	int end = start + length;
	pthread_mutex_unlock(&input_value);
	printf("myid: %ld; start: %i; end: %i\n", (long int)syscall(224), start, end);

	//loop variables
	int i;
	int j;
	int result;
	while (k < number_of_nodes)
	{
		for(i = start; i < end; ++i)
		{	
			for(j = 0; j < number_of_nodes; ++j)
			{
				pthread_mutex_lock(&mutexmatrix);
				result = input_data[i*number_of_nodes+j] || (input_data[i*number_of_nodes+k] && input_data[k*number_of_nodes+j]);
				input_data[i*number_of_nodes+j] = result;
				pthread_mutex_unlock (&mutexmatrix);
			}
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
	input_data = read_file(argv[1]);
	printf("Initial matrix:\n");
	print_matrix(input_data);
	
	//set length of of operations
	length = number_of_nodes/number_of_threads;

	//Initialize thread vector
	threads = (pthread_t *)malloc(sizeof(pthread_t)*number_of_threads);

	//Initialize mutex
	pthread_mutex_init(&mutexmatrix, NULL);
	pthread_mutex_init(&input_value, NULL);
	
	//Initialize barrier
	pthread_barrier_init(&barrier, NULL, number_of_threads+1);
	pthread_barrier_init(&kbarrier, NULL, number_of_threads+1);

	//Set thread attribute and status
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	//Create threads
	for(i = 0; i < number_of_threads; ++i)
	{
		pthread_mutex_lock(&input_value);
		pthread_create(&threads[i], &attr, transitive_closure, (void *)(i));

	}

	//Transitive closure algorithm
	for(k; k < number_of_nodes;)
	{
		pthread_barrier_wait(&barrier);
		++k;
		pthread_barrier_wait(&kbarrier);
	}

	//Join threads
	for(i = 0; i < number_of_threads; ++i)
	{
		pthread_join(threads[i], &status);
	}

	printf("Transitive Closure Matrix:\n");
	print_matrix(input_data);
	
	free(input_data);
	//free(output_data);
	free(threads);	

	pthread_attr_destroy(&attr);
	pthread_mutex_destroy(&mutexmatrix);
	pthread_mutex_destroy(&input_value);
	pthread_barrier_destroy(&barrier);
	pthread_barrier_destroy(&kbarrier);
	pthread_exit(NULL);
}

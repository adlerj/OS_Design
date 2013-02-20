#include "functions.c"

/*Global Variables*/
int number_of_threads;
int number_of_nodes;

unsigned char *data_matrix = NULL;

pthread_t *threads = NULL;
pthread_mutex_t mutexmatrix;
pthread_mutex_t input_value;

pthread_barrier_t barrier;
pthread_barrier_t kbarrier;

int k = 0;
int length;

/*Method headers*/
void *transitive_closure_thread(void *);

/*main: thread processing without queue*/
int main(int argc, char *argv[])
{
	struct timeval t0;
	struct timeval t1;

	/*Create variable for loops*/
	int i;

	/*Variable to hold time elapsed*/
	long elapsed;

	gettimeofday(&t0, 0);
	/*Pull data from file*/
	data_matrix = read_file(argv[1], &number_of_threads, &number_of_nodes, "wtc_thr");

	/*
	printf("Initial matrix:\n");
	print_matrix(data_matrix, number_of_nodes);
	*/
	
	/*set length of of operations*/
	length = number_of_nodes/number_of_threads;

	/*Initialize thread vector*/
	threads = (pthread_t *)malloc(sizeof(pthread_t)*number_of_threads);

	/*Initialize mutex*/
	pthread_mutex_init(&mutexmatrix, NULL);
	pthread_mutex_init(&input_value, NULL);
	
	/*Initialize barrier*/
	pthread_barrier_init(&barrier, NULL, number_of_threads+1);
	pthread_barrier_init(&kbarrier, NULL, number_of_threads+1);

	/*Create threads*/
	for(i = 0; i < number_of_threads; ++i)
	{
		pthread_mutex_lock(&input_value);
		pthread_create(&threads[i], NULL, transitive_closure_thread, (void *)(i));
	}

	/*Transitive closure algorithm*/
	while(k < number_of_nodes)
	{
		pthread_barrier_wait(&barrier);
		++k;
		pthread_barrier_wait(&kbarrier);
	}

	/*Join threads*/
	for(i = 0; i < number_of_threads; ++i)
	{
		pthread_join(threads[i], NULL);
	}
	gettimeofday(&t1, 0);
	elapsed = (t1.tv_sec-t0.tv_sec)*1000000 + t1.tv_usec-t0.tv_usec;
	printf("%lu\n", elapsed);

	/*
	printf("Transitive Closure Matrix:\n");
	print_matrix(data_matrix, number_of_nodes);
	*/

	free(data_matrix);
	free(threads);

	pthread_mutex_destroy(&mutexmatrix);
	pthread_mutex_destroy(&input_value);
	pthread_barrier_destroy(&barrier);
	pthread_barrier_destroy(&kbarrier);
	
	return 0;
}

void *transitive_closure_thread(void *arg)
{
	int i;
	int j;
	int result;
	
	/*set parameters for operation*/
	int start = ((int)arg)*length;
	int end = start + length;
	pthread_mutex_unlock(&input_value);

	while (k < number_of_nodes)
	{
		for(i = start; i < end; ++i)
		{	
			int ik = bit_array_get(data_matrix, i*number_of_nodes+k);
			for(j = 0; j < number_of_nodes; ++j)
			{
				pthread_mutex_lock(&mutexmatrix);
				result = (ik && bit_array_get(data_matrix, k*number_of_nodes+j));
				if(result == 1)
				{
					bit_array_set(data_matrix, i*number_of_nodes+j);
				}
				pthread_mutex_unlock (&mutexmatrix);
			}
		}
		pthread_barrier_wait(&barrier);	
		pthread_barrier_wait(&kbarrier);
	}
	return NULL;
}

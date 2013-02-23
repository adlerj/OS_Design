#include "wtc_thr.c"

/*Method headers*/
void *transitive_closure_thread_worker(void *);

/*main: thread processing with queue*/
int wtc_btthr(char *argv)
{
	struct timeval t0;
	struct timeval t1;

	/*Create variable for loops*/
	int i;

	/*Variable to hold time elapsed*/
	long elapsed;

	/*Pull data from file*/
	data_matrix = read_file(argv, &number_of_threads, &number_of_nodes, "wtc_btthr");

	/*Initialize thread vector*/
	threads = (pthread_t *)malloc(sizeof(pthread_t)*number_of_threads);

	/*Initialize mutex*/
	pthread_mutex_init(&mutexmatrix, NULL);
	pthread_mutex_init(&mutexqueue, NULL);
	
	/*Initialize barrier*/
	pthread_barrier_init(&barrier, NULL, number_of_threads+1);
	pthread_barrier_init(&kbarrier, NULL, number_of_threads+1);

	/*initialize queue*/
	queue = number_of_nodes;

	gettimeofday(&t0, 0);
	/*Create threads*/
	for(i = 0; i < number_of_threads; ++i)
	{
		pthread_create(&threads[i], NULL, transitive_closure_thread_worker, NULL);
	}

	/*Transitive closure algorithm*/
	for(k = 0; k < number_of_nodes;)
	{
		pthread_mutex_lock(&mutexqueue);
		queue = 10;
		pthread_mutex_unlock(&mutexqueue);
		pthread_barrier_wait(&kbarrier);
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
	
	
	print_matrix(data_matrix, number_of_nodes);
	printf("Time: %lu us\n", elapsed);

	free(data_matrix);
	free(threads);

	pthread_mutex_destroy(&mutexmatrix);
	pthread_mutex_destroy(&mutexqueue);
	pthread_barrier_destroy(&barrier);
	pthread_barrier_destroy(&kbarrier);
	
	return 0;
}

void *transitive_closure_thread_worker(void *arg)
{
	/*local variables*/
	int i;
	int j;
	int result;

	unsigned ik;
	while(k < number_of_nodes)
	{
		pthread_barrier_wait(&kbarrier);

		while(queue != -1)
		{
			pthread_mutex_lock(&mutexqueue);
			if(queue != -1)
			{
				i = queue;
				queue--;
			}
			pthread_mutex_unlock(&mutexqueue);

			ik = bit_array_get(data_matrix, i*number_of_nodes+k);
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

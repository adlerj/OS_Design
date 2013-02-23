#include "functions.c"
/*Global Variables*/
int number_of_threads;
int number_of_nodes;

int queue;

unsigned char *data_matrix = NULL;

pthread_t *threads = NULL;
pthread_mutex_t mutexmatrix;
pthread_mutex_t mutexqueue;
pthread_mutex_t input_value;

pthread_barrier_t barrier;
pthread_barrier_t kbarrier;

int k = 0;
int length;
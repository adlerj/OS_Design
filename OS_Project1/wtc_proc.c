#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h> 
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <pthread.h>

void print_matrix2(int *matrix,int lineCount)
{
	int i;
	int j;
	for(i = 0; i < lineCount; i++)
	{
		for(j = 0; j < lineCount; j++)
		{
			printf("%i ", matrix[i*lineCount + j]);
		}
		printf("\n");
	}
}

void err_exit2()
{
	perror("wtc");
	exit(1);
}

int wtc_proc(char *argv){

	struct timeval t0;
	struct timeval t1;

	int threadCount;
	int lineCount;

	FILE *file = fopen(argv, "r"); 

	fscanf(file, "%d", &threadCount);   
	fscanf(file, "%d", &lineCount);   

	//open shared memory
	int shm = shm_open("/myshm", O_RDWR | O_CREAT, S_IRWXU);
	int size = (sizeof(int) * lineCount * lineCount) + (sizeof(sem_t)) + sizeof(pthread_barrier_t);
	ftruncate(shm, size);


	//read in matrix to shared memory
	int * matrix = (int *) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);		
	int a;
	int b;
	while (!feof (file))
    {
		fscanf (file, "%d %d", &a, &b);
		matrix[((a-1)*lineCount) + (b-1)] = 1; 
    }
	fclose (file); 
	
	//based around int pointers
	int semOffset = (sizeof(sem_t)/sizeof(int));

	//init functions in shared mem
	sem_t * matrixLock;
	matrixLock = (sem_t *) (matrix + (lineCount*lineCount));
	sem_init(matrixLock, 1, 1);


	pthread_barrierattr_t attr; //needed for process shared barriers
	
	pthread_barrierattr_init(&attr);
	pthread_barrierattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);

	pthread_barrier_t * barrier1 = (pthread_barrier_t *) (matrixLock + semOffset);
	pthread_barrier_init(barrier1, &attr, threadCount+1);
	

	int pid = 1;
	int * childPids;
	childPids = malloc(sizeof(int)*threadCount);
	if(childPids == NULL){err_exit2();}
	
	int myId = -1; //parentID is -1
	int x = 0;
	
	gettimeofday(&t0, 0);

	for(;(x < threadCount) && (pid != 0); x++){
		if((pid=fork()) < 0){//error
			err_exit2();
		}
		if(pid != 0){ //Parent
			childPids[x] = pid;
		}
		else{ //Child
			myId = x;
		}
	}

	//get pointers from shared mem
	matrix = (int *) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
	matrixLock = (sem_t *) (matrix + (lineCount*lineCount));
	
	barrier1 = (pthread_barrier_t *) (matrixLock + semOffset);

	int rowsPer = lineCount/threadCount;

	if(myId > -1){//Children
		int k = 0;
		for(;k < lineCount; k++){
			int y = (myId * rowsPer);
			for(;y < ((myId+1) * rowsPer); y++){
				sem_wait(matrixLock);			
				int j = 0;
				for(;j < lineCount; j++){
					matrix[y + (j * (lineCount))] = matrix[y + (j * (lineCount))] || (matrix[y + (k * (lineCount))] && matrix[k + (j * (lineCount))]);
				}
				sem_post(matrixLock);			
			}
			pthread_barrier_wait(barrier1);
		}
	}
	else{//Parent
		int k = 0;
		for(;k < lineCount; k++){
			pthread_barrier_wait(barrier1);
		}
		
		int z = 0;
		for(;z < threadCount; z++){
			kill(childPids[z], SIGKILL);//bye kids
		}
		free(childPids);

		matrix = (int *) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
		print_matrix2(matrix, lineCount);

		munmap(matrix, size);
		shm_unlink("/myshm");//unlink shared mem

		gettimeofday(&t1, 0);
		long elapsed = (t1.tv_sec-t0.tv_sec)*1000000 + t1.tv_usec-t0.tv_usec;
		printf("Time: %lu us\n", elapsed);
	}
	return 0;
}

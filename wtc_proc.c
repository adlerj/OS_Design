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


void err_exit()
{
	perror("wtc");
	exit(1);
}

int main(int argc, char ** argv){

	struct timeval t0;
	struct timeval t1;

	gettimeofday(&t0, 0);
	
	if(argc != 2){fprintf(stderr, "Invalid Arguments.  Usage: functions <filename>\n"); exit(3);}

	int threadCount;
	int lineCount;

	FILE *file = fopen(argv[1], "r"); 

	fscanf(file, "%d", &threadCount);   
	fscanf(file, "%d", &lineCount);   
/////////////////////////////////////////////////////////////////////////////////
	int shm = shm_open("/myshm", O_RDWR | O_CREAT, S_IRWXU);
	int size = (sizeof(int) * lineCount * lineCount) + (sizeof(sem_t)) + sizeof(pthread_barrier_t);
	ftruncate(shm, size);
//////////////////////////////////////////////////////////////////////////////////
	int * matrix = (int *) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);		
	int a;
	int b;
	while (!feof (file))
    {
		fscanf (file, "%d %d", &a, &b);
		matrix[((a-1)*lineCount) + (b-1)] = 1;
		//printf ("%d %d\n", a, b);     
    }
	fclose (file); 
	////////////////////////////////////////////////////////////////////////////////////////////////////
	int semOffset = (sizeof(sem_t)/sizeof(int));
	int barrierOffset = (sizeof(pthread_barrier_t)/sizeof(int));

	sem_t * matrixLock;
	matrixLock = (sem_t *) (matrix + (lineCount*lineCount));
	sem_init(matrixLock, 1, 1);

	pthread_barrierattr_t attr; 
	int ret; 
	ret = pthread_barrierattr_init(&attr);
	pthread_barrierattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);

	pthread_barrier_t * barrier1 = (pthread_barrier_t *) (matrixLock + semOffset);
	pthread_barrier_init(barrier1, &attr, threadCount+1);
	//////////////////////////////////////////////////////////////////////////////////////////////////
	//printf("Number of processes: %i\nNumber of Lines: %i\n\n", threadCount, lineCount);

	//printf("\nInitial matrix:\n\n");
	////print_matrix(matrix, lineCount);
	
	int pid = 1;
	int * childPids = malloc(sizeof(int)*threadCount);
	int myId = -1; //parentID is -1
	int x = 0;
	
	for(;(x < threadCount) && (pid != 0); x++){
		if((pid=fork()) < 0){
			printf("doodoo %i\n",pid);
			err_exit();
		}
		if(pid != 0){ //Parent keep track of child pids
			childPids[x] = pid;
		}
		else{ //child obtain identity
			//printf("Thread %i started!\n", x);
			myId = x;
		}
	}

	int i = 0;
	
	/*if(pid!=0){
		for(;i < threadCount; i++){printf("My ID is: %i\n", childPids[i]);}
	}*/
////////////////Prepared//Processes///////////////////////
	
	matrix = (int *) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
	matrixLock = (sem_t *) (matrix + (lineCount*lineCount));
	
	barrier1 = (pthread_barrier_t *) (matrixLock + semOffset);
////////////////////////////////////////////////////////
	int rowsPer = lineCount/threadCount;

	if(myId > -1){//Children
		int nextFlip = 1;
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
		int done = 0;
		int nextFlip = 1;
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
		printf("\nOutput Matrix:\n");
		print_matrix(matrix, lineCount);

		munmap(matrix, size);
		shm_unlink("/myshm");

		gettimeofday(&t1, 0);
		long elapsed = (t1.tv_sec-t0.tv_sec)*1000000 + t1.tv_usec-t0.tv_usec;
		printf("%lu\n", elapsed);
		//free(t0);free(t1);
	}
	return 0;
}

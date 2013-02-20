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

#include "functions2.h"


void err_exit()
{
	perror("wtc");
	exit(1);
}

//open matrix, get line and thread count
FILE * openMatrix(char * fileName, int * threadCount, int * lineCount)
{
	FILE *file = fopen(fileName, "r"); 

	fscanf(file, "%d", threadCount);   
	fscanf(file, "%d", lineCount); 
	return file;
}

int readMatrix(FILE * file, int * matrix, int lineCount)
{
	int a;
	int b;
	while (!feof (file))
    {
		fscanf (file, "%d %d", &a, &b);
		matrix[((a-1)*lineCount) + (b-1)] = 1;
		//printf ("%d %d\n", a, b);     
    }
	fclose (file); 
}

void setQueue(int * queue, int lineCount){
	int l;
	int * pQueue = queue;
	for(l = 0; l < lineCount; l++){
		*pQueue = l;
		pQueue++;
	}
	*pQueue=-1;
}

int allocShm(int lineCount, FILE * file, int size){
	int shm = shm_open("/myshm", O_RDWR | O_CREAT, S_IRWXU);
	ftruncate(shm, size);

	return shm;	
}

void initShm(int shm, int lineCount, int threadCount, FILE * file, int size){
	int semOffset = (sizeof(sem_t)/sizeof(int));

	int * matrix = (int *) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);		
	readMatrix(file, matrix, lineCount);
	
	sem_t * matrixLock;
	matrixLock = (sem_t *) (matrix + (lineCount*lineCount));
	sem_init(matrixLock, 1, 1);

	sem_t * cLock = (sem_t *) (matrixLock + semOffset);
	sem_init(cLock, 1, 1);

	int * count = (int *) (cLock + semOffset);
	*count = threadCount;

	sem_t * bLock = (sem_t *) (count + 1);
	sem_init(bLock, 1, 1);

	int * bCount = (int *) (bLock + semOffset);
	*bCount = 0;

	int * cont = (int *) (bCount + 1);
	*cont = 0;

	sem_t * contLock = (sem_t *) (cont + 1);
	sem_init(contLock, 1, 1);

	int * queue = (int *) (contLock + semOffset);
	setQueue(queue, lineCount);	

	sem_t * queueLock = (sem_t *) (queue + (lineCount+1));
	sem_init(queueLock, 1, 1);	

	int * qNum = (int *) (queueLock + semOffset);
	*qNum = 0;
}

void childCompute(int * matrix, sem_t * matrixLock, int k, sem_t * queueLock, int * queue, int * qNum, int lineCount){
	int y;
	y = queuePop(queueLock, queue, qNum);
			
	while(y!= -1)
	{
		sem_wait(matrixLock);			
		int j = 0;
		for(;j < lineCount; j++){
			matrix[y + (j * (lineCount))] = matrix[y + (j * (lineCount))] || (matrix[y + (k * (lineCount))] && matrix[k + (j * (lineCount))]);
		}
		sem_post(matrixLock);	
		
		y = queuePop(queueLock, queue, qNum);
	}			
}

int queuePop(sem_t * queueLock, int * queue, int * qNum){
	sem_wait(queueLock);
	int y;
	y = queue[*qNum];
	if(y!=-1){
		(*qNum)++;
	}
	sem_post(queueLock);
	return y;
}

int main(int argc, char ** argv){

	struct timeval t0;
	struct timeval t1;

	gettimeofday(&t0, 0);
	
	if(argc != 2){fprintf(stderr, "Invalid Arguments.  Usage: functions <filename>\n"); exit(3);}

	int threadCount;
	int lineCount;

	FILE * file = openMatrix(argv[1], &threadCount, &lineCount);
	int shm;
	
	int size = (sizeof(int) * lineCount * lineCount) + (sizeof(sem_t)*5) + (sizeof(int)*(4 + 1 + lineCount));
	shm = allocShm(lineCount, file, size);
	initShm(shm, lineCount, threadCount, file, size);

	int * matrix = (int *) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);		
	
	//printf("\nInitial matrix:\n\n");
	//print_matrix(matrix, lineCount);
	//printf("\n");
	
	int pid = 1;
	int * childPids = malloc(sizeof(int)*threadCount);
	int myId = -1; //parentID is -1
	int x = 0;
	
	/*Process Making*/
	for(;(x < threadCount) && (pid != 0); x++){
		if((pid=fork()) < 0){
			err_exit();
		}
		if(pid != 0){ //Parent keep track of child pids
			childPids[x] = pid;
		}
		else{ //child obtain identity
			myId = x;
		}
	}

	int semOffset = (sizeof(sem_t)/sizeof(int));

	matrix = (int *) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
	sem_t * matrixLock = (sem_t *) (matrix + (lineCount*lineCount));
	
	sem_t * cLock = (sem_t *) (matrixLock + semOffset);
	int * count = (int *) (cLock + semOffset);
	
	sem_t * bLock = (sem_t *) (count + 1);
	int * bCount = (int *) (bLock + semOffset);

	int * cont = (int *) (bCount + 1);
	sem_t * contLock = (sem_t *) (cont + 1);

	int * queue = (int *) (contLock + semOffset);
	sem_t * queueLock = (sem_t *) (queue + (lineCount+1));
	int * qNum = (int *) (queueLock + semOffset);


	if(myId > -1){//Children
		int nextFlip = 1;
		int k = 0;
		for(;k < lineCount; k++){
			int y;
			childCompute(matrix, matrixLock, k, queueLock, queue, qNum, lineCount);
			sem_wait(bLock);
			(*bCount)++;
			sem_post(bLock);
			int tCont = -1;
			do{
				sem_wait(contLock);
				tCont = *cont;
				sem_post(contLock);
			}while(tCont!= nextFlip);
			nextFlip = !nextFlip;
		}
		
		sem_wait(cLock);
		(*count)--;
		sem_post(cLock);
	}
	else{//Parent
		int done = 0;
		int nextFlip = 1;
		while(!done){;
			sem_wait(bLock);
			//printf("bCount: %i\n", *bCount);
			if((*bCount == threadCount)&&(*cont !=nextFlip)){
				sem_wait(queueLock);
				*qNum = 0;				
				sem_post(queueLock);

				//printf("Reset Queue\n");
				sem_wait(contLock);
				nextFlip = *cont;
				*cont = !(*cont);
				sem_post(contLock);
				*bCount = 0;
			}	
			sem_post(bLock);
			sem_wait(cLock);
			if(*count == 0){done = 1;}
			sem_post(cLock);
		}
		//printf("All children finished!\n");
		int z = 0;
		for(;z < threadCount; z++){
			kill(childPids[z], SIGKILL);//bye kids
			//printf("Killed child %i\n", z);
		}
		free(childPids);

		matrix = (int *) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
		//printf("\nOutput Matrix:\n");
		//print_matrix(matrix, lineCount);

		munmap(matrix, size);
		shm_unlink("/myshm");

		gettimeofday(&t1, 0);
		long elapsed = (t1.tv_sec-t0.tv_sec)*1000000 + t1.tv_usec-t0.tv_usec;
		printf("%lu\n", elapsed);
	}
	return 0;
}

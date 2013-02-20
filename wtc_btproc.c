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
	int size = (sizeof(int) * lineCount * lineCount) + (sizeof(sem_t)*5) + (sizeof(int)*(4 + 1 + lineCount));
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
	sem_t * matrixLock = (sem_t *) (matrix +(sizeof(int)*lineCount*lineCount));//first semaphore in shared mem
	sem_init(matrixLock, 1, 1);

	sem_t * cLock = (sem_t *) (matrix +(sizeof(int)*lineCount*lineCount) + sizeof(sem_t));//second semaphore in shared mem
	sem_init(cLock, 1, 1);

	int * count = (int *) (matrix +(sizeof(int)*lineCount*lineCount) + (2 * sizeof(sem_t)));
	*count = threadCount;

	sem_t * bLock = (sem_t *) (matrix +(sizeof(int)*lineCount*lineCount) + (2 * sizeof(sem_t)) + sizeof(int));
	sem_init(bLock, 1, 1);

	int * bCount = (int *) (matrix +(sizeof(int)*lineCount*lineCount) + (3 * sizeof(sem_t)) + sizeof(int));
	*bCount = 0;

	int * cont = (int *) (matrix +(sizeof(int)*lineCount*lineCount) + (3 * sizeof(sem_t)) + (2*sizeof(int)));
	*cont = 0;

	sem_t * contLock = (sem_t *) (matrix +(sizeof(int)*lineCount*lineCount) + (3 * sizeof(sem_t)) + (3*sizeof(int)));
	sem_init(contLock, 1, 1);

	int * queue = (int *) (matrix +(sizeof(int)*lineCount*lineCount) + (4 * sizeof(sem_t)) + (3*sizeof(int)));

	int l;
	int * pQueue = queue;
	for(l = 0; l < lineCount; l++){
		*pQueue = l;
		pQueue++;
	}
	*pQueue=-1;

	sem_t * queueLock = (sem_t *) (matrix +(sizeof(int) * lineCount * lineCount) + (sizeof(sem_t)*4) + (sizeof(int)*(3 + 1 + lineCount)));
	sem_init(queueLock, 1, 1);	

	int * qNum = (int *) (matrix +(sizeof(int) * lineCount * lineCount) + (sizeof(sem_t)*5) + (sizeof(int)*(3 + 1 + lineCount)));
	*qNum = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////////
	//printf("Number of processes: %i\nNumber of Lines: %i\n\n", threadCount, lineCount);

	printf("\nInitial matrix:\n\n");
	print_matrix(matrix, lineCount);
	printf("\n");
	
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
	matrixLock = (sem_t *) (matrix +(sizeof(int)*lineCount*lineCount));//first semaphore in shared mem
	
	cLock = (sem_t *) (matrix +(sizeof(int)*lineCount*lineCount) + sizeof(sem_t));//second semaphore in shared mem
	count = (int *) (matrix +(sizeof(int)*lineCount*lineCount) + (2 * sizeof(sem_t)));
	
	bLock = (sem_t *) (matrix +(sizeof(int)*lineCount*lineCount) + (2 * sizeof(sem_t)) + sizeof(int));
	bCount = (int *) (matrix +(sizeof(int)*lineCount*lineCount) + (3 * sizeof(sem_t)) + sizeof(int));
	
	cont = (int *) (matrix +(sizeof(int)*lineCount*lineCount) + (3 * sizeof(sem_t)) + (2*sizeof(int)));
	contLock = (sem_t *) (matrix +(sizeof(int)*lineCount*lineCount) + (3 * sizeof(sem_t)) + (3*sizeof(int)));
	
	queue = (int *) (matrix +(sizeof(int)*lineCount*lineCount) + (4 * sizeof(sem_t)) + (3*sizeof(int)));
	queueLock = (sem_t *) (matrix +(sizeof(int) * lineCount * lineCount) + (sizeof(sem_t)*4) + (sizeof(int)*(3 + 1 + lineCount)));
	qNum = (int *) (matrix +(sizeof(int) * lineCount * lineCount) + (sizeof(sem_t)*5) + (sizeof(int)*(3 + 1 + lineCount)));
		
	int rowsPer = lineCount/threadCount;

	if(myId > -1){//Children
		int nextFlip = 1;
		int k = 0;
		for(;k < lineCount; k++){
			int y = 0;;
			sem_wait(queueLock);			
			y = queue[*qNum];
			if(y!=-1){
				(*qNum)++;
			}
			sem_post(queueLock);

			while(y!= -1)
			{
				printf("Child %i working on line %i of iteration %i\n", myId, y, k);
				sem_wait(matrixLock);			
				int j = 0;
				for(;j < lineCount; j++){
					matrix[y + (j * (lineCount))] = matrix[y + (j * (lineCount))] || (matrix[y + (k * (lineCount))] && matrix[k + (j * (lineCount))]);
				}
				sem_post(matrixLock);	

				//pull from queue
				sem_wait(queueLock);
				y = queue[*qNum];
				if(y!=-1){(*qNum)++;}

				sem_post(queueLock);
			}
			//ready for next k				
			sem_wait(bLock);
			(*bCount)++;
			sem_post(bLock);
			int tCont = -1;
			do{
				sem_wait(contLock);
				tCont = *cont;
				sem_post(contLock);
			}while(tCont!= nextFlip);
			//queue = queue - lineCount;
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

				printf("Parent resetting Queue\n");
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
		printf("\nOutput Matrix:\n");
		print_matrix(matrix, lineCount);

		munmap(matrix, size);
		shm_unlink("/myshm");

		gettimeofday(&t1, 0);
		long elapsed = (t1.tv_sec-t0.tv_sec)*1000000 + t1.tv_usec-t0.tv_usec;
		printf("\nTime(microseconds): %lu\n", elapsed);
		//free(t0);free(t1);
	}
	return 0;
}

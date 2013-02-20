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
	int size = (sizeof(int) * lineCount * lineCount) + (sizeof(sem_t)*4) + (sizeof(int)*4);
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
	
	cLock = (sem_t *) (matrixLock + semOffset);
	count = (int *) (cLock + semOffset);
	
	bLock = (sem_t *) (count + 1);
	bCount = (int *) (bLock + semOffset);

	cont = (int *) (bCount + 1);
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
			int temp = 0;
			sem_wait(bLock);
			(*bCount)++;
			//printf("[ID:%i|bCount %i]\n", myId, *bCount);
			temp=*bCount;
			sem_post(bLock);
			int tCont = -1;
			do{
				//printf("Proc %i, waiting\n", myId);
				sem_wait(contLock);
				tCont = *cont;
				//printf("Cont: %i\n", *cont);
				sem_post(contLock);
			}while(tCont!= nextFlip);
			nextFlip = !nextFlip;
		}

		int temp = 0;
		//printf("Child %i DONE!\n", myId);
		
		sem_wait(cLock);
		(*count)--;
		sem_post(cLock);
		//sem_getvalue(done,&temp);
		//printf("Done is at %i\n",temp);
	}
	else{//Parent
		int done = 0;
		int nextFlip = 1;
		while(!done){;
			sem_wait(bLock);
			//printf("bCount: %i\n", *bCount);
			if((*bCount == threadCount)&&(*cont !=nextFlip)){
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
		//free(t0);free(t1);
	}
	return 0;
}

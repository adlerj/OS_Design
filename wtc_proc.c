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

#include "functions2.h"


void err_exit()
{
	perror("wtc");
	exit(1);
}

int main(int argc, char ** argv){
	
	if(argc != 2){fprintf(stderr, "Invalid Arguments.  Usage: functions <filename>\n"); exit(3);}

	int threadCount;
	int lineCount;

	FILE *file = fopen(argv[1], "r"); 

	fscanf(file, "%d", &threadCount);   
	fscanf(file, "%d", &lineCount);   

	int shm = shm_open("/myshm", O_RDWR | O_CREAT, S_IRWXU);
	int size = (sizeof(int) * lineCount * lineCount) + (sizeof(sem_t)*2);
	ftruncate(shm, size);

	int * matrix = (int *) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);		int a;
	int b;
	while (!feof (file))
    {
		fscanf (file, "%d %d", &a, &b);
		matrix[((a-1)*lineCount) + (b-1)] = 1;
		printf ("%d %d\n", a, b);     
    }
	fclose (file); 
	printf("Hey papa\n");
	////////////////////////////////////////////////////////////////////////////////////////////////////
	sem_t * matrixLock = (sem_t *) matrix +(sizeof(int)*lineCount*lineCount);//first semaphore in shared mem
	sem_init(matrixLock, 1, 1);
	
	sem_t * done = (sem_t *) matrix +(sizeof(int)*lineCount*lineCount) + sizeof(sem_t);//second semaphore in shared mem
	sem_init(done, 1, (-1 * threadCount));
	//////////////////////////////////////////////////////////////////////////////////////////////////
	printf("Number of processes: %i\nNumber of Lines: %i\n\n", threadCount, lineCount);

	printf("\nInitial matrix:\n\n");
	print_matrix(matrix, lineCount);
	
	int pid = 1;
	int * childPids = malloc(sizeof(int)*threadCount);
	int myId = -1; //parentID is -1
	int x = 0;
	
	for(;(x < threadCount) && (pid != 0); x++){
		if((pid=fork()) < 0){
				err_exit();
		}
		if(pid != 0){ //Parent keep track of child pids
			childPids[x] = pid;
		}
		else{ //child obtain identity
			printf("Thread %i started!\n", x);
			myId = x;
		}
	}

	int i = 0;
	
	if(pid!=0){
		for(;i < threadCount; i++){printf("My ID is: %i\n", childPids[i]);}
	}
////////////////Prepared//Processes///////////////////////
	
	
	matrixLock = (sem_t *) matrix +(sizeof(int)*lineCount*lineCount);//first semaphore in shared mem
	done = (sem_t *) matrix +(sizeof(int)*lineCount*lineCount) + sizeof(sem_t);//second semaphore in shared mem
	

	int rowsPer = lineCount/threadCount;

	int k = 0;

	if(myId > -1){//Children
		for(;k < lineCount; k++){
			int y = (myId * rowsPer);
			for(;y < ((myId+1) * rowsPer); y++){
				sem_wait(matrixLock);
				matrix = (int *) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
				int j = 0;
				for(;j < lineCount; j++)
				{
					int yjIndex = y + (j * (lineCount-1));
					int ykIndex = y + (k * (lineCount-1));
					int kjIndex = k + (j * (lineCount-1));
					matrix[yjIndex] = matrix[yjIndex] | matrix[ykIndex] & matrix[kjIndex];
				}
				sem_post(matrixLock);
			}
		}
		sem_post(done);
	}
	else{//Parent
		sem_wait(done);
		int z = 0;
		for(;z < threadCount; z++){
			kill(childPids[z], SIGKILL);//bye kids
		}
	}
	matrix = (int *) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
	print_matrix(matrix, lineCount);
	return 0;
}
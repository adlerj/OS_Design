#include "functions.c"

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

	int shm = shm_open("/myshm", O_RDWR | O_CREAT, S_IRWXU);
	int SIZE = sizeof(int) * lineCount * lineCount/8 +1;
	int size = (SIZE) + (sizeof(sem_t)*5) + (sizeof(int)*(4 + 1 + lineCount));
	ftruncate(shm, size);

	unsigned char * matrix = (unsigned char *) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);		
	int a;
	int b;
	while (!feof (file))
    {
		fscanf (file, "%d %d", &a, &b);
		bit_array_set(matrix, (a-1)* lineCount + (b-1));
    }
	fclose (file); 

	sem_t * matrixLock = (sem_t *) (matrix +SIZE);//first semaphore in shared mem
	sem_init(matrixLock, 1, 1);

	sem_t * cLock = (sem_t *) (matrix +SIZE + sizeof(sem_t));//second semaphore in shared mem
	sem_init(cLock, 1, 1);

	int * count = (int *) (matrix +SIZE + (2 * sizeof(sem_t)));
	*count = threadCount;

	sem_t * bLock = (sem_t *) (matrix +SIZE + (2 * sizeof(sem_t)) + sizeof(int));
	sem_init(bLock, 1, 1);

	int * bCount = (int *) (matrix +SIZE + (3 * sizeof(sem_t)) + sizeof(int));
	*bCount = 0;

	int * cont = (int *) (matrix +SIZE + (3 * sizeof(sem_t)) + (2*sizeof(int)));
	*cont = 0;

	sem_t * contLock = (sem_t *) (matrix +SIZE + (3 * sizeof(sem_t)) + (3*sizeof(int)));
	sem_init(contLock, 1, 1);

	int * queue = (int *) (matrix +SIZE + (4 * sizeof(sem_t)) + (3*sizeof(int)));

	int l;
	int * pQueue = queue;
	for(l = 0; l < lineCount; l++){
		*pQueue = l;
		pQueue++;
	}
	*pQueue=-1;

	sem_t * queueLock = (sem_t *) (matrix +(SIZE) + (sizeof(sem_t)*4) + (sizeof(int)*(3 + 1 + lineCount)));
	sem_init(queueLock, 1, 1);	

	int * qNum = (int *) (matrix +(SIZE) + (sizeof(sem_t)*5) + (sizeof(int)*(3 + 1 + lineCount)));
	*qNum = 0;

	//printf("\nInitial matrix:\n\n");
	//print_matrix(matrix, lineCount);
	//printf("\n");
	
	int pid = 1;
	int * childPids = malloc(sizeof(int)*threadCount);
	int myId = -1; //parentID is -1
	int x = 0;
	
	for(;(x < threadCount) && (pid != 0); x++){
		if((pid=fork()) < 0){
			printf("doodoo %i\n",pid);
			err_exit("wtc_btproc");
		}
		if(pid != 0){ //Parent keep track of child pids
			childPids[x] = pid;
		}
		else{ //child obtain identity
			myId = x;
		}
	}

/*Prepared Processes*/
	
	matrix = (unsigned char *) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
	matrixLock = (sem_t *) (matrix +SIZE);//first semaphore in shared mem
	
	cLock = (sem_t *) (matrix +SIZE + sizeof(sem_t));//second semaphore in shared mem
	count = (int *) (matrix +SIZE + (2 * sizeof(sem_t)));
	
	bLock = (sem_t *) (matrix +SIZE + (2 * sizeof(sem_t)) + sizeof(int));
	bCount = (int *) (matrix +SIZE + (3 * sizeof(sem_t)) + sizeof(int));
	
	cont = (int *) (matrix +SIZE + (3 * sizeof(sem_t)) + (2*sizeof(int)));
	contLock = (sem_t *) (matrix +SIZE + (3 * sizeof(sem_t)) + (3*sizeof(int)));
	
	queue = (int *) (matrix +SIZE + (4 * sizeof(sem_t)) + (3*sizeof(int)));
	queueLock = (sem_t *) (matrix +(SIZE) + (sizeof(sem_t)*4) + (sizeof(int)*(3 + 1 + lineCount)));
	qNum = (int *) (matrix +(SIZE) + (sizeof(sem_t)*5) + (sizeof(int)*(3 + 1 + lineCount)));
		
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
				//printf("Child %i working on line %i of iteration %i\n", myId, y, k);
				sem_wait(matrixLock);			
				int j = 0;
				int ik = bit_array_get(matrix, y*lineCount+k);
				for(;j < lineCount; j++){
					int result = (ik && bit_array_get(matrix, k*lineCount+j));
					if(result == 1)
					{
						bit_array_set(matrix, y*lineCount+j);
					}
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

			if((*bCount == threadCount)&&(*cont !=nextFlip)){
				sem_wait(queueLock);
				*qNum = 0;				
				sem_post(queueLock);

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
		int z = 0;
		for(;z < threadCount; z++){
			kill(childPids[z], SIGKILL);//bye kids
		}
		free(childPids);

		matrix = (unsigned char *) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
		
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

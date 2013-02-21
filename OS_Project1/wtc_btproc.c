#include "wtc_proc.c"

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
    }
	fclose (file);
	return 1;
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

	int * matrix = (int *) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);		
	readMatrix(file, matrix, lineCount);
	
	sem_t * matrixLock;
	matrixLock = (sem_t *) (matrix + (lineCount*lineCount));
	sem_init(matrixLock, 1, 1);

	int * queue = (int *) (matrixLock + 1);
	setQueue(queue, lineCount);	

	sem_t * queueLock = (sem_t *) (queue + (lineCount+1));
	sem_init(queueLock, 1, 1);	

	int * qNum = (int *) (queueLock + 1);

	*qNum = 0;

	pthread_barrierattr_t attr; 

	pthread_barrierattr_init(&attr);
	pthread_barrierattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);

	pthread_barrier_t * barrier1 = (pthread_barrier_t *) (qNum + 1);
	pthread_barrier_init(barrier1, &attr, threadCount+1);

	pthread_barrier_t * barrier2 = (pthread_barrier_t *) (barrier1 + 1);
	pthread_barrier_init(barrier2, &attr, threadCount+1);
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

int wtc_btproc(char * argv){
	struct timeval t0;
	struct timeval t1;

	int threadCount;
	int lineCount;

	FILE * file = openMatrix(argv, &threadCount, &lineCount);
	int shm;
	
	int size = (sizeof(int) * lineCount * lineCount) + (sizeof(sem_t)*2) + (sizeof(int)*(2 + lineCount));
	size = size + (2*sizeof(pthread_barrier_t));
	shm = allocShm(lineCount, file, size);
	initShm(shm, lineCount, threadCount, file, size);

	int * matrix = (int *) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);		
	
	int pid = 1;
	int * childPids = malloc(sizeof(int)*threadCount);
	int myId = -1; //parentID is -1
	int x = 0;
	
	gettimeofday(&t0, 0);
	/*Process Making*/
	for(;(x < threadCount) && (pid != 0); x++){
		if((pid=fork()) < 0){
			err_exit2();
		}
		if(pid != 0){ //Parent keep track of child pids
			childPids[x] = pid;
		}
		else{ //child obtain identity
			myId = x;
		}
	}

	matrix = (int *) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
	sem_t * matrixLock = (sem_t *) (matrix + (lineCount*lineCount));
	

	int * queue = (int *) (matrixLock + 1);
	sem_t * queueLock = (sem_t *) (queue + (lineCount+1));
	int * qNum = (int *) (queueLock + 1);

	pthread_barrier_t * barrier1 = (pthread_barrier_t *) (qNum + 1);
	pthread_barrier_t * barrier2 = (pthread_barrier_t *) (barrier1 + 1);


	if(myId > -1){//Children
		int k = 0;
		for(;k < lineCount; k++){
			childCompute(matrix, matrixLock, k, queueLock, queue, qNum, lineCount);
			pthread_barrier_wait(barrier1);
			pthread_barrier_wait(barrier2);
		}
	}
	else{//Parent
		int k = 0;
		for(;k < lineCount; k++){
			pthread_barrier_wait(barrier1);
			(*qNum)=0;
			pthread_barrier_wait(barrier2);
		}
		int z = 0;
		/*kill children*/
		for(;z < threadCount; z++){
			kill(childPids[z], SIGKILL);
		}
		free(childPids);

		matrix = (int *) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
		print_matrix2(matrix, lineCount);

		munmap(matrix, size);
		shm_unlink("/myshm");

		gettimeofday(&t1, 0);
		long elapsed = (t1.tv_sec-t0.tv_sec)*1000000 + t1.tv_usec-t0.tv_usec;
		printf("Time: %lu us\n", elapsed);
	}
	return 0;
}

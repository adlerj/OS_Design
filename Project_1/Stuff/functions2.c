#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>

//Creates an n by n matrix in a one d matrix
//n = number_of_nodes
//matrix[n][m] = n*number_of_nodes + m
int *create_matrix(int lineCount)
{
	int *matrix = (int *)malloc(sizeof(int)*lineCount*lineCount);
	return matrix;
}

//Reads input file
//First entry -> number of threads
//Second entry -> number of nodes
int *read_file (char *filename, int * threadCount, int * lineCount)
{
	
	FILE *file = fopen(filename, "r");//
	fscanf(file, "%d", threadCount);
	fscanf(file, "%d", lineCount);
	
	int *matrix = create_matrix(*lineCount);

	int a;
	int b;
	while (!feof (file))
    {
		fscanf (file, "%d %d", &a, &b);
		matrix[(a-1)*(*lineCount) + (b-1)] = 1;
		printf ("%d %d\n", a, b);     
    }
	fclose (file);
	return matrix;        
}

//Prints input matrix
void print_matrix(int *matrix, int lineCount)
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

//Releases memory for matrix
void release_matrix(int *matrix)
{
	free(matrix);
}

//matrix -> input matrix created from pairs in file
//boolean -> matrix generated with transitive closure algorithm
/*int main(int argc, char *argv[])
{
	//Added error checking, going to implement file directory check
	if(argc != 2){fprintf(stderr, "Invalid Arguments.  Usage: functions <filename>\n"); exit(3);}
	int *matrix = read_file(argv[1]);
	printf("Initial matrix:\n");
	print_matrix(matrix);

	int *boolean = create_matrix();

	int i;
	int j;
	for(i = 0; i < number_of_nodes; ++i)
	{
		for(j = 0; j < number_of_nodes; ++j)
		{
			boolean[i*number_of_nodes+j] = matrix[i*number_of_nodes+j];
		}
	}
	
	//Transitive closure algorithm
	int k;
	for(k = 0; k < number_of_nodes; ++k)
	{
		for(i = 0; i < number_of_nodes; ++i)
		{
			for(j = 0; j < numbero_f_nodes; ++j)
			{
				boolean[i*number_of_nodes+j] = (boolean[i*number_of_nodes+j] || (boolean[i*number_of_nodes+k] && boolean[k*number_of_nodes+j]));
			}
		}
	}

	printf("Boolean matrix:\n");

	print_matrix(boolean);

	release_matrix(matrix);
	release_matrix(boolean);

	exit(0);
}*/

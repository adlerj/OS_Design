#include "wtc_btthr.c"
/*#include "wtc_btproc.c"*/

int main(int argc, char *argv[])
{
	if(argc < 3)
	{
		err_exit("wtc");
	}
	switch(atoi(argv[1]))
	{
		case 1:
			printf("Transitive Closure Processes:\n");
			/*wtc_proc(argv[2]);*/
			break;

		case 2:
			printf("Transitive Closure Threads:\n");
			wtc_thr(argv[2]);
			break;

		case 3:
			printf("Transitive Closure Processes Bag of Tasks:\n");
			/*wtc_btproc(argv[2]);*/
			break;

		case 4:
			printf("Transitive Closure Threads Bag of Tasks:\n");
			wtc_btthr(argv[2]);
			break;

		default:
			printf("wtc: Bad selection...\n");
			break;
	}
	return 0;
}
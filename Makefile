CC=gcc

all: wtc_proc wtc_btproc

wtc_btproc: wtc_btproc.o functions2.o
	$(CC) -lrt wtc_btproc.o functions2.o -o wtc_btproc

wtc_proc: wtc_proc.o functions2.o
	$(CC) -lrt wtc_proc.o functions2.o -o wtc_proc

wtc_proc.o: wtc_btproc.c
	$(CC) -c wtc_btproc.c

wtc_proc.o: wtc_proc.c
	$(CC) -c wtc_proc.c

functions2.o: functions2.c
	$(CC) -c functions2.c

clean:
	rm -rf *o functions wtc_proc wtc_btproc

memcheck: 
	clear;make;valgrind --leak-check=yes ./wtc input1.in
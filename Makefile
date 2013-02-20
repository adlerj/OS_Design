CC=gcc

all: wtc_proc wtc_btproc

wtc_btproc: wtc_btproc.o functions2.o
	$(CC) -lrt wtc_btproc.o functions2.o -o wtc_btproc

wtc_proc: wtc_proc.o functions2.o
	$(CC) -lrt wtc_proc.o functions2.o -o wtc_proc

clean:
	 rm -rf *.o wtc_thr wtc_btthr wtc_proc
	 rm -rf *o functions wtc_proc wtc_btproc

memcheck: 
	clear;make;valgrind --leak-check=yes ./wtc input1.in


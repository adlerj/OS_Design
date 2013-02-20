CC=gcc

all: wtc

wtc: wtc_proc.o functions2.o
	$(CC) -lrt wtc_proc.o functions2.o -o wtc

wtc_proc.o: wtc_proc.c
	$(CC) -c wtc_proc.c

functions2.o: functions2.c
	$(CC) -c functions2.c

clean:
	rm -rf *o functions wtc	

memcheck: 
	clear;make;valgrind --leak-check=yes ./wtc input1.in
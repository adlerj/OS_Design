CC=gcc

all: functions

functions: functions2.o
	$(CC) functions2.o -o functions

functions2.o: functions2.c
	$(CC) -c functions2.c

clean:
	rm -rf *o functions	

CC=gcc -Wall -pedantic

all: wtc_thr wtc_btthr wtc_proc

wtc_thr: wtc_thr.c functions.c bit_char.c
	$(CC) wtc_thr.c -o wtc_thr -lpthread

wtc_btthr: wtc_btthr.c functions.c bit_char.c
	$(CC) wtc_btthr.c -o wtc_btthr -lpthread

wtc_proc: wtc_proc.c functions.c bit_char.c
	$(CC) wtc_proc.c -o wtc_proc -lrt

clean:
	rm -rf *.o wtc_thr wtc_btthr wtc_proc

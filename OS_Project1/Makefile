CC=gcc -Wall

wtc: wtc.c wtc_thr.c wtc_btthr.c wtc_proc.c wtc_btproc.c functions.c globals.c bit_char.c
	$(CC) wtc.c -o wtc -lpthread -lrt

clean:
	rm -rf *.o wtc

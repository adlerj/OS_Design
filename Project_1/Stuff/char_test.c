#include <stdio.h>
#include <stdlib.h>

#include "bit_char.c"

int main()
{
	unsigned char *ptr; // array of unsigned ints for bit manipulation
	ptr = bit_array_create(16); // malloc array with at least 16 bits

	unsigned x = bit_array_get(ptr, 3); // get bit3 [ the fourth bit (bits start from 0) ]
	bit_array_set(ptr, 3, 1); // 1000
	bit_array_set(ptr, 2, 1); // 1100
	bit_array_set(ptr, 1, 1); // 1110
	bit_array_set(ptr, 2, 0);
	bit_array_set(ptr, 14, 1);
	unsigned y = bit_array_get(ptr, 3);
	unsigned q = bit_array_get(ptr, 2);
	unsigned m = bit_array_get(ptr, 0);
	printf("bit3 before: %d\nbit3 bit2 bit0 after: %d  %d  %d\n", x,y,q,m);

	bit_array_destroy(ptr); //free malloc from create
	return 0;
}

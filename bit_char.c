unsigned char *bit_array_create(unsigned);
void bit_array_destroy(unsigned char *);
void bit_array_set(unsigned char *, unsigned, int);
unsigned bit_array_get(unsigned char *, unsigned);


unsigned char *bit_array_create(unsigned size)
{
	unsigned char *array = (unsigned char *)calloc(((size/(8*sizeof(unsigned char))) + 1), 1);
	return array;
}

void bit_array_destroy(unsigned char *array)
{
	free(array);
}

void bit_array_set(unsigned char *array, unsigned place, int flag)
{
	if(flag == 0)
	{
		array[place/(8*sizeof(unsigned char))] &= ~( 1 << (place % (8*sizeof(unsigned char))));
	}
	else
	{
		array[place/(8*sizeof(unsigned char))] |= ( 1 << (place % (8*sizeof(unsigned char))));
	}
}

unsigned bit_array_get(unsigned char *array, unsigned place)
{
	if (array[place/(8*sizeof(unsigned char))] & ( 1 << (place % (8*sizeof(unsigned char)))))
		return 1;
	else
		return 0;
}

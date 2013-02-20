unsigned char *bit_array_create(unsigned); /* malloc array of char */
void bit_array_destroy(unsigned char *); /* free mallow */
void bit_array_set(unsigned char *, unsigned); /* takes array and bit to set to 1 */
unsigned bit_array_get(unsigned char *, unsigned); /* takes from array */


unsigned char *bit_array_create(unsigned size) /* size in number of bits */
{
	return (unsigned char *)calloc(((size/(8*sizeof(unsigned char))) + 1), 1); /*malloc amount of bytes needed for bits input */
}

void bit_array_destroy(unsigned char *array) /* take in array from bit_array_create() */
{
	free(array); /*frees array*/
}

void bit_array_set(unsigned char *array, unsigned place) /* take created array and bit to set */
{

	array[place/(8*sizeof(unsigned char))] |= (1 << (place % (8*sizeof(unsigned char)))); /* bitwise OR the specified bit with 1 */ 
}

unsigned bit_array_get(unsigned char *array, unsigned place) /* take created array and returns bit at location */
{
	if (array[place/(8*sizeof(unsigned char))] & ( 1 << (place % (8*sizeof(unsigned char)))))
		return 1; /* return 1 if specified bit is 1 */
	else
		return 0; /* return 0 if specified bit is 0 */
}

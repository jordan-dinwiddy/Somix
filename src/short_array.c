#include <stdlib.h>
#include <stdio.h>
#include "short_array.h"

struct short_array *short_array_init(int size)
{
	struct short_array *retval = (struct short_array *) 
					malloc(sizeof(struct short_array));

	retval->data = malloc(size * sizeof(short));
	retval->capacity = size;
	retval->size = 0;

	return retval;
}

void short_array_add(short s, struct short_array *array)
{
	if(array->capacity == array->size) {
		/* we will have to allocate more memory */
		array->capacity *= ARR_SIZE_INCREASE_FACTOR;
		
		array->data = (short *) realloc(array->data, 
			array->capacity * sizeof(short));
	}

	array->data[array->size] = s;
	array->size++;
}

/**
 * Prints the given array to the specified file pointer.
 */
void short_array_print_unsigned(struct short_array *array, FILE *fp)
{
	int ix;
	for(ix = 0; ix < array->size; ix++) 
			fprintf(fp, "%d,%u\n", ix+1, 
				(unsigned short) array->data[ix]);
}

/**
 * Writes the given array to the specified file.
 *
 * Returns 0 on success.
 */
int short_array_unsigned_write(struct short_array *array, const char *filename)
{
	FILE *fp;
	fp = fopen(filename, "w");

	if(fp == NULL) return -1;

	short_array_print_unsigned(array, fp);

	return 0;
}

void short_array_destroy(struct short_array *array)
{
	free(array->data);
	free(array);
}

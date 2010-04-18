#include <stdio.h>

#define ARR_SIZE_INCREASE_FACTOR 2
#define ARR_DEFAULT_SIZE 1024

struct short_array {
	int capacity;		/* how many we can store - without resizing */
	int size;		/* how many we are storing */
	short *data;		/* array of shorts */
};

struct short_array *short_array_init(int size);
void short_array_add(short s, struct short_array *array);
void short_array_print_unsigned(struct short_array *array, FILE *fp);
int short_array_unsigned_write(struct short_array *array, 
	const char *filename);
void short_array_destroy(struct short_array *array);

#include <stdio.h>
#include <stdlib.h>
#include "comms.h"

/**
 * For fatal error messages issued.
 */
void panic(const char *s)
{
	fprintf(stderr, "PANIC: %s\n", s);
	exit(-1);
}


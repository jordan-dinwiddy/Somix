#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "comms.h"

/**
 * For fatal error messages issued.
 */

void panic(const char *format, ...)
{
	va_list args;
	fprintf(stderr, "PANIC: ");
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");

	exit(-1);
}


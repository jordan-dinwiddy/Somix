#include <stdio.h>

//#define DEBUG

#ifdef DEBUG
#define debug(format, args...) printf(format "\n", ##args);
#else
#define debug(format, args...)
#endif


#define info(format, args...) printf("INFO: "); printf(format "\n", ##args);
/**
 * For fatal error messages issued.
 */
void panic(const char *format, ...);




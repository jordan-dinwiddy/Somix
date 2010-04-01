#include <stdio.h>

//#define DEBUG

/* Info levels. Getting more verbose the higher we go.
 * More important messages will be recorded at lower info levels.
 */
#define INFO_LEVEL_1
//#define INFO_LEVEL_2

#ifdef DEBUG
#define debug(format, args...) printf(format "\n", ##args);
#else
#define debug(format, args...)
#endif


#ifdef INFO_LEVEL_1
#define info_1(format, args...) printf("INFO: "); printf(format "\n", ##args);
#else
#define info_1(format, args...)
#endif

#ifdef INFO_LEVEL_2
#define info(format, args...) printf("INFO: "); printf(format "\n", ##args);
#else
#define info(format, args...)
#endif

/**
 * For fatal error messages issued.
 */
void panic(const char *format, ...);




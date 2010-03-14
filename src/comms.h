#define DEBUG

#ifdef DEBUG
#define debug(format, args...) printf(format "\n", ##args);
#else
#define debug(format, args...)
#endif


/**
 * For fatal error messages issued.
 */
void panic(const char *format, ...);




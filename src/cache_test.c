#include <stdio.h>
#include "const.h"
#include "cache.h"

int main(void)
{
	printf("Attempting to create new block.YO..\n");
	
	init_cache();
	print_cache();
	return 0;
}

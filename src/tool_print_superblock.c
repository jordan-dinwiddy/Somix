#include <stdio.h>
#include <stdlib.h> /* has mode_t defined somehwhere within */

#include "minix_fuse_lib.h"
struct minix_fuse_fs fs;

int main(int argc, char **argv)
{
	char *device = argv[1];

	printf("printing superblock on device=%s...\n",  device);
// uyo
	if(!minix_openfs(device, &fs)) {
		printf("failed to open device\n");
		return -1;
	}
	minix_print_sb();
	return 1;
}

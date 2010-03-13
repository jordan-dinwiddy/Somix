#include <stdlib.h>
#include <stdio.h>
#include "minix_fuse_lib.h"

struct minix_fuse_fs fs;

int main(int argc, char **argv)
{
	char *device = argv[1];

	printf("printing bitmaps for device=%s...\n", device);

	if(!minix_openfs(device, &fs)) {
		printf("failed to open device\n");
		return -1;
	}

	printf("inode bitmap:\n");
	print_bitmap(fs.inode_bmap);

	printf("\nzone bitmap:\n");
	print_bitmap(fs.zone_bmap);
	return 1;
}

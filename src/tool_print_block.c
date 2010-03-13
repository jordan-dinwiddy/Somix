#include <stdio.h>
#include <stdlib.h>
#include "minix_fuse_lib.h"

struct minix_fuse_fs fs;

int main(int argc, char **argv)
{
	char *device = argv[1];
	int block = atoi(argv[2]);

	printf("printing block %d on device=%s...\n", block, device);

	if(!minix_openfs(device, &fs)) {
		printf("failed to open device\n");
		return -1;
	}

	struct minix_block *blk = get_block(block);
	if(blk == NULL) {
		printf("failed to load block\n");
		exit(-1);
	}

	print_buf(blk->data, BLOCK_SIZE);
	printf("\n");
	return 1;
}
